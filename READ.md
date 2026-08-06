# FC_BSP — Taşınabilir STM32 Uçuş Kontrol Yazılım Altyapısı

Farklı STM32 işlemcileri ve farklı sensör setleri kullanan uçuş kontrol
kartlarının aynı yazılım tabanını paylaşabilmesi için tasarlanmış bir
Board Support Package (BSP) katmanı.

Amaç iki yönlü: kendi tasarladığım uçuş kontrol kartının yazılımını
geliştirmek, ve başka biri kendi kartını tasarladığında aynı kod tabanını
tek bir konfigürasyon dosyasıyla o karta uyarlayabilmesini sağlamak.

---

## 1. Problem

Bir uçuş kontrol kartı tasarlayan herkes aynı duvarla karşılaşıyor: donanım
hazır, ama yazılım sıfırdan. Mevcut seçenekler şunlar:

| Seçenek | Sorun |
|---|---|
| Betaflight / INAV | Hazır ama devasa; kendi kartını eklemek için binlerce satırlık target sistemine girmek gerekiyor, üstelik kod öğrenme amacıyla okunabilir değil |
| ArduPilot | Daha da büyük, HAL katmanı bir işletim sistemi kadar karmaşık |
| Sıfırdan yazmak | Her yeni kartta IMU sürücüsünden I2C sürücüsüne kadar her şey yeniden yazılıyor |

Aradaki boşluk şu: **RTOS'suz, okunabilir, tek header ile konfigüre
edilebilen, ama gerçekten taşınabilir bir altyapı.** FC_BSP bu boşluğu
hedefliyor.

Referans aldığım yapı, daha önce geliştirdiğim çok işlemcili Ethernet BSP
projesi. Orada da aynı fikir vardı: kullanıcı tek bir `config.h` düzenler,
altyapı hangi MCU'da hangi çevre birimini kullanacağını kendisi çözer.

---

## 2. Temel tasarım kuralı

Bağımlılık zinciri tek yönlüdür ve asla atlanmaz:

```
main.c
  → fc_sensors      (kart konfigürasyonunu cihazlara çevirir)
    → fc_imu / fc_baro   (sürücüden bağımsız ortak mantık)
      → fc_bus           (I2C mi SPI mi — sürücü bunu bilmez)
        → fc_port        (TAŞINABİLİRLİK SÖZLEŞMESİ)
          → register     (sadece burada donanıma dokunulur)
```

`fc_port.h` üstündeki hiçbir dosya bir peripheral register'ına dokunmaz.
Bu kuralın pratik sonucu şu: `fc_imu_mpu6050.c` dosyası tek satır
değişmeden hem F4'te hem G4'te, hem I2C hem SPI üzerinde çalışır.

---

## 3. Katmanlar

### 3.1 `board_config.h` — kullanıcının dokunduğu tek dosya

Yeni bir karta geçmek için düzenlenen tek yer. `Inc/boards/` altındaki
hazır kart tanımlarından birini seçer ya da kendi kopyanı yazarsın:

```c
#define FC_TARGET_MCU       FC_MCU_STM32G474
#define FC_SYSCLK_HZ        170000000u

#define FC_IMU_DRIVER       FC_IMU_ICM42688
#define FC_IMU_BUS_TYPE     FC_BUS_TYPE_SPI
#define FC_IMU_BUS_ID       FC_SPI_1
#define FC_IMU_CS           FC_PIN(A, 4, 0)
#define FC_IMU_ORIENT       FC_ORIENT_CW270_FLIP

#define FC_BARO_DRIVER      FC_BARO_SPL06
#define FC_BARO_BUS_TYPE    FC_BUS_TYPE_I2C
#define FC_BARO_BUS_ID      FC_I2C_1
#define FC_BARO_ADDR        0x76
```

Pinler tek bir 16 bit kelimeye paketlenir: `FC_PIN(port, numara, alternate
function)`. Bu sayede pin tanımları hem derleme zamanı sabiti hem de
fonksiyonlara parametre olarak geçirilebilir bir değer.

### 3.2 `fc_device.h` — derleme zamanı doğrulama

Kullanıcının seçimlerini bir MCU ailesine eşler ve **imkânsız kartları
derleme aşamasında yakalar**:

```c
#if (FC_IMU_DRIVER == FC_IMU_MPU6050) && (FC_IMU_BUS_TYPE == FC_BUS_TYPE_SPI)
#  error "MPU6050 is an I2C-only part. The SPI-capable sibling is the MPU6000."
#endif
```

Bu katman bilinçli bir güvenlik tercihi. Ethernet'te yanlış konfigürasyon =
paket gitmez; uçuş kontrolde yanlış konfigürasyon = kart düşer. Yakalanabilecek
her hatanın linker'a bile ulaşmaması gerekiyor.

### 3.3 `fc_port.h` — taşınabilirlik sözleşmesi

Her MCU ailesinin uygulamak zorunda olduğu arayüz. Şu an F4 ve G4 var:

```c
fc_status_t fc_port_init(void);
uint32_t    fc_port_micros(void);
fc_status_t fc_port_gpio_output(fc_pin_t pin, bool init_high);
fc_status_t fc_port_spi_init(fc_spi_id_t, fc_pin_t sck, fc_pin_t miso,
                             fc_pin_t mosi, uint32_t hz, uint8_t mode);
fc_status_t fc_port_spi_xfer(fc_spi_id_t, const uint8_t *tx,
                             uint8_t *rx, uint16_t len);
fc_status_t fc_port_i2c_read(fc_i2c_id_t, uint8_t addr, uint8_t reg,
                             uint8_t *buf, uint16_t len);
```

Aileler arasında gerçekten değişen şeyler:

| Konu | STM32F4 | STM32G4 |
|---|---|---|
| GPIO saat hattı | `RCC->AHB1ENR` | `RCC->AHB2ENR` |
| GPIO register düzeni | aynı | aynı → ortak dosyada |
| SPI veri boyutu | `CR1.DFF` | `CR2.DS` + `FRXTH` |
| I2C | eski çevre birimi: `FREQ`/`CCR`/`TRISE` | I2Cv2: tek `TIMINGR` kelimesi |
| Zaman tabanı | DWT_CYCCNT | DWT_CYCCNT → ortak dosyada |

Ortak olan kısımlar (`GPIO`, zaman tabanı) `fc_port_common.c` içinde tek
kez yazılır; sadece ailelere özgü kancalar (`fc_port_rcc_gpio_enable`)
aile dosyalarında bulunur.

Her aile dosyası tamamen `#ifdef FC_FAMILY_XX` ile sarılıdır. Böylece tüm
port dosyalarını build'e ekleyebilirsin, sadece doğru olan derlenir —
build sistemine koşullu dosya listesi yazma ihtiyacı ortadan kalkar.

### 3.4 `fc_bus.h` — projenin en kritik kararı

Aynı sensör bir kartta I2C, başka bir kartta SPI üzerinde olabilir.
Radiolink F405'te IMU SPI'da, SPL06 barometre I2C'de. Kendi kartımda ise
ikisi de I2C'de. Sürücüyü iki kez yazmamanın tek yolu, sürücünün bus
tipini hiç bilmemesi:

```c
struct fc_bus_s {
    const fc_bus_ops_t *ops;   /* I2C mi SPI mi — sadece burada belli */
    uint8_t  id;               /* hangi kontrolcü                     */
    uint8_t  addr;             /* I2C adresi                          */
    fc_pin_t cs;               /* SPI chip select                     */
};
```

Sürücü sadece `fc_bus_read(bus, reg, buf, len)` çağırır. SPI tarafında
register byte'ının MSB'sini set etme, CS'i indirip kaldırma işi bus
katmanında yapılır. Taşınabilirliğin yaklaşık %70'ini bu tek soyutlama
çözüyor.

### 3.5 Sensör sözleşmeleri

Sürücüler **ham sayaç değeri + ölçek katsayısı** döndürür; SI birimine
çevirmezler:

```c
typedef struct {
    const char *name;
    uint8_t     whoami_reg, whoami_val;
    fc_status_t (*init)(fc_imu_dev_t *dev);
    fc_status_t (*read_raw)(fc_imu_dev_t *dev, fc_vec3i_t *gyro,
                            fc_vec3i_t *accel, int16_t *temp_raw);
    float       (*temp_c)(int16_t temp_raw);
} fc_imu_driver_t;
```

Birim dönüşümü, eksen döndürme (`FC_ORIENT_CW270_FLIP` gibi) ve gyro bias
çıkarma `fc_imu.c` içinde **tek bir yerde** yapılır. Sonuç: yeni bir IMU
sürücüsü yaklaşık 80 satır ve kalibrasyon mantığını tekrar etmiyor.

Cihaz yapısında sürücüye özel veriler için sabit boyutlu bir alan var
(`uint8_t priv[FC_IMU_PRIV_MAX]`). BMP280 kalibrasyon katsayılarını,
SPL06 ise kendi 9 katsayısını ve oversampling ölçekleyicisini buraya
koyar. Dinamik bellek yok, ve aynı sürücüden birden fazla örnek
(çift gyro'lu kartlar) desteklenebiliyor.

### 3.6 `fc_sensors.c` — yapıştırıcı

`board_config.h`'ı okuyup gerekli çevre birimlerini bir kez ayağa kaldırır,
seçilen sürücüleri bağlar ve hazır cihazları döndürür. IMU ile barometre
aynı I2C hattındaysa hattı iki kez kurmaz. Uygulama katmanının gördüğü
tek dosya budur.

---

## 4. Sonuç: uygulama kodu karttan bağımsız

`examples/main.c` dosyası F405 kartında ve G474 kartında **byte byte
aynıdır**:

```c
if (fc_sensors_init(&g_sensors) != FC_OK) fail_blink();
fc_imu_calibrate_gyro(&g_sensors.imu, 1000u);

for (;;) {
    fc_imu_sample_t imu;
    if (fc_imu_read(&g_sensors.imu, &imu) == FC_OK) {
        /* estimator / PID / mixer buraya girecek */
    }
}
```

Değişen tek şey `board_config.h`'daki include satırı.

---

## 5. Desteklenenler

**MCU aileleri:** STM32F4 (F405, F411, F407), STM32G4 (G474, G431)
**IMU:** MPU6050 (I2C), ICM-42688-P (SPI/I2C)
**Barometre:** BMP280, SPL06-001

Her iki hedef konfigürasyon da `gcc -Wall -Wextra` ile uyarısız derleniyor.

---

## 6. Referans: DIY / açık donanım uçuş kontrol kartlarında kullanılan komponentler

Kendi kartımı (STM32F407VGT6 + MPU6050) tasarlarken ve FC_BSP'nin
kapsamını genişletirken, başka insanların tasarladığı açık donanım /
hobi amaçlı uçuş kontrol kartlarına bakıldı. Amaç: hangi komponent
kombinasyonlarının "standart" sayıldığını görüp `fc_device.h` ve
`Inc/boards/` altına hangi sürücülerin öncelikli eklenmesi gerektiğine
karar vermek.

İncelenen örnekler: OpenDrone AIO FC F405 (şematik + BOM açık,
STM32F405RGT6 tabanlı, GitHub'da `phonght32/OpenDrone_HW_AIO_FC_F405`),
Matek F405 serisi (STD/Mini/WING/WSE/HDTE — kullanıcı el kitaplarında
komponent listesi yayınlanıyor), Betaflight'ın resmi "manufacturer
design guidelines" belgesi, ve çeşitli hobi projeleri (Hermes-FlightController,
Elecrow açık STM32F4 kartı vb).

### 6.1 IMU

| Parça | Bus | Not |
|---|---|---|
| MPU6000 | SPI | Betaflight/INAV döneminin "altın standart"ı; gürültüye/titreşime dayanıklı, üretimi artık kısıtlı |
| MPU6050 | I2C | Bizim kartta kullanılan; ucuz, hobi/eğitim kartlarında hâlâ görülüyor, ama I2C-only olduğu için modern ticari tasarımlarda (Betaflight unified target) artık kabul edilmiyor |
| ICM-42688-P | SPI (I2C de var) | 2025-2026 itibarıyla en yaygın seçim; düşük gürültü, 8 kHz gyro çıkışı, BMI270 ile pin-uyumlu |
| BMI270 (Bosch) | SPI | Maksimum 6.4 kHz gyro hızı sınırlaması var; Betaflight yeni F411 tasarımlarında artık önerilmiyor |

FC_BSP zaten MPU6050 (I2C) ve ICM-42688-P (SPI/I2C) sürücü sözleşmesini
kapsıyor (bkz. §5). Sıradaki mantıklı ekleme MPU6000 (SPI) olurdu —
ICM-42688-P ile aynı sürücü mimarisini paylaşır (sadece register haritası
farklıdır), taşınabilirlik iddiasını test etmek için ucuz bir ek olur.

### 6.2 Barometre

| Parça | Bus | Not |
|---|---|---|
| BMP280 | I2C/SPI | En yaygın; ucuz, INAV/Matek kartlarının çoğunda |
| MS5611 | I2C/SPI | Eski nesil "altın standart", OpenPilot/Pixhawk ailesinde hâlâ referans |
| SPL06-001 | I2C | BMP280 muadili, bazı Çin üretimi kartlarda kullanılıyor |

FC_BSP zaten BMP280 ve SPL06-001'i kapsıyor; MS5611 eklenmesi Pixhawk/APM
tarzı kartlarla uyum için mantıklı olur ama racing/freestyle sınıfı
kartlarda öncelikli değil.

### 6.3 Kapsam dışı ama "gerçek" bir FC'de bulunan komponentler

FC_BSP şu an sadece IMU + barometre okuyor. Referans kartlarda ayrıca
şunlar var — hiçbiri şu anki mimariyi bozmaz, ama ileride `fc_port.h`
sözleşmesine eklenecek fonksiyonları önceden düşünmekte fayda var:

| Komponent | Örnek parça | FC_BSP'ye etkisi |
|---|---|---|
| Blackbox flash | W25Q128 (SPI NOR, 16-128 Mbit) | Ayrı bir `fc_bus` (SPI) tüketicisi; §8 yol haritasındaki "blackbox" maddesiyle örtüşüyor |
| OSD | AT7456E (SPI) | Analog FPV çıkışı planlanıyorsa gerekir; şu an kapsam dışı |
| Akım/gerilim ölçümü | Direnç bölücü (gerilim, ~1:10) + şönt/INA186 (akım) | Basit ADC okuması; `fc_port_adc_read()` gibi bir fonksiyon gerekecek |
| Motor çıkışı (DShot) | Timer + DMA | §8'de zaten "en riskli parça" olarak işaretli |
| RC girişi | SBUS (F4'te donanımsal invertör gerekir) / CRSF (invertöre gerek yok) | UART + DMA; F4 UART'ları RX'i donanımsal ters çeviremiyor — kartında invertör devresi yoksa CRSF daha kolay yol |
| WS2812 LED şeridi | Timer PWM + DMA (bit-banging) | Kozmetik, düşük öncelik |
| Buzzer | MOSFET üzerinden sürülen pasif buzzer | Basit GPIO, düşük öncelik |
| Programlama/debug | SWD (2 pin) | Zaten kartında olmalı |

### 6.4 Güç katmanı gözlemi

Referans kartların hepsinde en az iki regülatör var: MCU/sensörler için
3.3V LDO (RT9013, XC6206, AMS1117 gibi — düşük gürültü önemli, IMU aynı
hatta), ve harici yük (VTX, servo, alıcı) için ayrı bir 5V/9V anahtarlamalı
regülatör (TPS5430, MP2315 gibi). Kendi kartını tasarlayanlar için pratik
ders: IMU'nun analog/gürültü hassasiyeti yüzünden onu besleyen LDO'yu
motor sürücü hatlarından mümkün olduğunca ayrı tutmak yaygın bir
tasarım kuralı — kendi kartında gyro bias/gürültü sorunu yaşarsan ilk
bakılacak yer burası.

---

## 7. Genişletme

**Yeni kart:** `Inc/boards/` altına bir header, `board_config.h`'da bir
include satırı. Başka hiçbir dosyaya dokunulmaz.

**Yeni sensör:** tek bir `.c` dosyası (`init`, `read_raw`, sürücü yapısı),
`fc_device.h`'a bir kimlik `#define`'ı, `fc_sensors.c`'ye iki satır seçim.

**Yeni MCU ailesi:** `Src/port/fc_port_<aile>.c` içinde `fc_port.h`'daki
fonksiyonların uygulanması. Üst katmanların hiçbiri değişmez.

---

## 8. Bilinen sınırlar ve sıradaki adımlar

Şu an bir iskelet var; uçabilir bir firmware yok. Bilinçli olarak
yapılmayanlar ve doğrulanması gerekenler:

- **Saat ağacı kurulmuyor.** Kendi RCC kodun `main`'de çağrılmalı ve
  `FC_SYSCLK_HZ` onunla eşleşmeli.
- **Bus erişimleri blocking.** 14 byte IMU okuması 400 kHz I2C'de ~380 µs
  sürüyor; 1 kHz kontrol döngüsünün üçte biri. Sıradaki iş DMA + kesme.
- **F4 I2C 2 byte okuması** RM0090'daki POS bit dizisini kullanmıyor;
  basitleştirilmiş polling yolu pratikte çalışıyor ama scope ile
  doğrulanmalı.
- **G4 I2C TIMINGR** değeri kart dosyasında elle verilmeli (CubeMX veya
  RM0440 tablosu).
- **Sensör register'ları datasheet'lerden yazıldı**, gerçek donanımda
  doğrulanmadı.

### Yol haritası

1. Donanımda doğrulama: `fc_sensors_selftest()`, sabit kartta makul gyro bias
2. Telemetri + parametre okuma/yazma (yer istasyonu ile PID tune için şart)
3. Motor çıkışı: PWM → DShot. Timer + DMA gerektirdiği için port
   katmanının en riskli parçası, erken denemekte fayda var
4. RC girişi: SBUS / CRSF
5. Estimator (tamamlayıcı filtre → EKF), PID, mixer
6. Failsafe, arm/disarm mantığı, konfigürasyon saklama (flash)
7. Blackbox, OSD

---

## 9. Konumlandırma

Benzer sistemlerin konfigürasyon yaklaşımları:

| Proje | Yöntem |
|---|---|
| Betaflight | Derleme zamanı `#define` yığını + unified target |
| ArduPilot | `hwdef.dat` + Python kod üreteci |
| Zephyr | Devicetree |
| **FC_BSP** | Tek header, RTOS'suz, kod üreteci gerektirmeyen |

FC_BSP'nin iddiası hız ya da özellik sayısı değil: **okunabilir ve
öğretilebilir bir taşınabilirlik modeli.** Kendi kartını tasarlayan
birinin, bir kod üretecine ya da 100 bin satırlık bir kod tabanına
girmeden kartını uçurabilmesi hedefleniyor.