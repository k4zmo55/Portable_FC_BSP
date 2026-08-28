# RCC (Reset and Clock Control) — Detaylı Dokümantasyon

Bu doküman FC_BSP projesindeki `Inc/Drivers/rcc.h` ve `Src/rcc.c`
dosyalarını temel alır. Amaç, STM32F407'de saat ağacının (clock tree)
nasıl çalıştığını donanım seviyesinden anlamak ve bu projenin RCC
sürücüsünün her satırının *neden* öyle yazıldığını görmek.

Referans kaynak: RM0090 (STM32F405/415, STM32F407/417, STM32F427/437,
STM32F429/439 Reference Manual), "Reset and clock control (RCC)" bölümü.

---

## 1. RCC nedir, neden var?

Bir mikrodenetleyicide hiçbir çevre birimi (GPIO, I2C, SPI, Timer, USART...)
kendiliğinden çalışmaz. Her birimin bir **saat sinyaline (clock)** ihtiyacı
vardır; sinyal yoksa register'a yazsan bile o periferik "dinlemez".
STM32F4'te bu dağıtım şemasını yöneten tek periferik **RCC**'dir:

- Hangi osilatörün (dahili/harici) açık olduğunu kontrol eder,
- PLL çarpanlarını ayarlayıp yeni bir frekans üretir,
- O frekansı hangi hızda hangi veri yoluna (AHB, APB1, APB2) dağıtacağını
  belirler,
- Her periferiğe giden saat hattını tek tek açar/kapatır (`xxxENR`
  registerleri) — bu proje bağlamında `GPIOD_PCLK_EN()` gibi makrolar bu
  işi yapıyor (bkz. `Inc/Device/stm32f4xx.h:311-359`).

Reset sonrası STM32F407, **HSI (16 MHz dahili RC osilatör)** ile ve tüm
periferik saatleri kapalı halde açılır. `RCC_Init()` çağrılmadan önce
sistem 16 MHz'de, PLL kapalı, çalışır durumdadır — ama bu proje 168 MHz
hedeflediği için `main.c` ilk iş olarak `RCC_Init()` çağırır.

---

## 2. STM32F407 Saat Ağacı (Clock Tree)

```
                         ┌──────────────┐
   HSI (16 MHz, dahili) ─┤              │
                         │  PLLSRC MUX  ├──► PLL Girişi (PLL_IN)
   HSE (harici kristal) ─┤ (PLLCFGR.22) │
   (bu projede 8 MHz)    └──────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   /M  →  VCO girişi    │  1-2 MHz aralığında olmalı
                    │  (PLL_M: 2-63)         │
                    └───────────────────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   x N  →  VCO çıkışı   │  100-432 MHz aralığında olmalı
                    │  (PLL_N: 50-432)       │
                    └───────────────────────┘
                          │              │
                          ▼              ▼
                  ┌──────────────┐  ┌──────────────┐
                  │  /P → SYSCLK │  │ /Q → 48 MHz   │
                  │ (2/4/6/8)    │  │ USB/SDIO/RNG  │
                  └──────────────┘  └──────────────┘
                          │
                          ▼
        ┌─────────────────────────────────────┐
        │     SYSCLK kaynak seçimi (CFGR.SW)  │
        │        HSI  /  HSE  /  PLL          │
        └─────────────────────────────────────┘
                          │
                          ▼  SYSCLK (bu projede 168 MHz)
                ┌───────────────────┐
                │  AHB Prescaler     │  (CFGR.HPRE)
                │  /1 /2 /4 ... /512 │
                └───────────────────┘
                          │
                          ▼  HCLK  (CPU, DMA, bellek — bu projede 168 MHz)
              ┌───────────┴───────────┐
              ▼                       ▼
     ┌─────────────────┐    ┌──────────────────┐
     │ APB1 Prescaler  │    │ APB2 Prescaler   │
     │ (CFGR.PPRE1)    │    │ (CFGR.PPRE2)     │
     │ max 42 MHz      │    │ max 84 MHz       │
     └─────────────────┘    └──────────────────┘
              │                       │
              ▼                       ▼
     PCLK1 (TIM2-7, I2C,      PCLK2 (TIM1/8, USART1/6,
     USART2/3, SPI2/3...)     SPI1, ADC, SYSCFG...)
```

**Kritik kısıtlar (RM0090):**
- VCO girişi (`PLL_IN / M`) **1-2 MHz** aralığında olmalı (jitter'ı
  minimumda tutmak için ideali 2 MHz).
- VCO çıkışı (`VCO_in x N`) **100-432 MHz** aralığında olmalı.
- APB1 (düşük hızlı bus) **42 MHz**'i geçemez.
- APB2 (yüksek hızlı bus) **84 MHz**'i geçemez.
- HCLK (AHB / CPU hızı) STM32F407'de **168 MHz**'i geçemez (F42x/43x'te
  overdrive ile 180 MHz mümkün, bu kart F407 olduğu için geçerli değil).

---

## 3. İlgili Register'lar (bu projede kullanılanlar)

`Inc/Device/stm32f4xx.h:170-202` içindeki `RCC_TypeDef_t` yapısı RCC'nin
bellek haritasını tanımlar; `#define RCC ((RCC_TypeDef_t*)RCC_BASE_ADDR)`
(satır 278) ile `RCC->CR`, `RCC->CFGR` gibi erişim sağlanır.
`RCC_BASE_ADDR = AHB1_BASE_ADDR + 0x3800`.

Sürücünün dokunduğu registerler:

| Register | Offset | Bu projede kullanılan bitler |
|---|---|---|
| `RCC->CR` | 0x00 | `HSION`(0), `HSIRDY`(1), `HSEON`(16), `HSERDY`(17), `PLLON`(24), `PLLRDY`(25) |
| `RCC->PLLCFGR` | 0x04 | `PLLM`[5:0], `PLLN`[14:6], `PLLP`[17:16], `PLLSRC`(22), `PLLQ`[27:24] |
| `RCC->CFGR` | 0x08 | `SW`[1:0], `SWS`[3:2], `HPRE`[7:4], `PPRE1`[12:10], `PPRE2`[15:13] |
| `FLASH->ACR` | 0x00 (FLASH bloğu) | `LATENCY`[3:0], `PRFTEN`(8), `ICEN`(9), `DCEN`(10) |

Sürücünün **dokunmadığı** ama var olan registerler: `CIR` (kesme),
`AHBxRSTR`/`APBxRSTR` (periferik reset), `BDCR`/`CSR` (LSE/LSI, RTC/backup
domain), `SSCGR` (spread spectrum), `PLLI2SCFGR` (I2S PLL'i). Bunlar
"Bilinen sınırlar" bölümünde tekrar geçiyor.

---

## 4. `rcc.h` — Arayüz ve Veri Tipleri

### 4.1 Neden register değeriyle bire bir eşleşen enum'lar?

```c
typedef enum {
    RCC_OSC_HSI = 0,
    RCC_OSC_HSE = 1
} RCC_OscSource_t;
```

`RCC_OSC_HSE` değeri tam olarak `PLLCFGR.PLLSRC` bitinin değeriyle
aynıdır (`rcc.c:85`: `(pRCCConfig->OscSource << 22)`). Yani enum, sadece
okunabilirlik için bir "isim" değil — doğrudan register'a yazılabilecek
bir sayısal değer. Bu yaklaşımın bedeli: enum sırası **asla** rastgele
değiştirilemez, çünkü donanım semantiğiyle kilitli.

Aynı mantık `RCC_SysClkSource_t` (CFGR.SW/SWS ile eşleşir),
`RCC_AHBPrescaler_t` ve `RCC_APBPrescaler_t` için de geçerli.

### 4.2 AHB Prescaler neden doğrusal değil?

```c
typedef enum {
    RCC_AHB_DIV_1   = 0x0,
    RCC_AHB_DIV_2   = 0x8,
    RCC_AHB_DIV_4   = 0x9,
    ...
    RCC_AHB_DIV_512 = 0xF
} RCC_AHBPrescaler_t;
```

`HPRE` alanı 4 bit (0-15) ama 16 farklı bölen değeri gerekiyor
(`/1, /2, /4, /8, /16, /64, /128, /256, /512` — yani `/32` **atlanıyor**).
STM32 tasarımcıları `/32`'yi kasıtlı çıkarmışlar; bit deseni şu şekilde
kodlanmış: en üst bit (bit 3) set değilse bölen 1, set ise alt 3 bit
`(değer - 8)` kadar 2'nin kuvveti. Bu yüzden `0x0..0x7` hepsi `/1`'e denk
gelir (register'da 0-7 arası herhangi bir değer `/1` anlamına gelir, RM0090
bunu "not divided" olarak tanımlar), `0x8`'den itibaren gerçek bölme
başlar. `rcc.c`'deki `AHBPrescShiftTable[16]` bu deseni bir çevrim
tablosuna dönüştürüyor (bkz. §6.5).

### 4.3 `RCC_PLLConfig_t` ve `RCC_Config_t`

```c
typedef struct {
    uint32_t PLL_M; /* 2-63 */
    uint32_t PLL_N; /* 50-432 */
    uint32_t PLL_P; /* 2/4/6/8 */
    uint32_t PLL_Q; /* 2-15 */
} RCC_PLLConfig_t;

typedef struct {
    RCC_OscSource_t     OscSource;
    uint32_t            OscFreqHz;
    uint8_t              UsePLL;
    RCC_PLLConfig_t      PLL;
    RCC_AHBPrescaler_t   AHBPrescaler;
    RCC_APBPrescaler_t   APB1Prescaler;
    RCC_APBPrescaler_t   APB2Prescaler;
    uint8_t              FlashLatency;
} RCC_Config_t;
```

Tasarım kararı: bu struct'ı dolduran kod (`main.c`) **hiçbir register
adresi bilmek zorunda değil**. Sadece "hangi kaynak, hangi çarpan, hangi
bölen" sorusuna cevap veriyor; register'a yazma mantığı tamamen
`RCC_Init()` içine hapsedilmiş. `READ.md`'de belirtildiği gibi bu, aynı
struct'ın başka bir Cortex-M ailesi için de (kendi `rcc.c`'siyle)
yeniden kullanılabilir olmasını hedefliyor.

`FlashLatency` alanının struct içinde olması bilinçli: doğru wait-state
sayısı hem hedef SYSCLK'a hem de çekirdek voltajına (Vcore) bağlı bir
tablo değeri (§5), otomatik hesaplanmıyor — çağıran kodun doğru değeri
vermesi bekleniyor. Bu potansiyel bir hata kaynağı; §8'de tekrar ele
alınıyor.

### 4.4 `IS_RCC_*` makroları — çalışma zamanı doğrulama

```c
#define IS_RCC_PLL_M(M)  (((M) >= 2U)  && ((M) <= 63U))
#define IS_RCC_PLL_N(N)  (((N) >= 50U) && ((N) <= 432U))
#define IS_RCC_PLL_P(P)  (((P) == 2U) || ((P) == 4U) || ((P) == 6U) || ((P) == 8U))
#define IS_RCC_PLL_Q(Q)  (((Q) >= 2U) && ((Q) <= 15U))
```

Bunlar RM0090'daki PLL kısıt tablosunun doğrudan kod karşılığı. `RCC_Init`
bu makroları girişte kontrol eder (`rcc.c:36-52`) ve geçersiz bir
konfigürasyonu **register'a hiç yazmadan** `RCC_ERROR` ile reddeder.
Dikkat: bu makrolar sadece *aralık* kontrolü yapar; VCO giriş/çıkış
aralığı (1-2 MHz / 100-432 MHz) gibi M/N/OscFreqHz'in **birlikte**
oluşturduğu kısıtları kontrol etmez (bkz. §8).

---

## 5. Flash Gecikmesi (Wait States) — neden ve nasıl

Flash bellek, CPU'dan çok daha yavaştır. SYSCLK arttıkça flash okuma
işlemi CPU çevrimine yetişemez; bu yüzden STM32, her okumaya kaç
"bekleme çevrimi" (wait state) ekleneceğini `FLASH->ACR.LATENCY`
alanından öğrenir. Yanlış (düşük) bir değer verilirse çip hatalı kod
çalıştırır ya da kilitlenir.

RM0090 Tablo (Vcore = 2.7-3.6V aralığı, bu kartın tipik çalışma voltajı):

| HCLK (AHB hızı) | Gerekli Latency |
|---|---|
| 0 - 30 MHz | 0 WS |
| 30 - 60 MHz | 1 WS |
| 60 - 90 MHz | 2 WS |
| 90 - 120 MHz | 3 WS |
| 120 - 150 MHz | 4 WS |
| 150 - 168 MHz | 5 WS |

`main.c`'deki örnek 168 MHz hedeflediği için `.FlashLatency = 5` doğru
değer. `rcc.c:74-77`:

```c
FLASH->ACR = (pRCCConfig->FlashLatency << 0)  /* LATENCY */
           | (1 << 8)                         /* PRFTEN: prefetch */
           | (1 << 9)                         /* ICEN: instruction cache */
           | (1 << 10);                       /* DCEN: data cache */
```

`PRFTEN`/`ICEN`/`DCEN` (prefetch + I-cache + D-cache) flash gecikmesinin
CPU performansına etkisini telafi eder — yüksek SYSCLK'ta bunlar
kapalıysa performans kaybı büyük olur.

**Sıralama kritik:** bu ayar `RCC_Init()` içinde PLL/HSE açılmadan HEMEN
ÖNCE (adım 2, `rcc.c:73`) yapılıyor — yorum satırında da vurgulanmış:
*"Flash gecikmesi - saat hizlanmadan ONCE ayarlanmali"*. Eğer önce saat
hızlandırılıp sonra latency ayarlansaydı, CPU flash'tan zaten yetişemediği
bir hızda okuma yapmaya çalışırdı (fetch hatası / instruction corruption
riski).

---

## 6. `rcc.c` — Fonksiyon Fonksiyon

### 6.1 `WaitFlag()` — polling ile bayrak bekleme

```c
static RCC_Status_t WaitFlag(volatile uint32_t *pReg, uint32_t mask)
{
    uint32_t timeout = 0;
    while(!(*pReg & mask))
    {
        if(++timeout > CLOCK_TIMEOUT)
        {
            return RCC_TIMEOUT;
        }
    }
    return RCC_OK;
}
```

STM32'de bir osilatörü (`HSEON`) ya da PLL'i (`PLLON`) açmak **anlık**
değildir — kristal osilasyona oturmalı ya da PLL kilitlenmelidir. Donanım
bunu `HSERDY`/`PLLRDY` bitleriyle bildirir. Bu fonksiyon o biti
`CLOCK_TIMEOUT` (500000) döngü boyunca bekler; süre dolarsa `RCC_TIMEOUT`
döner. Bu bir **busy-wait / polling** deseni — kesme kullanılmıyor,
basit ve bare-metal bir başlangıç kodu için yeterli.

`main.c:55-70`'te bu timeout'un nasıl ele alındığı görülüyor: HSE takılı
değilse ya da PLL kilitlenmezse `RCC_Init` hata döner, program PD12
LED'ini hızlı yanıp söndürerek (`ErrorBlink`) sonsuz döngüye girer —
"donanım sessizce yanlış hızda çalışmaya devam etmesin" prensibi.

### 6.2 `RCC_Init()` — adım adım

**Adım 0 — Girdi doğrulama (`rcc.c:36-52`):** `IS_RCC_*` makrolarıyla
NULL pointer, geçersiz enum, geçersiz PLL parametresi kontrolü. Herhangi
biri geçersizse hiçbir register'a dokunmadan `RCC_ERROR` döner.

**Adım 1 — Osilatörü aç (`rcc.c:54-71`):**
```c
if(pRCCConfig->OscSource == RCC_OSC_HSE)
{
    s_HSEFreqHz = pRCCConfig->OscFreqHz;
    RCC->CR |= (1 << 16); /* HSEON */
    status = WaitFlag(&RCC->CR, (1 << 17)); /* HSERDY */
}
else
{
    RCC->CR |= (1 << 0); /* HSION */
    status = WaitFlag(&RCC->CR, (1 << 1)); /* HSIRDY */
}
```
`s_HSEFreqHz` burada saklanıyor çünkü **register'lar frekans değeri
tutmaz**, sadece "HSE kullanılıyor" bilgisini tutar. Kristalin gerçek
frekansı (8 MHz, 25 MHz, ne ise) yazılımın bir yerde bilmesi/saklaması
gerekir — `RCC_GetSystemClock()` bu değeri geri hesaplamak için kullanır
(§6.3).

Not: HSI zaten reset sonrası açık gelir (STM32F407'nin varsayılan
durumu); yine de kod bunu garanti altına almak için tekrar `HSION` set
ediyor — zararsız ve daha güvenli (hangi durumdan geldiği bilinmeyen bir
sistemde varsayım yapmamak için iyi bir pratik).

**Adım 2 — Flash latency:** §5'te anlatıldı, saat hızlandırılmadan önce.

**Adım 3-4 — PLL kurulumu (`rcc.c:79-95`, sadece `UsePLL=1` ise):**
```c
RCC->PLLCFGR = (pRCCConfig->PLL.PLL_M << 0)
             | (pRCCConfig->PLL.PLL_N << 6)
             | (((pRCCConfig->PLL.PLL_P / 2) - 1) << 16)
             | (pRCCConfig->OscSource << 22)
             | (pRCCConfig->PLL.PLL_Q << 24);

RCC->CR |= (1 << 24); /* PLLON */
status = WaitFlag(&RCC->CR, (1 << 25)); /* PLLRDY */
```
`PLLP` alanı özel: register 2 bit ve `{00,01,10,11}` → `{2,4,6,8}`
eşlemesi kullanır, yani `değer = (P/2) - 1`. Kod bunu
`((PLL_P / 2) - 1) << 16` ile hesaplıyor — struct'ta insan-okur (2/4/6/8),
register'da makine-okur (0/1/2/3) formatına çeviriyor. Bu, struct'ın
"register semantiğiyle birebir" olduğu iddiasının tek istisnası ve kasıtlı
(header'daki yorum bunu "SYSCLK bolen (SYSCLK = VCO_out / PLL_P)" olarak
açıklıyor, kullanıcıya gerçek bölen değerini veriyor).

`RCC->PLLCFGR = (...)` (`|=` değil `=`) — register tamamen üzerine
yazılıyor. Bu, PLL zaten açıkken (`PLLON=1` iken) `PLLCFGR`'a yazmanın
donanımda **tanımsız/izin verilmeyen** bir işlem olduğu gerçeğiyle örtüşen
bir varsayıma dayanıyor: `RCC_Init()`'in yalnızca soğuk başlangıçta (PLL
henüz off) çağrılacağı varsayılıyor. Çalışma zamanında saat ağacını
"yeniden" kurmak için bu fonksiyon güvenle tekrar çağrılamaz (bkz. §8).

**Adım 5 — Bus prescaler'ları (`rcc.c:97-103`):**
```c
RCC->CFGR &= ~(0xF << 4);   RCC->CFGR |= (pRCCConfig->AHBPrescaler  << 4);
RCC->CFGR &= ~(0x7 << 10);  RCC->CFGR |= (pRCCConfig->APB1Prescaler << 10);
RCC->CFGR &= ~(0x7 << 13);  RCC->CFGR |= (pRCCConfig->APB2Prescaler << 13);
```
Burada `RCC->PLLCFGR`'ın aksine **read-modify-write** (önce ilgili bit
alanını temizle, sonra yeni değeri OR'la) kullanılıyor — çünkü `CFGR`
zaten SYSCLK kaynağı gibi başka canlı bitler taşıyor, tamamen üzerine
yazmak onları da sıfırlardı.

**Adım 6 — SYSCLK kaynağını seç ve doğrula (`rcc.c:105-119`):**
```c
targetSrc = pRCCConfig->UsePLL ? RCC_SYSCLK_SRC_PLL :
            (pRCCConfig->OscSource == RCC_OSC_HSE ? RCC_SYSCLK_SRC_HSE : RCC_SYSCLK_SRC_HSI);

RCC->CFGR &= ~(0x3 << 0);
RCC->CFGR |=  (targetSrc << 0);   /* SW alanı: "lütfen bu kaynağa geç" */

while(((RCC->CFGR >> 2) & 0x3) != (uint32_t)targetSrc)  /* SWS alanı: donanımın onayı */
{ if(++timeout > CLOCK_TIMEOUT) return RCC_TIMEOUT; }
```
`SW` ("switch": yazılımın isteği) ile `SWS` ("switch status": donanımın
gerçekte hangi kaynakta olduğu) **iki ayrı alan**. SW'ye yazmak anlık
geçiş garantisi vermez — birkaç saat çevrimi sürebilir. Kod bu yüzden
`SWS`'yi ayrıca polling ile bekliyor; bu satır olmasaydı, fonksiyon
`RCC_OK` dönebilir ama SYSCLK henüz eski kaynakta olabilirdi.

### 6.3 `RCC_GetSystemClock()` — frekansı register'lardan geri hesaplama

```c
uint8_t clkSrc = (RCC->CFGR >> 2) & 0x3;   /* SWS: gercekte hangi kaynak aktif */

if(clkSrc == RCC_SYSCLK_SRC_HSI)      sysclk = HSI_VALUE_HZ;         /* sabit 16 MHz */
else if(clkSrc == RCC_SYSCLK_SRC_HSE) sysclk = s_HSEFreqHz;          /* Adım 1'de saklanan deger */
else /* PLL */
{
    uint32_t pllSrcIsHSE = (RCC->PLLCFGR >> 22) & 0x1;
    uint32_t pllM = (RCC->PLLCFGR >> 0)  & 0x3F;
    uint32_t pllN = (RCC->PLLCFGR >> 6)  & 0x1FF;
    uint32_t pllP = (((RCC->PLLCFGR >> 16) & 0x3) + 1) * 2;   /* register→gercek deger, tersi */
    uint32_t pllInputHz = pllSrcIsHSE ? s_HSEFreqHz : HSI_VALUE_HZ;

    sysclk = ((pllInputHz / pllM) * pllN) / pllP;
}
```

Bu fonksiyon, `RCC_Init()`'te kullanılan struct'ı **hatırlamaz** — o
struct sadece bir kerelik konfigürasyon girdisiydi ve yığın/stack'te
kaybolmuş olabilir. Bunun yerine gerçek register içeriğini okuyup
frekansı yeniden türetir. Bu, `RCC_GetPCLK1Value()` /
`RCC_GetPCLK2Value()` gibi diğer fonksiyonların (ve ileride yazılacak
UART baud rate hesaplayıcısı, SysTick kurulumu gibi kodların) her zaman
**donanımın o anki gerçek durumunu** görmesini sağlar — global bir "şu an
kaçtı" değişkenine güvenmez.

Dikkat: `pllM / pllN` sırasıyla değil, **önce bölme sonra çarpma**
(`(pllInputHz / pllM) * pllN`) yapılıyor. Bu, 32-bit tamsayı taşmasını
önlemek için bilinçli bir sıralama: `pllInputHz * pllN` (örn. 8MHz x 336
= 2.688 GHz) `uint32_t` sınırına (~4.29 GHz) yakın/taşabilir senaryolar
için risklidir; `pllInputHz / pllM` önce yapılırsa (8MHz/8=1MHz), sonra
`x336` işlemi güvenli aralıkta kalır. Bedeli: çok küçük bir hassasiyet
kaybı (M ile tam bölünmeyen frekanslarda), ama bu projede M değerleri
VCO girişini tam sayıya getirecek şekilde seçildiği için pratikte kayıpsız.

### 6.4 `RCC_GetPCLK1Value()` / `RCC_GetPCLK2Value()`

```c
uint32_t sysclk   = RCC_GetSystemClock();
uint8_t  ahbShift  = AHBPrescShiftTable[(RCC->CFGR >> 4)  & 0xF];
uint8_t  apb1Shift = APBPrescShiftTable[(RCC->CFGR >> 10) & 0x7];
return (sysclk >> ahbShift) >> apb1Shift;
```

`SYSCLK → HCLK (AHB) → PCLK1/PCLK2` zincirini takip ediyor. Bölme,
`/` yerine `>>` (shift) ile yapılıyor çünkü tüm prescaler değerleri
2'nin kuvvetleridir — shift, bölmeden daha ucuz bir işlemdir (bu bare-metal,
performansa duyarlı bir ortam için anlamlı bir mikro-optimizasyon).

Bu iki fonksiyonun asıl kullanım amacı ileride yazılacak periferik
sürücüleridir: örn. bir USART baud rate hesaplayıcısı `RCC_GetPCLK2Value()`
çağırıp `USART1` için doğru `BRR` değerini bulacak, ya da I2C sürücüsü
`RCC_GetPCLK1Value()`'i `CCR`/`TRISE` hesabında kullanacak
(`stm32f4xx.h`'deki `I2Cx_RegDef_t` yorumlarında bu bağlantı zaten işaret
ediliyor: *"CCR: SCL periyodu"*, *"TRISE: (APB1_MHz + 1)"*).

### 6.5 Prescaler çevrim tabloları

```c
static const uint8_t AHBPrescShiftTable[16] = {0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9};
static const uint8_t APBPrescShiftTable[8]  = {0,0,0,0,1,2,3,4};
```

`AHBPrescShiftTable`, `HPRE` alanının 16 olası değerini (§4.2'de anlatılan
"/32 atlanıyor" deseniyle) doğrudan bir **shift miktarına** çeviriyor:
indeks 0-7 → shift 0 (bölme yok), indeks 8 → shift 1 (`/2`), indeks 9 →
shift 2 (`/4`) ... indeks 15 → shift 9 (`/512`). Bu tablo aslında CMSIS'in
`system_stm32f4xx.c` dosyasındaki standart `AHBPrescTable`'ın karşılığı
(kod içi yorumda da bu kaynağa atıf var, `rcc.c:9`).

`APBPrescShiftTable` daha basit: `PPREx` alanının en üst biti (bit 2,
yani indeks ≥4) set değilse bölme yok (indeks 0-3 → shift 0), set ise
`indeks - 3` kadar shift (indeks 4 → `/2`, ..., indeks 7 → `/16`).

---

## 7. Uçtan Uca Örnek: `main.c`'deki 168 MHz Konfigürasyonu

```c
RCC_Config_t ClockConfig = {
    .OscSource     = RCC_OSC_HSE,
    .OscFreqHz     = 8000000UL,
    .UsePLL        = 1,
    .PLL           = { .PLL_M = 8, .PLL_N = 336, .PLL_P = 2, .PLL_Q = 7 },
    .AHBPrescaler  = RCC_AHB_DIV_1,
    .APB1Prescaler = RCC_APB_DIV_4,
    .APB2Prescaler = RCC_APB_DIV_2,
    .FlashLatency  = 5
};
```

Bu, klasik "STM32F407 Discovery / genel amaçlı F407 kartı, 8 MHz harici
kristal, maksimum performans" konfigürasyonudur. Hesap:

| Adım | Formül | Sonuç |
|---|---|---|
| VCO girişi | `HSE / PLL_M` = `8 MHz / 8` | **1 MHz** (1-2 MHz aralığında ✓) |
| VCO çıkışı | `VCO_in x PLL_N` = `1 MHz x 336` | **336 MHz** (100-432 MHz aralığında ✓) |
| SYSCLK | `VCO_out / PLL_P` = `336 / 2` | **168 MHz** (F407 maksimumu ✓) |
| USB/SDIO/RNG saati | `VCO_out / PLL_Q` = `336 / 7` | **48 MHz** (USB OTG FS'in gerektirdiği tam değer ✓) |
| HCLK (AHB) | `SYSCLK / 1` | **168 MHz** |
| PCLK1 (APB1) | `HCLK / 4` | **42 MHz** (limit tam sınırında ✓) |
| PCLK2 (APB2) | `HCLK / 2` | **84 MHz** (limit tam sınırında ✓) |
| Flash latency | 168 MHz, Vcore 2.7-3.6V → tablo | **5 WS** ✓ |

Bu değerler kesinlikle rastgele değil: `PLL_Q = 7` özellikle 48 MHz tam
sayı çıkması için seçilmiş (USB OTG FS 48 MHz ister, yaklaşık değer kabul
etmez). `APB1Prescaler = /4` ve `APB2Prescaler = /2` seçimleri, her ikisinin
de kendi limitine (42 MHz, 84 MHz) mümkün olduğunca yakın — yani mümkün
olan en yüksek periferik hızını — hedefliyor.

`main.c` akışı:
1. `GPIO_PeriClockControl(GPIOD, ENABLE)` — LED portunun saatini RCC
   üzerinden aç (`GPIOD_PCLK_EN()` → `RCC->AHB1ENR |= (1<<3)`).
2. `RCC_Init(&ClockConfig)` — yukarıdaki saat ağacını kur.
3. Başarısızsa (`RCC_TIMEOUT`/`RCC_ERROR`) → `ErrorBlink()`: PD12'yi hızlı
   yanıp söndürerek donanımsal bir "saat kurulamadı" sinyali ver ve
   sonsuz döngüde kal — bozuk bir saat hızıyla devam etmenin (yanlış
   UART baud rate, yanlış timer periyodu, potansiyel olarak "kart düşer"
   senaryosu — bkz. `READ.md` §3.2) riskinden kaçınmak için bilinçli bir
   fail-safe.
4. Başarılıysa 3 LED'i (PD12/13/14) başlatıp sonsuz döngüde toggle eder.

---

## 8. Bilinen Sınırlar / Dikkat Edilmesi Gerekenler

Bu sürücü bilinçli olarak minimal tutulmuş bir "temel saat ağacı kurucusu".
Aşağıdakiler eksik ya da kırılgan noktalar — genişletirken göz önünde
bulundurulmalı:

- **VCO giriş/çıkış aralığı kontrol edilmiyor.** `IS_RCC_PLL_M/N` sadece
  M ve N'nin *tek başına* izinli aralıkta olduğunu kontrol ediyor
  (M: 2-63, N: 50-432); ama `OscFreqHz / M` gerçekten 1-2 MHz'de mi, ya
  da `VCO_in x N` gerçekten 100-432 MHz'de mi — bunlar kontrol edilmiyor.
  Yanlış bir `board_config` (örn. 25 MHz HSE ile M=2) VCO'yu spesifikasyon
  dışına çıkarabilir; PLL yine de kilitlenip `PLLRDY` set edebilir ama
  jitter/kararsızlık riski oluşur.
- **HSE bypass modu yok.** Bazı kartlarda kristal yerine harici osilatör
  (aktif clock source) bağlanır; bu durumda `CR.HSEBYP` bitinin de
  set edilmesi gerekir. Bu sürücü sadece pasif kristal varsayıyor.
- **Clock Security System (CSS) kullanılmıyor.** HSE çalışırken aniden
  giderse (kristal arızası), donanım otomatik olarak HSI'ye düşebilir
  ve bir NMI üretebilir — ama bu, `CR.CSSON` set edilmediği sürece aktif
  değil. Uçuş kontrol kartı bağlamında (`READ.md`) bu potansiyel olarak
  önemli bir güvenlik açığı: HSE giderse SYSCLK sessizce donar/bozulur.
- **`RCC_Init()` yeniden çağrıya güvenli değil.** `RCC->PLLCFGR = (...)`
  (adım 3) PLL zaten kilitliyken yazılırsa RM0090'a göre tanımsız
  davranış riski taşır. Çalışma zamanında saat hızını değiştirmek
  isteyen bir kod önce `PLLON`'u kapatıp `PLLRDY`'nin düşmesini
  beklemeli — bu sürücü bunu yapmıyor, yani sadece **soğuk başlangıçta,
  bir kez** çağrılmak üzere tasarlanmış.
- **LSE/LSI/RTC domain'i kapsam dışı.** `BDCR`/`CSR` registerleri hiç
  kullanılmıyor; RTC veya düşük güç saat kaynağı gerekiyorsa ayrı bir
  genişletme gerekir.
- **MCO1/MCO2 çıkışları yok.** Saat sinyalini bir pine çıkarıp osiloskopla
  doğrulama (donanım bring-up sırasında çok kullanılan bir teknik)
  şu an desteklenmiyor.
- **`FlashLatency` elle veriliyor, hesaplanmıyor.** §5'teki tabloyu
  `SYSCLK`'a göre otomatik seçen bir yardımcı fonksiyon yok; yanlış
  (düşük) bir değer girilirse derleme hatası da vermez, kart doğrudan
  kilitlenir/çöker. Bu, `READ.md`'nin "Saat ağacı kurulmuyor" notuyla
  aynı bilinen risk kategorisinde.

---

## 9. Nasıl Genişletilir

**Farklı bir kristal frekansı için (örn. 25 MHz HSE):** `main.c`'de
`OscFreqHz` ve PLL_M'i değiştirmek yeterli — örn. M=25 seçilirse VCO
girişi yine 1 MHz olur, N/P/Q aynı kalabilir. Register mantığına
dokunmaya gerek yok; bu tam olarak `RCC_Config_t`'nin var oluş amacı.

**Yeni bir SYSCLK hedefi için:** PLL_M/N/P kombinasyonunu RM0090'daki
kısıtlara göre yeniden hesapla (VCO giriş 1-2 MHz, VCO çıkış 100-432 MHz,
SYSCLK ≤168 MHz), `FlashLatency`'yi §5 tablosundan seç, APB1/APB2
prescaler'ları 42/84 MHz limitlerine göre ayarla.

**MCO çıkışı eklemek için:** `RCC_TypeDef_t`'te zaten `CFGR` register'ı
var (MCO1/MCO2 alanları `CFGR`'ın üst bitlerinde, bu structta henüz
enum/makro olarak modellenmemiş) — `rcc.h`'a yeni bir enum +
`RCC_Init()` ya da ayrı bir `RCC_MCOConfig()` fonksiyonu eklenebilir.

**Yeni bir MCU ailesi için (örn. STM32G4, `READ.md` §3.3'te bahsedilen
hedef):** Aynı `RCC_Config_t` arayüzü korunur; sadece `rcc.c` içindeki
register isimleri/bit pozisyonları değişir (G4'te I2C, GPIO saat hattı
`AHB2ENR` üzerinden gelir, PLL yapılandırması farklı bir register
düzenine sahiptir). Üst katman (`main.c`) tek satır değişmeden
taşınabilir olmalı — projenin genel taşınabilirlik hedefiyle birebir
örtüşen bir senaryo.
