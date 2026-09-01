# I2C (Inter-Integrated Circuit) — Sürücü Öncesi Ön Rapor

Bu doküman, `FC_BSP` projesine I2C sürücüsü eklemeden önce protokolü ve
STM32F4'ün I2C çevre birimini donanım seviyesinde anlamak için hazırlandı.
`Docs/RCC.md` ve `Docs/DMA.md` ile aynı amaca hizmet ediyor: kod yazmaya
başlamadan önce *neden* böyle tasarlanacağını görmek.

Referans kaynak: RM0090 (STM32F405/415, STM32F407/417, STM32F427/437,
STM32F429/439 Reference Manual), "Inter-integrated circuit (I2C) interface"
bölümü.

---

## 1. I2C nedir, neden var?

SPI'da 4 hat (SCK, MOSI, MISO, CS) ve her slave için ayrı bir CS pini
gerekiyordu. I2C sadece **2 hat** kullanır — `SCL` (saat) ve `SDA` (veri) —
ve aynı hatta **birden fazla slave** aynı anda durabilir; hangisiyle
konuşulacağı CS pini yerine bir **7-bit adres** ile belirlenir.

Bu projenin bağlamında (`READ.md`) kendi kartınızdaki MPU6050 IMU'su I2C
üzerinde. SPI'a göre daha az pin kullanır ama (bu dokümanın asıl konusu)
protokol tarafı SPI'dan belirgin biçimde daha karmaşıktır: SPI'da "saat at,
veri kay" yeterliyken I2C'de adresleme, ACK/NACK, START/STOP koşulları gibi
durum makinesi adımları var.

**Kritik fiziksel fark:** I2C hatları **open-drain**'dir — çevre birimi
hattı sadece LOW'a çekebilir, HIGH'a çıkaramaz; HIGH'a çıkış harici
**pull-up dirençleri** ile olur (tipik 4.7kΩ, bazı IMU modüllerinde karta
lehimli gelir). GPIO tarafında bu, `GPIO_OUTPUT_OPEN_DRAIN` + pull-up
seçilerek yapılır — `gpio.h`'de bu mod zaten var (`GPIO_OTYPE_t`), ekstra
bir şey eklemeye gerek yok.

---

## 2. STM32F407 I2C Mimarisi

```
                    RCC->APB1ENR (I2C1EN=bit21, I2C2EN=bit22, I2C3EN=bit23)
                                  │
              ┌───────────────────┼───────────────────┐
              ▼                   ▼                   ▼
         ┌─────────┐        ┌─────────┐         ┌─────────┐
         │  I2C1    │        │  I2C2    │         │  I2C3    │   (hepsi APB1, max 42 MHz)
         └─────────┘        └─────────┘         └─────────┘
              │
     PB6/PB8 = SCL (AF4)     <- STM32F407 Discovery/genel kartlarda tipik
     PB7/PB9 = SDA (AF4)        eşleme; kesin pin kart şeması onaylanınca
                                 sabitlenecek (bkz. §9)
```

`RCC->APB1ENR` bitleri zaten `Inc/Device/stm32f4xx.h:381-383`'te tanımlı
(`I2C1_PCLK_EN()` vb.) — `RCC.md`'de gördüğümüz desenin aynısı, ekstra bir
şey eklemeye gerek yok.

**Neden APB1, APB2 değil?** `RCC.md` §2'deki ayrımın aynısı: I2C1-3 düşük
hızlı APB1 periferikleri, max 42 MHz. Bu projede `PCLK1 = 42 MHz`
(`RCC.md` §7) — CCR/TRISE hesaplarının temeli bu değer olacak (§5).

- Hatırlatma : APB1 (42 MHz) , APB2 (84 MHz)

---

## 3. Protokol Temelleri — Bit Seviyesinde Bus

```
SDA:  ‾‾‾\___/‾‾‾\_ADDR[6:0]_/R̄W\_ACK\_DATA[7:0]_\_ACK\_______/‾‾‾
SCL:  ‾‾‾‾‾\_/‾\_/‾\_/‾\_...‾\_/‾\_/‾\_/‾\_...‾\_/‾\_/‾‾‾‾‾‾‾‾‾‾
      │    │                              │                 │
    (bosta) START                                          STOP
           (SCL=1 iken               ADDR/DATA sirasinda    (SCL=1 iken
            SDA 1→0)                 SCL=1 iken SDA sabit    SDA 0→1)
                                      olmali -- degisim
                                      sadece SCL=0 iken
```

Adım adım:

1. **START (S):** SCL yüksekken SDA'nın 1→0 düşmesi. Bundan önce hat
   "boşta" (her iki hat da idle'da pull-up ile HIGH).
2. **Adres + R/W̄:** Master, 7-bit slave adresini gönderir, ardından 1 bit
   yön (`0`=yazma/master→slave, `1`=okuma/slave→master).
3. **ACK/NACK:** Her 8 bitlik byte'tan sonra alıcı 9. saat darbesinde
   `SDA`'yı LOW çeker (ACK = "aldım"); çekmezse (SDA HIGH kalır) NACK.
   Adres byte'ında slave kendi adresini tanırsa ACK verir — hiçbir slave
   ACK vermezse (yanlış adres/cihaz yok) master bunu görür.
4. **Veri byte'ları:** MSB-first, her biri kendi ACK/NACK'iyle.
5. **STOP (P):** SCL yüksekken SDA'nın 0→1 çıkması. Hat tekrar boşta.
6. **Repeated START:** STOP atmadan tekrar START atmak — bus'ı bırakmadan
   yönü değiştirmeye yarar (bkz. §7, sensör register okuma deseni tam
   olarak bunu kullanır).

**SPI ile temel fark:** SPI'da CS ile "kiminle konuştuğunuz" fiziksel
kablolamada belli; I2C'de bu bilgi protokolün içinde (adres byte'ı) taşınır
— bu yüzden I2C durum makinesi SPI'dan daha fazla adıma bölünür ve STM32
tarafında her adımın kendi bayrağı var (§4).

---

## 4. İlgili Register'lar

`I2Cx_RegDef_t` zaten `Inc/Device/stm32f4xx.h:230-241`'de tanımlı (DMA'nın
aksine burada struct hazır, sadece bit pozisyon makroları eksik — DMA'da
`DMA_SxCR_EN` gibi makroları `dma.c`'ye eklerken yaptığımızın aynısı I2C
için de yapılacak):

| Register | Anlamı |
|---|---|
| `I2C_CR1` | `PE` (periferik enable), `START`, `STOP`, `ACK`, `SWRST`, `POS` (2-byte okuma özel biti, §6.2) |
| `I2C_CR2` | `FREQ[5:0]` (APB1 saat frekansı **MHz** cinsinden, donanımın zamanlama hesapları için — CCR/TRISE'dan bağımsız, ayrıca girilmesi gerekir), `ITEVTEN`/`ITBUFEN` (kesme), `DMAEN` |
| `I2C_OAR1`/`OAR2` | Kendi adresimiz (slave modda; bu proje sadece master kullanacağı için önemi düşük) |
| `I2C_DR` | Gönderme/alma veri register'ı (8 bit) |
| `I2C_SR1` | `SB` (start gönderildi), `ADDR` (adres ACK'lendi), `BTF` (byte transfer bitti), `TXE`, `RXNE`, hata bayrakları (`AF`, `ARLO`, `BERR`, `OVR`, `TIMEOUT`) |
| `I2C_SR2` | `MSL` (master modda mıyız), `BUSY`, `TRA` (transmitter mı receiver mı) — **`ADDR`'ı temizlemek için okunması zorunlu** (§6.1) |
| `I2C_CCR` | SCL periyodunu belirleyen bölme değeri + `F/S` (standart/fast mod) + `DUTY` |
| `I2C_TRISE` | Maksimum SCL yükselme süresi (§5) |
| `I2C_FLTR` | Dijital/analog gürültü filtresi (bu projede varsayılan/kapalı kalabilir) |

**SPI'daki `SR` ile karşılaştırma:** SPI'da tek bir `SR` vardı, I2C'de
`SR1`/`SR2` diye ikiye bölünmüş ve **`SR1` okuduktan sonra `SR2`'yi de
okumak bazı bayrakları temizlemenin bir parçası** — bu, projede daha önce
gördüğümüz "flag semantiği" derslerinden bir adım daha sinsi bir versiyon
(§6.1'de detay).

---

## 5. Saat Hesabı: `CCR` ve `TRISE`

I2C'de SPI'daki gibi basit bir "böl-2'nin-kuvveti" (`BR[2:0]`) yok;
donanım size `CCR` (kaç `TPCLK1` periyodunda bir SCL yarı-periyodu) ve
`TRISE` (yükselme süresi limiti, saat periyoduna eklenecek marj) hesaplatır.

**`CR2.FREQ`** — donanımın iç zamanlamalarında kullandığı, `APB1` saatini
**MHz** cinsinden bildiren alan (bu projede `PCLK1 = 42 MHz` →
`FREQ = 42`). `RCC_GetPCLK1Value()` (`rcc.h`, `RCC.md` §6.4'te tanıtıldı)
tam bu değeri Hz cinsinden verir — I2C sürücüsü bunu çağırıp `/1000000`
yapacak.

**Standart mod (100 kHz), `CCR` formülü:**

```
CCR = PCLK1 / (2 × 100000)
```

`PCLK1 = 42 MHz` için: `CCR = 42 000 000 / 200 000 = 210`

**Fast mod (400 kHz), `DUTY=0` (1:2 oran) formülü:**

```
CCR = PCLK1 / (3 × 400000)
```

`PCLK1 = 42 MHz` için: `CCR = 42 000 000 / 1 200 000 = 35`

**`TRISE` formülü (her iki modda da):**

```
TRISE = (PCLK1_MHz × maks_yukselme_suresi_ns / 1000) + 1
```

Standart modda maks yükselme süresi 1000 ns, fast modda 300 ns (I2C
spesifikasyonu sabitleri):

- Standart mod: `TRISE = (42 × 1000 / 1000) + 1 = 43`
- Fast mod: `TRISE = (42 × 300 / 1000) + 1 = 13.6 → 13` (tamsayıya
  yuvarlanır, `RCC.md`'nin `>>`/tamsayı bölme yaklaşımıyla tutarlı)

**MPU6050 için pratik seçim:** Datasheet'i standart modu (100 kHz) garanti
eder, fast modu (400 kHz) da destekler ama bazı klon/modül kartlarda hat
kapasitansı fast modda gürültüye daha duyarlı olabilir. İlk sürümde
**100 kHz** ile başlayıp hat sağlamsa 400 kHz'e geçmek makul bir sıra —
tıpkı `SPI_PRIORITY_HIGH` yerine önce düşük saat hızıyla doğrulama
yaklaşımı gibi.

---

## 6. Master Modda Tipik İşlem Akışı

### 6.1 Master Transmit (register'a yazma)

RM0090'ın master transmit akış şeması, register seviyesinde şöyle:

1. `CR1.START = 1` yaz → donanım START koşulunu üretir.
2. `SR1.SB` set olana kadar bekle (start gönderildi).
3. `DR`'a slave'in 7-bit adresini + `0` (yazma) yaz.
4. `SR1.ADDR` set olana kadar bekle (slave adresi ACK'ledi).
5. **`ADDR`'ı temizle: önce `SR1` sonra `SR2` oku** — sadece `SR1` okumak
   yetmez, `ADDR` biti donanımda `SR1` **ve** `SR2` okunana kadar set
   kalır. Bunu atlarsanız kod bir sonraki adımda sonsuza kadar bekler
   (klasik I2C bug'ı, STM32 forumlarında en sık sorulan konulardan biri).
6. Veri byte'larını `SR1.TXE` bekleyerek `DR`'a yaz (SPI'daki
   `SPI_Send`'in `TXE` bekleme mantığıyla birebir aynı desen).
7. Son byte yazıldıktan sonra `SR1.BTF` bekle (shift register da boşaldı,
   SPI'daki `BSY` bekleme mantığının I2C karşılığı — `Docs/DMA.md` §6'da
   SPI için anlattığımız "DMA bitti ≠ fiziksel olarak bitti" dersinin
   aynısı burada da geçerli).
8. `CR1.STOP = 1` yaz → STOP koşulu üretilir, bus serbest kalır.

### 6.2 Master Receive (register okuma) — asıl tuzak burada

Alma tarafı, ACK/NACK'in **doğru bir sonraki byte'tan önce** ayarlanması
gerektiği için üç ayrı özel duruma ayrılır (RM0090'ın en çok dikkat
çektiği kısım):

| Kaç byte okunacak | Prosedür |
|---|---|
| **1 byte** | `ACK=0` (NACK) START'tan hemen sonra, `ADDR` temizlenmeden **önce** ayarlanmalı; `ADDR` temizlenir temizlenmez `STOP` yazılır; sonra `RXNE` beklenip `DR` okunur. Sıra ters olursa donanım fazladan byte okumaya çalışır. |
| **2 byte** | `CR1.POS = 1` (bu byte çiftinde ACK'i "bir sonraki byte için değil, ikinci byte için" konumlandırır) + `ACK=0`, `ADDR` temizlendikten hemen sonra `BTF` beklenir (RXNE değil!), `STOP` yazılır, sonra iki byte art arda okunur. |
| **N ≥ 3 byte** | Normal akış: `ADDR` temizlenince `ACK=1` kalır, her byte `RXNE` ile okunur; **sondan bir önceki** byte alındığında `ACK=0` yapılır, son byte `RXNE` ile okunmadan hemen önce `STOP` yazılır. |

Bu üç dallı yapı, I2Cv1'in (F1/F4 ailesi; F7/H7'deki I2Cv2/`TIMINGR`
tasarımından farklı, `READ.md` §3.3'te bu ayrıma zaten değinilmiş) en çok
eleştirilen tarafı — donanım tasarımı "bir sonraki byte'ın ACK'i, bu
byte'ın DR okumasından önce ayarlanmalı" kuralını N=1,2 için özel
durumlara zorluyor. `READ.md`'nin kendi notu da bunu doğruluyor
(§8: *"F4 I2C 2 byte okuması RM0090'daki POS bit dizisini kullanmıyor;
basitleştirilmiş polling yolu pratikte çalışıyor ama scope ile
doğrulanmalı"*) — yani bu sürücüde 2-byte durumu için başta basit/polling
bir yol denenip gerçek donanımda (osiloskop veya mantık analizör ile)
doğrulanması planlanıyor.

### 6.3 Repeated Start — sensör register okuma deseni

MPU6050 gibi bir I2C sensöründen register okumak neredeyse her zaman şu
deseni izler — **write sonra STOP atmadan read**:

```
START → ADDR+W → ACK → REG_ADDR → ACK →
REPEATED START → ADDR+R → ACK → DATA(N byte) → NACK(son byte) → STOP
```

Yani `I2C_MasterSendData` (register adresini yaz, STOP **atma**) ile
`I2C_MasterReceiveData` (repeated START ile devam et) birbirine
zincirlenecek — bu, planlanan API'de (§8) `Sr` (repeated start) parametresi
olarak görünecek, tıpkı bazı HAL kütüphanelerindeki `I2C_Master_ReadData(...,
uint8_t Sr)` deseni gibi.

---

## 7. Hata Bayrakları

| Bayrak | Anlamı | Bu projede önemi |
|---|---|---|
| `AF` (Acknowledge Failure) | Beklenen ACK gelmedi (yanlış adres ya da slave yok/meşgul) | En sık görülecek hata — MPU6050 adresi yanlışsa ya da kablolama koptuysa burada patlar |
| `BERR` (Bus Error) | START/STOP sırasında protokol ihlali (yanlış zamanda durum değişikliği) | Genelde gürültü/elektriksel sorun işareti |
| `ARLO` (Arbitration Lost) | Çoklu master senaryosunda bus kaybı | Bu projede tek master (STM32) olduğu için pratikte oluşmamalı |
| `OVR` (Overrun/Underrun) | `DR` zamanında okunmadı/yazılmadı | SPI'daki `OVR` ile aynı aile, aynı `SPI_ClearOVRFlag` mantığıyla temizlenecek |
| `TIMEOUT` | SCL 25 ms'den uzun süre LOW'da kaldı | Donanımsal bir hattın kilitlenmesi belirtisi — kurtarma prosedürü gerekir (§9) |

---

## 8. Bu Sürücüde Planlanan API (taslak, henüz yazılmadı)

`spi.c`'deki `SPI_Send`/`SPI_Receive` ile aynı imza deseni izlenerek,
`SPI_Handle_t`/`DMA_Handle_t` ile aynı "önce config struct'ı doldur, sonra
`_Init` çağır" yaklaşımı korunacak:

```c
typedef struct{
    uint8_t SCLSpeed;     // Refer @I2C_SCLSpeed (100kHz / 400kHz)
    uint8_t DeviceAddress; // sadece slave modda kullanilir
    uint8_t ACKControl;    // Refer @I2C_ACKControl
    uint8_t FMDutyCycle;   // Refer @I2C_FMDutyCycle (sadece fast mode)
}I2C_Config_t;

typedef struct{
    I2Cx_RegDef_t *pI2Cx;
    I2C_Config_t   i2c_config;
    /* ileride IT/DMA icin: pTxBuffer, pRxBuffer, TxLen, RxLen, TxRxState... */
}I2C_Handle_t;

Status_t I2C_Init(I2C_Handle_t *pI2CHandle);

Status_t I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr,
                             uint8_t *pTxBuffer, uint32_t length);
Status_t I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr,
                                uint8_t *pRxBuffer, uint32_t length);
```

İlk sürüm **blocking** olacak (SPI'da izlediğimiz sırayla aynı: önce
blocking, sonra IT, sonra DMA) — MPU6050 gibi düşük hızlı bir sensörde
blocking I2C bile 14 byte'ı ~380 µs'de bitiriyor (`READ.md` §8'de bu rakam
zaten hesaplanmış), yani ilk çalışan sürüm için CPU maliyeti kabul
edilebilir; IT/DMA gerçek darboğaz haline gelirse (yüksek frekanslı
örnekleme gerektiğinde) eklenecek.

---

## 9. Bilinen Zorluklar / Dikkat Edilmesi Gerekenler

- **`ADDR` temizleme sırası (§6.1 adım 5)** — bu sürücüdeki en olası ilk
  bug kaynağı; sadece `SR1` okuyup `SR2`'yi unutmak kodu sonsuz döngüde
  kilitler.
- **1/2 byte okuma özel durumları (§6.2)** — `READ.md`'nin kendi notunda
  da işaretlendiği gibi, önce basit/polling yaklaşımıyla yazılıp gerçek
  donanımda (scope/mantık analizör) doğrulanacak.
- **Pin eşlemesi henüz kart şemasından teyit edilmedi.** §2'deki
  `PB6/PB7` (I2C1, AF4) genel F407 kartlarında tipik ama kendi kartınızın
  gerçek şeması onaylanmadan sabit kabul edilmemeli.
- **Bus kilitlenmesi (slave SDA'yı LOW'da bırakıp bırakırsa)** — I2C'nin
  SPI'a göre bilinen bir zafiyeti: bir transfer yarıda kesilirse (örn.
  reset sırasında) slave SDA'yı LOW'da unutabilir, sonraki START hiç
  başarılı olmaz. Kurtarma için tipik çözüm: I2C'yi devre dışı bırakıp
  SCL/SDA'yı geçici olarak GPIO çıkışına çevirip birkaç saat darbesi
  manuel üretmek — ilk sürümde kapsam dışı, ama gerçek donanımda "sensör
  bazen hiç yanıt vermiyor" sorunuyla karşılaşılırsa bakılacak ilk yer
  burası.
- **`CR2.FREQ` ile `CCR`/`TRISE` birbirinden bağımsız girilir** — `FREQ`
  girilmezse (ya da yanlış girilirse) donanımın kendi iç zamanlamaları
  bozulur, `CCR` doğru olsa bile SCL periyodu yanlış çıkar. SPI'da
  böyle bir "iki kere aynı bilgiyi gir" tuzağı yoktu, I2C'ye özgü.

---

## 10. Sonraki Adım

Rapor burada bitiyor. Bu bilgiler ışığında `i2c.h`/`i2c.c`'yi `spi.c`
deseniyle (önce blocking `I2C_Init`/`I2C_MasterSendData`/
`I2C_MasterReceiveData`, `SR1`/`SR2` bayrak yönetimi dahil) yazmaya
başlayabiliriz.
