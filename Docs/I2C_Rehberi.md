# I2C — Tekrar / Çalışma Rehberi

Bu doküman `Docs/I2C.md`'nin (sürücü yazılmadan önceki ön rapor) devamı;
orada protokol ve register teorisi anlatılmıştı, burada ise **gerçekten
yazılmış olan** `src/drivers/i2c/i2c.c` / `i2c.h` sürücüsü üzerinden konuyu
tekrar ediyoruz. Amaç: "neden böyle yazıldı"yı kod satırlarıyla eşleştirip
pekiştirmek.

Referans: RM0090, "Inter-integrated circuit (I2C) interface" bölümü.

---

## 1. Protokolün Tek Cümlelik Özeti

I2C, 2 hatlı (`SCL` saat, `SDA` veri, ikisi de **open-drain**), adresle
çoklu-slave destekleyen bir bus. Bir transfer her zaman şu iskelette:

```
START → [ADRES(7bit) + R/W̄ bit] → ACK → VERI byte(lar) + ACK/NACK → STOP
```

- **START**: SCL=1 iken SDA 1→0.
- **STOP**: SCL=1 iken SDA 0→1.
- **ACK**: alıcı, aldığı her byte'tan sonra 9. clock'ta SDA'yı LOW çeker.
- **Repeated START**: STOP atmadan tekrar START — yönü (yazmadan okumaya)
  bus'ı bırakmadan değiştirmek için kullanılır. Sensör register okumanın
  standart deseni budur (bkz. §5).

---

## 2. Kendi Kendine Test: Bu Soruları Cevaplayabiliyor musun?

Okumaya geçmeden önce (veya okuduktan sonra tekrar) kendine sor:

1. `SR1` okuduktan sonra neden ayrıca `SR2` de okunuyor (`ADDR` temizleme)?
2. 1 byte okurken `ACK=0` niye `ADDR` temizlenmeden **önce** ayarlanıyor?
3. `I2C_MemRead` içeride `I2C_MasterSend` + `I2C_MasterReceive`'i nasıl
   zincirliyor, `Sr` parametresi her çağrıda ne oluyor?
4. `I2C_MasterSendIT` çağrıldığında fonksiyon hemen mi dönüyor, yoksa byte'lar
   gönderilene kadar mı bekliyor? Asıl transfer nerede oluyor?
5. `CR2.FREQ` ile `CCR` neden birbirinden bağımsız girilmesi gereken iki
   ayrı alan?

Cevapları bilmiyorsan ilgili bölüme git, okuduktan sonra tekrar dene.

---

## 3. Donanım Bağlantısı (Bu Projede)

`src/main.c` → `GPIO_Config()`:

| Sinyal | Pin | Mod | Detay |
|---|---|---|---|
| SCL | PB6 | AF4 | Open-drain + pull-up |
| SDA | PB7 | AF4 | Open-drain + pull-up |

**Neden open-drain?** I2C hattı sadece LOW'a çekilebilir; HIGH'a çıkış
dış pull-up dirençle olur — birden fazla cihaz aynı hatta çakışmadan
"LOW'a çekme" yarışına girebilsin diye (push-pull olsaydı iki cihaz aynı
anda biri HIGH biri LOW yazınca kısa devre olurdu).

`GPIO_Config()`'te dikkat edilen (ve daha önce düzeltilen) noktalar:
- `GPIOB` clock'unun açılması gerekiyor (`GPIO_PeriClockControl(GPIOB, ENABLE)`)
  — SPI'nin `GPIOA`'sından ayrı bir port.
- `PinSpeed` alanının set edilmesi gerekiyor — boş bırakılırsa struct'ın
  stack'teki çöp değeri `GPIO_Init`'in validasyonundan geçemeyip pin hiç
  konfigüre edilmeden sessizce başarısız olabiliyor.

---

## 4. Saat Ayarı: `I2C_Init` Ne Hesaplıyor?

`i2c.c:51-146`. Sırayla:

1. **`RCC_GetPCLK1Value()`** ile gerçek `PCLK1` (Hz) alınır, `/1000000` ile
   `freq_mhz` çıkarılır (bu projede `PCLK1=42MHz` → `freq_mhz=42`).
2. **`CR2.FREQ`** bu MHz değeriyle yazılır — donanımın *iç* zamanlama
   mantığı için ayrı bir girdi; `CCR` doğru olsa bile `FREQ` yanlışsa SCL
   periyodu bozulur (`CCR`'den bağımsız, unutulması kolay bir alan).
3. **`OAR1`** yazılır — bit14 donanım gereği her zaman 1, adres `ADD[7:1]`'e
   kayarak yerleşir (bu proje sadece master kullandığından pratik önemi
   düşük ama `I2C_Init` her durumda set ediyor).
4. **`CCR`** hesaplanır:
   - Standart mod (≤100kHz): `CCR = PCLK1 / (2 × SCL)`
   - Fast mod, `DUTY=0`: `CCR = PCLK1 / (3 × SCL)`
   - Fast mod, `DUTY=1` (16/9): `CCR = PCLK1 / (25 × SCL)`
   - RM0090 kuralı: `CCR` en az `4` olmalı — kod bunu clamp ediyor
     (`i2c.c:131-134`).
5. **`TRISE`**:
   - Standart mod: `TRISE = freq_mhz + 1`
   - Fast mod: `TRISE = (freq_mhz × 300 / 1000) + 1`

Bu projede `I2C_SPEED_STANDARD` (100 kHz) kullanılıyor (`main.c` →
`I2C_Config()`), yani `CCR = 42 000 000 / 200 000 = 210`,
`TRISE = 42 + 1 = 43`.

---

## 5. Master Transmit — Kod ile Adım Adım

`I2C_MasterSend()` (`i2c.c:231-258`), üç yardımcıya bölünmüş:

```
I2C_MasterAddressPhaseWrite()   → START + SB bekle + ADRES(W) + ADDR bekle + SR1/SR2 oku
I2C_MasterWriteBytes()          → her byte icin TXE bekle, DR'a yaz
I2C_MasterWaitTransferComplete()→ TXE VE BTF bekle (son byte fiziksel olarak da gitti)
```

En kritik satır `I2C_MasterAddressPhaseWrite` içinde:

```c
while(I2C_GetFlagStatus(pI2Cx, I2C_SR1_ADDR) != STATUS_OK);
uint32_t dummy = pI2Cx->SR1;
dummy = pI2Cx->SR2;   // <-- bu satir olmadan ADDR biti temizlenmez
```

`ADDR` biti donanımda **hem `SR1` hem `SR2` okunana kadar** set kalır.
Sadece `SR1` okuyup geçmek — en klasik I2C bug'ı — kodu bir sonraki adımda
sonsuza kadar bekletir. Bu proje `SR2`'yi bilinçli olarak (kullanılmasa
bile) okuyup `(void)dummy` ile "okundu, atıldı" diyor.

`Sr` parametresi en sonda devreye giriyor: `I2C_DISABLE_SR` ise `STOP`
üretilir, `I2C_ENABLE_SR` ise üretilmez — bus repeated START için açık
bırakılır (bir sonraki çağrı doğrudan yeni bir `START` atacak).

---

## 6. Master Receive — Asıl Zor Kısım

`I2C_MasterReceive()` (`i2c.c:260-328`). Burada ACK/NACK'in **DR
okunmadan önce** doğru ayarlanmış olması gerekiyor çünkü ACK/NACK kararı
"bir sonraki byte geldiğinde ne yapılacağı"nı belirliyor — donanım bunu
geç öğrenirse fazladan byte kabul eder.

Kodda iki özel dal var:

**length == 1:**
```c
if(length == 1) {
    pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);   // ADDR temizlenmeden ONCE NACK
}
/* ... SR1/SR2 oku (ADDR temizle) ... */
if(length == 1 && Sr == I2C_DISABLE_SR) {
    I2C_Generate_Stop_Condition(pI2Cx);   // ADDR temizlenir temizlenmez STOP
}
```

**length >= 2, döngü içinde sondan bir önceki byte'ta:**
```c
if(length > 1 && i == (length - 2)) {
    pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
    if(Sr == I2C_DISABLE_SR) I2C_Generate_Stop_Condition(pI2Cx);
}
*pRxBuffer = (uint8_t)pI2Cx->DR;   // simdi son byte'i RXNE ile oku
```

Neden "sondan bir önceki"? Çünkü ACK biti, DR'den okunacak *bir sonraki*
byte için geçerli olacak — yani son byte'ı NACK'letmek istiyorsak, son
byte gelmeden **önce** (yani sondan bir önceki byte işlenirken) ACK'i
kapatmalıyız. `Docs/I2C.md §6.2`'de anlatılan RM0090'ın "N≥3" akışıyla
birebir aynı; bu sürücü ayrıca `POS` bitini kullanmadan 2-byte durumunu da
aynı genel kuralla (`i == length-2`) çözüyor — ön rapordaki notta
("basitleştirilmiş polling yolu... doğrulanmalı") bahsedilen yaklaşımın
uygulamaya geçmiş hali bu.

Fonksiyon sonunda `ACK` biti kullanıcı config'ine göre geri açılıyor
(`i2c_config.ACKControl`) — aksi halde bir sonraki `I2C_MasterReceive`
çağrısı NACK'lenmiş bus üzerinde başlardı.

---

## 7. Repeated Start ile Register Okuma: `I2C_MemRead`/`I2C_MemWrite`

Gerçek dünyada (örn. bir sensörden register okumak) en sık kullanılan
desen: *"şu register'ı yaz, sonra STOP atmadan aynı slave'den oku"*.
`i2c.c:651-691`:

```c
Status_t I2C_MemRead(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr,
                      uint8_t MemAddr, uint8_t *pRxBuffer, uint32_t length)
{
    I2C_MasterSend(pI2CHandle, &MemAddr, 1, SlaveAddr, I2C_ENABLE_SR);  //STOP atma
    return I2C_MasterReceive(pI2CHandle, pRxBuffer, length, SlaveAddr, I2C_DISABLE_SR);
}
```

Bus üzerinde gerçekleşen:

```
START → ADDR+W → ACK → MemAddr → ACK →
REPEATED START → ADDR+R → ACK → DATA(length byte) → NACK(son) → STOP
```

`I2C_MemWrite` ise **farklı bir sınıf problem**: register adresi ile veri
byte'ları *aynı* kesintisiz yazma fazında gitmeli (repeated START ile
değil!) — araya START/STOP girerse slave `MemAddr`'ı yeni bir işlem
sanır ve veri yanlış yere yazılır. Bu yüzden `I2C_MasterSend`'i
çağırmıyor, kendi START/STOP akışını (`I2C_MasterAddressPhaseWrite` +
iki ardışık `I2C_MasterWriteBytes` + tek `STOP`) yürütüyor.

**Kısaca:** okuma repeated-START zincirler (iki ayrı fonksiyon çağrısı),
yazma tek fazda birleştirir (tek fonksiyon içinde iki `WriteBytes`).

---

## 8. Interrupt Tabanlı Versiyon — Aynı Kurallar, Durum Makinesine Dağılmış

`I2C_MasterSendIT`/`I2C_MasterReceiveIT` (`i2c.c:374-437`) **hemen döner**
— transferi başlatıp `Handle`'a (`pTxBuffer`, `TxLen`, `TxRxState=I2C_BUSY_IN_TX`
vb.) durum bilgisini kaydeder ve `START`'ı tetikler; asıl byte-byte
aktarım `I2C_EV_IRQHandling()` (`i2c.c:557-593`) içinden, olay geldikçe
çağrılan alt handler'larda olur:

| Olay | Handler | Blocking karşılığı |
|---|---|---|
| `SB` (start gönderildi) | `I2C_SB_Interrupt_Handle` | `while(SB...)` sonrası adres yazma |
| `ADDR` (adres ACK'lendi) | `I2C_ADDR_Interrupt_Handle` | `SR1`/`SR2` okuma + 1-byte özel durumu |
| `TXE` (DR boş) | `I2C_TXE_Interrupt_Handle` | `I2C_MasterWriteBytes` döngüsü |
| `RXNE` (DR dolu) | `I2C_RXNE_Interrupt_Handle` | `I2C_MasterReceive` döngüsü, `i==length-2` kuralı |
| `BTF` (byte transfer bitti) | `I2C_BTF_Interrupt_Handle` | `I2C_MasterWaitTransferComplete` |

Yani **aynı RM0090 kuralları** (ADDR temizleme sırası, ACK'in DR
okumadan önce ayarlanması) burada da geçerli — sadece "bekle" yerine
"kesme geldiğinde devam et" haline getirilmiş. `I2C_ApplicationEventCallback()`
(zayıf/weak fonksiyon, `SPI_ApplicationEventCallback` ile aynı desen)
transfer bitince (`I2C_EVENT_TX_CMPLT`/`I2C_EVENT_RX_CMPLT`) veya hata
oluşunca (`I2C_ERROR_AF` vb.) uygulama koduna haber verir.

**Hata durumunda kilitlenmemek için önemli detay:** `I2C_ER_IRQHandling()`
içinde `AF` (adres NACK) geldiğinde `TxRxState` `I2C_READY`'e
resetleniyor (`I2C_CloseSendData`/`I2C_CloseReceiveData` ile) — aksi
halde bir sonraki `I2C_MasterSendIT` çağrısı hep `STATUS_BUSY` dönerdi.

---

## 9. Hata Bayrakları — Hızlı Referans

| Bayrak | Ne zaman olur | Bu projede en olası sebep |
|---|---|---|
| `AF` | Beklenen ACK gelmedi | Yanlış slave adresi, kablo kopuk, slave güç yok |
| `BERR` | START/STOP sırasında protokol ihlali | Elektriksel gürültü |
| `ARLO` | Çoklu master, bus kaybı | Tek master (STM32) olduğu için pratikte oluşmamalı |
| `OVR` | `DR` zamanında okunmadı/yazılmadı | SPI'daki `OVR` ile aynı aile |
| `TIMEOUT` | `SCL` 25ms'den uzun LOW'da kaldı | Bus kilitlenmesi (§10) |

---

## 10. Bilinen Sınırlamalar / Sonraki Bakılacak Yerler

- **DMA tabanlı I2C** (`I2C_MasterSendDMA`/`I2C_MasterReceiveDMA`)
  header'da (`i2c.h:110-111`) deklare edilmiş ama `i2c.c`'de henüz
  implementasyonu yok — SPI'daki DMA deseni (`Docs/DMA.md`) buraya
  taşınacaksa bir sonraki adım burası.
- **Bus kilitlenme kurtarma** (bir slave transfer ortasında SDA'yı LOW'da
  bırakırsa) bu sürücüde ele alınmıyor — gerçek donanımda "sensör hiç
  cevap vermiyor" görülürse ilk bakılacak yer.
- **Slave modu** (`I2C_SlaveSendData`/`I2C_SlaveReceiveData`) tek-byte
  seviyesinde var ama `ADDR`/genel çağrı (general call) senaryoları için
  ayrı bir örnek/test yok.

---

## 11. Özet Tablo — Fonksiyon → Ne Zaman Kullanılır

| Fonksiyon | Kullanım |
|---|---|
| `I2C_MasterSend` | Blocking, tek yönlü yazma (STOP ile bitirir veya `Sr` ile açık bırakır) |
| `I2C_MasterReceive` | Blocking, tek yönlü okuma |
| `I2C_MemRead` | "Register adresi yaz + repeated START + oku" — sensör okuma deseni |
| `I2C_MemWrite` | "Register adresi + veri, aynı kesintisiz fazda" — sensör yazma deseni |
| `I2C_MasterSendIT`/`ReceiveIT` | Non-blocking, CPU'yu meşgul etmeden gönder/al |
| `I2C_SlaveSendData`/`ReceiveData` | Slave modda tek byte gönder/al (EV_IRQHandling içinden) |

`src/main.c` içindeki örnek (`main()` fonksiyonu) `I2C_MasterSend` +
`I2C_MasterReceive`'i `Sr=I2C_ENABLE_SR` ile zincirleyerek aslında
`I2C_MemRead`'in elle açılmış hali — ikisini karşılaştırmak iyi bir
tekrar egzersizi.
