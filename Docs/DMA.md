# DMA (Direct Memory Access) — SPI ile Kullanım İçin Ön Rapor

Bu doküman, `FC_BSP` projesine DMA tabanlı SPI transferi eklemeden önce
konuyu donanım seviyesinde anlamak için hazırlandı. `Docs/RCC.md` ile aynı
amaca hizmet ediyor: kod yazmaya başlamadan önce *neden* böyle
tasarlanacağını görmek.

Referans kaynak: RM0090 (STM32F405/415, STM32F407/417, STM32F427/437,
STM32F429/439 Reference Manual), "DMA controller (DMA)" bölümü.

---

## 1. DMA nedir, neden var?

`SPI_Send`/`SPI_Receive`/`SPI_TransmitReceive` (bugüne kadar yazdığımız
her şey) **blocking**: CPU, her byte için `TXE`/`RXNE` bayrağını
bekleyerek döngüde kilitleniyor. `SPI_SendIT`/`SPI_ReceiveIT` bunu
kesmeyle çözdü ama yine de **her byte için** bir kesme (ISR giriş/çıkışı,
kayıt saklama, `SPI_IRQHandling` çağrısı) gerekiyor — 14 baytlık bir IMU
burst okumasında 14 kesme demek.

DMA, CPU'yu bu işten tamamen çıkarır: siz DMA'ya "şu kaynaktan şu hedefe,
şu kadar veriyi, şu periferiğin isteği geldikçe taşı" dersiniz; CPU başka
işlere devam eder, transfer bitince (isterseniz) **tek bir** kesme alır.
Uçuş kontrolcüsü bağlamında (`READ.md`), yüksek frekanslı IMU örneklemesi
ya da blackbox'a sürekli veri yazımı gibi işlerde CPU'yu bu kadar meşgul
etmemek kritik.

---

## 2. STM32F407 DMA Mimarisi

```
                    RCC->AHB1ENR (DMA1EN=bit21, DMA2EN=bit22)
                                  │
              ┌───────────────────┴───────────────────┐
              ▼                                        ▼
         ┌─────────┐                              ┌─────────┐
         │  DMA1    │  (APB1 periferikleri icin)   │  DMA2    │  (APB2 + bellek-bellek)
         └─────────┘                              └─────────┘
              │                                        │
     8 Stream (0-7)                             8 Stream (0-7)
              │                                        │
     her stream'de 8 kanal (0-7)              her stream'de 8 kanal (0-7)
     secilebilir (CHSEL[2:0])                  secilebilir (CHSEL[2:0])
              │                                        │
     her stream'in kendi FIFO'su              her stream'in kendi FIFO'su
     (direct mod ya da FIFO modu)             (direct mod ya da FIFO modu)
```

**Kritik kısıt:** Bir stream aynı anda **tek bir** periferik isteğine
hizmet eder (kanal seçimiyle belirlenir) ve **tek yönlüdür**
(`DIR` alanı: periferik→bellek, bellek→periferik ya da bellek→bellek).
Yani full-duplex bir SPI transferi (aynı anda gönder+al) için **iki ayrı
stream** gerekir: biri TX (bellek→SPI), biri RX (SPI→bellek) — ikisi de
aynı anda başlatılmalı.

**Neden DMA1 *ve* DMA2 var, tek DMA yetmiyor mu?** STM32F4'te AHB
matrisinde DMA1 sadece APB1 tarafındaki periferiklere (SPI2, SPI3,
I2C1-3, USART2-3, TIM2-7...) erişebilir; DMA2 hem APB2 periferiklerine
(SPI1, USART1/6, ADC...) hem de bellek-bellek transferine erişebilir.
Bu, `RCC.md`'de gördüğümüz APB1/APB2 ayrımının DMA tarafındaki yansıması.

---

## 3. Bu Projedeki SPI Çevre Birimleri İçin Stream/Kanal Eşlemesi

RM0090 Tablo 42/43'teki (DMA1/DMA2 istek eşleme tablosu) SPI satırları:

| Periferik | DMA Controller | Stream (TX) | Stream (RX) | Kanal |
|---|---|---|---|---|
| **SPI1** (APB2, bu projede kullandığımız) | DMA2 | Stream3 *veya* Stream5 | Stream0 *veya* Stream2 | 3 |
| SPI2 | DMA1 | Stream4 | Stream3 | 0 |
| SPI3 | DMA1 | Stream5 *veya* Stream7 | Stream0 *veya* Stream2 | 0 |

SPI1 için TX/RX'e iki alternatif stream seçeneği olması (örn. TX için
Stream3 *veya* Stream5), esnekliktir — bir stream başka bir periferik
tarafından kullanılıyorsa diğerine kaçabilirsiniz. Bu projede henüz başka
bir DMA kullanıcısı yok, bu yüzden **SPI1 TX → DMA2 Stream3, SPI1 RX →
DMA2 Stream0** (kanal 3) seçimini öneriyorum — sade ve çakışma riski yok.

**Dikkat:** Bu eşleme tablosu donanımda sabittir, yazılımla değiştirilemez.
Yanlış stream/kanal seçmek **derleme hatası vermez, sessizce hiçbir
transfer olmaz** (DMA isteği hiç gelmediği için) — bu kategori hata, bu
projede daha önce gördüğümüz "flag semantiği ters" türü hatalardan daha
sinsi çünkü derleyici ya da linker yakalayamaz.

---

## 4. İlgili Register'lar (stream başına, `DMA_Stream_RegDef_t` — henüz
tanımlanmadı, `Inc/Device/stm32f4xx.h`'a eklenecek)

| Register | Anlamı |
|---|---|
| `DMA_SxCR` | Stream konfigürasyonu: `EN`, `DIR[1:0]`, `CIRC`, `PINC`/`MINC`, `PSIZE`/`MSIZE[1:0]`, `PL[1:0]` (öncelik), `CHSEL[2:0]`, `TCIE`/`HTIE`/`TEIE`/`DMEIE`, `DBM` (double-buffer) |
| `DMA_SxNDTR` | Kaç veri birimi taşınacak (transfer ilerledikçe geri sayar) |
| `DMA_SxPAR` | Periferik adresi (SPI için `&SPIx->DR`) |
| `DMA_SxM0AR` / `SxM1AR` | Bellek adresi (M1AR sadece double-buffer modunda, `M0AR` ile arka arkaya iki tampon arasında geçiş) |
| `DMA_SxFCR` | FIFO kontrolü: `DMDIS` (direct mod devre dışı → FIFO modu), `FTH[1:0]` (FIFO eşiği), `FS[2:0]` (FIFO durumu, read-only) |
| `DMA_LISR` / `DMA_HISR` | Kesme durumu (Stream 0-3 → LISR, Stream 4-7 → HISR) — `TCIF`, `HTIF`, `TEIF`, `DMEIF`, `FEIF` bitleri |
| `DMA_LIFCR` / `DMA_HIFCR` | Kesme bayraklarını temizleme (`SR` gibi otomatik temizlenmez, bu register'a `1` yazılır) |

**`SPI_CR1_SPE` ile paralel bir kural:** `DMA_SxCR.EN` biti set olduğunda
stream aktif olur; **stream çalışırken `NDTR`/`PAR`/`M0AR` ya da `CR`'nin
çoğu alanı değiştirilemez** — tıpkı SPI'de `CR1` yazmadan önce `SPE=0`
gerektiği gibi, burada da önce `EN=0` yapıp donanımın gerçekten
durduğunu (`EN` biti donanım tarafından 0'a düşene kadar) beklemek
gerekiyor.

---

## 5. SPI Tarafında Gerekli Tek Şey: `CR2.TXDMAEN` / `CR2.RXDMAEN`

`spi.c`'de zaten `SPI_CR2_RXDMAEN` (bit 0) ve `SPI_CR2_TXDMAEN` (bit 1)
makroları tanımlı (`stm32f4xx.h:477-478`) ama hiç kullanılmadı — interrupt
altyapısında `TXEIE`/`RXNEIE` kullanmıştık, DMA modunda onların yerini
bu iki bit alıyor. SPI çevre biriminin kendisi DMA'nın var olduğunu
bilmez; sadece "TXE/RXNE oluştuğunda beni DMA'ya bildir" der, DMA
tarafını tamamen stream'in kendi konfigürasyonu yönetir.

---

## 6. Tipik Kurulum Akışı (SPI1 TX örneği, DMA2 Stream3)

1. `RCC->AHB1ENR |= DMA2EN` (bit 22) — DMA2 saatini aç.
2. `DMA2_Stream3->CR &= ~DMA_SxCR_EN` — stream'i durdur, `EN` donanım
   tarafından 0'a düşene kadar bekle.
3. `DMA2->LIFCR/HIFCR` üzerinden ilgili stream'in tüm bayraklarını
   temizle (önceki bir transferden kalmış olabilir).
4. `CR`'yi kur: `CHSEL=3` (SPI1), `DIR=01` (bellek→periferik),
   `MINC=1` (bellek adresi her transferde artsın), `PINC=0` (periferik
   adresi hep `DR`, artmasın), `PSIZE=MSIZE=00` (byte, 8-bit DFF ile
   uyumlu), `CIRC=0` (tek seferlik) ya da `1` (sürekli örnekleme).
5. `PAR = (uint32_t)&SPIx->DR`, `M0AR = (uint32_t)pTxBuffer`,
   `NDTR = length`.
6. İsterseniz `TCIE`/`TEIE` set edip NVIC'te ilgili `DMA2_StreamX_IRQn`'i
   açın (transfer bitince kesme almak için — tıpkı `SPI_IRQHandling`'in
   DMA karşılığı olacak bir `DMA_IRQHandling` gerekecek).
7. `SPIx->CR2 |= SPI_CR2_TXDMAEN`.
8. `DMA2_Stream3->CR |= DMA_SxCR_EN` — stream'i başlat; SPI artık her
   `TXE` oluştuğunda DMA'dan otomatik veri isteyecek.

**Tamamlanma ölçütü DMA'nın `TCIF` bayrağıdır, SPI'nin `BSY` bayrağı
değil** — DMA "bellekten SPI'nin `DR`'sine son byte'ı da yazdım" dediğinde
tamamlanmış sayılır, ama SPI'nin shift register'ı o son byte'ı **hâlâ
telden kaydırıyor olabilir**. CS'i kaldırmadan önce hem `TCIF`'i hem
SPI'nin kendi `SR.BSY` bayrağının düşmesini beklemek gerekiyor — tam
`SPI_TransmitReceive`'de yaptığımız `BSY` kontrolünün DMA'da da geçerli
kalması.

---

## 7. Circular Mod — Ne Zaman Kullanılır

`CIRC=1` yapıldığında, `NDTR` sıfıra indiğinde donanım kendini otomatik
olarak başlangıç değerine resetleyip transferi **tekrar başlatır** —
yazılımın hiç araya girmesine gerek kalmadan. Bu proje bağlamında iki
gerçek kullanım senaryosu var:

- **Sürekli IMU örnekleme:** Sabit periyotla (örn. bir timer tetiklemesiyle)
  sensörden aynı register aralığını okuyup dairesel bir tampona yazmak.
- **Blackbox log akışı:** UART/SPI flash'a sürekli veri yazımı.

Normal (tek seferlik, `CIRC=0`) mod ise bizim şu anki `main.c`
örneklerimizdeki gibi "bir kerelik transfer, bitince dur" senaryosu için.

---

## 8. Bu Sürücüde Planlanan API (taslak, henüz yazılmadı)

`spi.c`'deki `SPI_SendIT`/`SPI_ReceiveIT` ile aynı imza deseni izlenerek:

```c
Status_t SPI_SendDMA(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t length);
Status_t SPI_ReceiveDMA(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t length);
void     DMA_IRQHandling(DMA_Stream_RegDef_t *pDMAStream, ...);
```

Bunun altında, `nvic.c`'ye benzer şekilde periferikten bağımsız bir
`dma.c`/`dma.h` katmanı (stream enable/disable, flag temizleme, config
yazma gibi donanım-seviyesi işler) ve onu çağıran SPI-özel fonksiyonlar
düşünülüyor — tıpkı `SPI_IRQInterruptConfig`'in `NVIC_IRQInterruptConfig`'e
yönlenmesi gibi.

---

## 9. Bilinen Zorluklar / Dikkat Edilmesi Gerekenler

- **Stream/kanal eşlemesi donanımsal sabit, hatası sessiz.** §3'teki
  tabloyu yanlış uygularsanız (örn. SPI1 için DMA1 kullanmak) hiçbir
  hata mesajı almazsınız, sadece transfer hiç gerçekleşmez.
- **DMA tamamlanması ≠ SPI transferinin fiziksel olarak bitmesi.** §6'nın
  sonundaki `BSY` notu — CS'i erken kaldırmak son byte'ı keser.
- **Full-duplex DMA iki stream ister, ayrı ayrı başlatılmalı** ve ikisi
  de aynı `SPIx`'e bağlı olmalı; sadece TX stream'i açıp RX'i unutmak,
  gönderirken MISO'dan geleni sessizce kaybetmenize yol açar (RXNE hiç
  okunmadığı için — tıpkı daha önce konuştuğumuz `SPI_Send`'in full-duplex
  modda RX tarafını hiç okumaması gibi bir risk, DMA'da da mevcut).
- **Bu MCU'da veri önbelleği (D-Cache) yok** (STM32F407, Cortex-M4 —
  F7/H7 ailesinin aksine) — yani DMA/CPU arasında cache-coherency
  sorunları burada söz konusu değil, ekstra `SCB_CleanDCache` gibi
  çağrılara gerek yok. İleride bir F7/H7 portu düşünülürse bu
  varsayım geçerliliğini yitirir.
- **`RCC->AHB1ENR` içinde `DMA1EN`/`DMA2EN` makroları henüz yok** —
  `GPIOx_PCLK_EN()`'lerin yanına eklenmesi gerekecek ilk adımlardan biri.

---

## 10. Sonraki Adım

Rapor burada bitiyor — kodlama aşamasına iki olası yoldan
başlayabiliriz, hangisini tercih ettiğinizi söylerseniz ona göre ilerleyelim.
