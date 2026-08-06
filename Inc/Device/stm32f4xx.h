#ifndef FC_STM32F4XX_H
#define FC_STM32F4XX_H


#ifdef __cplusplus
extern "C"{
#endif

#include <stdint.h>
#include <stddef.h>

#if defined(STM32F405xx) 
    #include "stm32f405xx.h"
#elif defined(STM32F407xx)
    #include "stm32f407xx.h"
#else
    #error "Desteklenen bir MCU varyanti tanimlanmamis (STM32F405xx / STM32F407xx)"
#endif

#define AHB3_BASE_ADDR (0xA0000000UL)
#define AHB2_BASE_ADDR (0x50000000UL)
#define AHB1_BASE_ADDR (0x40020000UL)
#define APB2_BASE_ADDR (0x40010000UL)
#define APB1_BASE_ADDR (0x40000000UL)

/******************************************************

        APB1 Peripherals Base Address Definations

******************************************************/
#define TIM2_BASE_ADDR  (APB1_BASE_ADDR + 0x0000)
#define TIM3_BASE_ADDR  (APB1_BASE_ADDR + 0x0400)
#define TIM4_BASE_ADDR  (APB1_BASE_ADDR + 0x0800)
#define TIM5_BASE_ADDR  (APB1_BASE_ADDR + 0x0C00)
#define TIM6_BASE_ADDR  (APB1_BASE_ADDR + 0x1000)
#define TIM7_BASE_ADDR  (APB1_BASE_ADDR + 0x1400)
#define TIM12_BASE_ADDR (APB1_BASE_ADDR + 0x1800)
#define TIM13_BASE_ADDR (APB1_BASE_ADDR + 0x1C00)
#define TIM14_BASE_ADDR (APB1_BASE_ADDR + 0x2000)
#define RTC_BASE_ADDR   (APB1_BASE_ADDR + 0x2800)
#define WWDG_BASE_ADDR  (APB1_BASE_ADDR + 0x2C00)
#define IWDG_BASE_ADDR  (APB1_BASE_ADDR + 0x3000)
#define SPI2_BASE_ADDR  (APB1_BASE_ADDR + 0x3800)
#define SPI3_BASE_ADDR  (APB1_BASE_ADDR + 0x3C00)
#define USART2_BASE_ADDR (APB1_BASE_ADDR + 0x4400)
#define USART3_BASE_ADDR (APB1_BASE_ADDR + 0x4800)
#define UART4_BASE_ADDR  (APB1_BASE_ADDR + 0x4C00)
#define UART5_BASE_ADDR  (APB1_BASE_ADDR + 0x5000)
#define I2C1_BASE_ADDR   (APB1_BASE_ADDR + 0x5400)
#define I2C2_BASE_ADDR   (APB1_BASE_ADDR + 0x5800)
#define I2C3_BASE_ADDR   (APB1_BASE_ADDR + 0x5C00)
#define CAN1_BASE_ADDR   (APB1_BASE_ADDR + 0x6400)
#define CAN2_BASE_ADDR   (APB1_BASE_ADDR + 0x6800)
#define PWR_BASE_ADDR    (APB1_BASE_ADDR + 0x7000)
#define DAC_BASE_ADDR    (APB1_BASE_ADDR + 0x7400)
#define UART7_BASE_ADDR  (APB1_BASE_ADDR + 0x7800)
#define UART8_BASE_ADDR  (APB1_BASE_ADDR + 0x7C00)


/******************************************************

        APB2 Peripherals Base Address Definations

******************************************************/
#define TIM1_BASE_ADDR   (APB2_BASE_ADDR + 0x0000)
#define TIM8_BASE_ADDR   (APB2_BASE_ADDR + 0x0400)
#define USART1_BASE_ADDR (APB2_BASE_ADDR + 0x1000)
#define USART6_BASE_ADDR (APB2_BASE_ADDR + 0x1400)
#define ADC_BASE_ADDR    (APB2_BASE_ADDR + 0x2000)
#define SDIO_BASE_ADDR   (APB2_BASE_ADDR + 0x2C00)
#define SPI1_BASE_ADDR   (APB2_BASE_ADDR + 0x3000)
#define SPI4_BASE_ADDR   (APB2_BASE_ADDR + 0x3400)
#define SYSCFG_BASE_ADDR (APB2_BASE_ADDR + 0x3800)
#define EXTI_BASE_ADDR   (APB2_BASE_ADDR + 0x3C00)
#define TIM9_BASE_ADDR   (APB2_BASE_ADDR + 0x4000)
#define TIM10_BASE_ADDR  (APB2_BASE_ADDR + 0x4400)
#define TIM11_BASE_ADDR  (APB2_BASE_ADDR + 0x4800)
#define SPI5_BASE_ADDR   (APB2_BASE_ADDR + 0x5000)
#define SPI6_BASE_ADDR   (APB2_BASE_ADDR + 0x5400)

/******************************************************

        AHB1 Peripherals Base Address Definations

******************************************************/
#define GPIOA_BASE_ADDR  (AHB1_BASE_ADDR + 0x0000)
#define GPIOB_BASE_ADDR  (AHB1_BASE_ADDR + 0x0400)
#define GPIOC_BASE_ADDR  (AHB1_BASE_ADDR + 0x0800)
#define GPIOD_BASE_ADDR  (AHB1_BASE_ADDR + 0x0C00)
#define GPIOE_BASE_ADDR  (AHB1_BASE_ADDR + 0x1000)
#define GPIOF_BASE_ADDR  (AHB1_BASE_ADDR + 0x1400)
#define GPIOG_BASE_ADDR  (AHB1_BASE_ADDR + 0x1800)
#define GPIOH_BASE_ADDR  (AHB1_BASE_ADDR + 0x1C00)
#define GPIOI_BASE_ADDR  (AHB1_BASE_ADDR + 0x2000)
#define GPIOJ_BASE_ADDR  (AHB1_BASE_ADDR + 0x2400)
#define GPIOK_BASE_ADDR  (AHB1_BASE_ADDR + 0x2800)
#define CRC_BASE_ADDR    (AHB1_BASE_ADDR + 0x3000)
#define RCC_BASE_ADDR    (AHB1_BASE_ADDR + 0x3800)
#define FLASH_BASE_ADDR  (AHB1_BASE_ADDR + 0x3C00)
#define BKPSRAM_BASE_ADDR (AHB1_BASE_ADDR + 0x4000)
#define DMA1_BASE_ADDR   (AHB1_BASE_ADDR + 0x6000)
#define DMA2_BASE_ADDR   (AHB1_BASE_ADDR + 0x6400)


typedef struct{
    volatile uint32_t MODER;    /*Offset: 0x00*/
    volatile uint32_t OTYPER;   /*Offset: 0x04*/
    volatile uint32_t OSPEEDR;  /*Offset: 0x08*/
    volatile uint32_t PUPDR;    /*Offset: 0x0C*/
    volatile uint32_t IDR;      /*Offset: 0x10*/
    volatile uint32_t ODR;      /*Offset: 0x14*/
    volatile uint32_t BSRR;     /*Offset: 0x18*/ 
    volatile uint32_t LCKR;     /*Offset: 0x1C*/
    volatile uint32_t AFR[2];   /*Offset: 0x20 + 0x24*/
}GPIOx_RegDef_t;


typedef struct {
  volatile uint32_t CR;           /*!< 0x00  clock control                        */
  volatile uint32_t PLLCFGR;      /*!< 0x04  PLL konfigurasyonu                   */
  volatile uint32_t CFGR;         /*!< 0x08  clock konfigurasyonu                 */
  volatile uint32_t CIR;          /*!< 0x0C  clock interrupt                      */
  volatile uint32_t AHB1RSTR;     /*!< 0x10                                       */
  volatile uint32_t AHB2RSTR;     /*!< 0x14                                       */
  volatile uint32_t AHB3RSTR;     /*!< 0x18                                       */
  uint32_t      RESERVED0;    /*!< 0x1C                                       */
  volatile uint32_t APB1RSTR;     /*!< 0x20                                       */
  volatile uint32_t APB2RSTR;     /*!< 0x24                                       */
  uint32_t      RESERVED1[2]; /*!< 0x28                                       */
  volatile uint32_t AHB1ENR;      /*!< 0x30  AHB1 clock enable                    */
  volatile uint32_t AHB2ENR;      /*!< 0x34                                       */
  volatile uint32_t AHB3ENR;      /*!< 0x38                                       */
  uint32_t      RESERVED2;    /*!< 0x3C                                       */
  volatile uint32_t APB1ENR;      /*!< 0x40  APB1 clock enable                    */
  volatile uint32_t APB2ENR;      /*!< 0x44  APB2 clock enable                    */
  uint32_t      RESERVED3[2]; /*!< 0x48                                       */
  volatile uint32_t AHB1LPENR;    /*!< 0x50                                       */
  volatile uint32_t AHB2LPENR;    /*!< 0x54                                       */
  volatile uint32_t AHB3LPENR;    /*!< 0x58                                       */
  uint32_t      RESERVED4;    /*!< 0x5C                                       */
  volatile uint32_t APB1LPENR;    /*!< 0x60                                       */
  volatile uint32_t APB2LPENR;    /*!< 0x64                                       */
  uint32_t      RESERVED5[2]; /*!< 0x68                                       */
  volatile uint32_t BDCR;         /*!< 0x70  backup domain control                */
  volatile uint32_t CSR;          /*!< 0x74  clock control & status               */
  uint32_t      RESERVED6[2]; /*!< 0x78                                       */
  volatile uint32_t SSCGR;        /*!< 0x80  spread spectrum                      */
  volatile uint32_t PLLI2SCFGR;   /*!< 0x84                                       */
}RCC_TypeDef;

typedef struct {
    volatile uint32_t MEMRMP;    /*!< 0x00 Memory remap: adres 0x00000000'a Flash/SRAM/FSMC esleme    */
    volatile uint32_t PMC;       /*!< 0x04 Peripheral mode config: ETH MII/RMII secimi, ADCxDC2       */
    volatile uint32_t EXTICR1;   /*!< 0x08 EXTI hat 0-3   -> hangi port (4 bit/hat)                    */
    volatile uint32_t EXTICR2;   /*!< 0x0C EXTI hat 4-7   -> hangi port                                */
    volatile uint32_t EXTICR3;   /*!< 0x10 EXTI hat 8-11  -> hangi port                                */
    volatile uint32_t EXTICR4;   /*!< 0x14 EXTI hat 12-15 -> hangi port                                */
    uint32_t RESERVED[2];        /*!< 0x18 - 0x1C rezerve (ATLAMA: CMPCR offsetini kaydirir)          */
    volatile uint32_t CMPCR;     /*!< 0x20 Compensation cell control: I/O surucu kompanzasyonu        */
} SYSCFG_TypeDef_t;

typedef struct {
    volatile uint32_t CR1;       /*!< 0x00 Control 1: PE, START, STOP, ACK, SWRST                     */
    volatile uint32_t CR2;       /*!< 0x04 Control 2: FREQ[5:0] (APB1 MHz), ITEVTEN, ITBUFEN, DMAEN   */
    volatile uint32_t OAR1;      /*!< 0x08 Own address 1: slave modda kendi adresi (7/10 bit)         */
    volatile uint32_t OAR2;      /*!< 0x0C Own address 2: dual address modu icin ikinci adres         */
    volatile uint32_t DR;        /*!< 0x10 Data: 8 bit gonderme/alma                                  */
    volatile uint32_t SR1;       /*!< 0x14 Status 1: SB, ADDR, BTF, RXNE, TXE + hata bayraklari       */
    volatile uint32_t SR2;       /*!< 0x18 Status 2: MSL, BUSY, TRA -- ADDR temizlemek icin okunur    */
    volatile uint32_t CCR;       /*!< 0x1C Clock control: SCL periyodu, F/S ve DUTY bitleri           */
    volatile uint32_t TRISE;     /*!< 0x20 Maks yukselme suresi: (APB1_MHz + 1) standart modda        */
    volatile uint32_t FLTR;      /*!< 0x24 Digital/analog gurultu filtresi                            */
} I2Cx_RegDef_t;

typedef struct {
    volatile uint32_t CR1;      /*!< 0x00 Control 1: CPHA, CPOL, MSTR, BR[2:0], SPE, LSBFIRST, SSI, SSM, DFF */
    volatile uint32_t CR2;      /*!< 0x04 Control 2: TXDMAEN, RXDMAEN, SSOE, TXEIE, RXNEIE, ERRIE            */
    volatile uint32_t SR;       /*!< 0x08 Status: RXNE, TXE, BSY, OVR, MODF, CRCERR                          */
    volatile uint32_t DR;       /*!< 0x0C Data: yazmak TX'e, okumak RX'ten -- ayni adres, iki fiziksel reg.  */
    volatile uint32_t CRCPR;    /*!< 0x10 CRC polinomu (reset degeri 0x0007); CRC kullanmiyorsan dokunma     */
    volatile uint32_t RXCRCR;   /*!< 0x14 Alinan veriden hesaplanan CRC (read-only)                          */
    volatile uint32_t TXCRCR;   /*!< 0x18 Gonderilen veriden hesaplanan CRC (read-only)                      */
    volatile uint32_t I2SCFGR;  /*!< 0x1C I2S konfigurasyonu: I2SMOD=0 birakilirsa blok SPI olarak calisir   */
    volatile uint32_t I2SPR;    /*!< 0x20 I2S prescaler; sadece I2S modunda anlamli                          */
} SPIx_RegDef_t;

typedef struct{
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR; 
}USARTx_RegDef_t;

#define GPIOA ((GPIOx_RegDef_t*)GPIOA_BASE_ADDR)
#define GPIOB ((GPIOx_RegDef_t*)GPIOB_BASE_ADDR)
#define GPIOC ((GPIOx_RegDef_t*)GPIOC_BASE_ADDR)
#define GPIOD ((GPIOx_RegDef_t*)GPIOD_BASE_ADDR)
#define GPIOE ((GPIOx_RegDef_t*)GPIOE_BASE_ADDR)
#define GPIOF ((GPIOx_RegDef_t*)GPIOF_BASE_ADDR)
#define GPIOG ((GPIOx_RegDef_t*)GPIOG_BASE_ADDR)
#define GPIOH ((GPIOx_RegDef_t*)GPIOH_BASE_ADDR)
#define GPIOI ((GPIOx_RegDef_t*)GPIOI_BASE_ADDR)
#define GPIOJ ((GPIOx_RegDef_t*)GPIOJ_BASE_ADDR)
#define GPIOK ((GPIOx_RegDef_t*)GPIOK_BASE_ADDR)

#define RCC   ((RCC_TypeDef_t*)RCC_BASE_ADDR)

#define SYSCFG ((SYSCFG_TypeDef_t*)SYSCFG_BASE_ADDR)

#define I2C1  ((I2Cx_RegDef_t*)I2C1_BASE_ADDR)
#define I2C2  ((I2Cx_RegDef_t*)I2C2_BASE_ADDR)
#define I2C3  ((I2Cx_RegDef_t*)I2C3_BASE_ADDR)

#define SPI1  ((SPIx_RegDef_t*)SPI1_BASE_ADDR)
#define SPI2  ((SPIx_RegDef_t*)SPI2_BASE_ADDR)
#define SPI3  ((SPIx_RegDef_t*)SPI3_BASE_ADDR)
#define SPI4  ((SPIx_RegDef_t*)SPI4_BASE_ADDR)
#define SPI5  ((SPIx_RegDef_t*)SPI5_BASE_ADDR)
#define SPI6  ((SPIx_RegDef_t*)SPI6_BASE_ADDR)

#define USART1 ((USARTx_RegDef_t*)USART1_BASE_ADDR)
#define USART2 ((USARTx_RegDef_t*)USART2_BASE_ADDR)
#define USART3 ((USARTx_RegDef_t*)USART3_BASE_ADDR)
#define USART6 ((USARTx_RegDef_t*)USART6_BASE_ADDR)
#define UART4  ((USARTx_RegDef_t*)UART4_BASE_ADDR)
#define UART5  ((USARTx_RegDef_t*)UART5_BASE_ADDR)
#define UART7  ((USARTx_RegDef_t*)UART7_BASE_ADDR)
#define UART8  ((USARTx_RegDef_t*)UART8_BASE_ADDR)

#define GPIOA_PCLK_EN()  (RCC->AHB1ENR |= (1 << 0))
#define GPIOB_PCLK_EN()  (RCC->AHB1ENR |= (1 << 1))
#define GPIOC_PCLK_EN()  (RCC->AHB1ENR |= (1 << 2))
#define GPIOD_PCLK_EN()  (RCC->AHB1ENR |= (1 << 3))
#define GPIOE_PCLK_EN()  (RCC->AHB1ENR |= (1 << 4))
#define GPIOF_PCLK_EN()  (RCC->AHB1ENR |= (1 << 5))
#define GPIOG_PCLK_EN()  (RCC->AHB1ENR |= (1 << 6))
#define GPIOH_PCLK_EN()  (RCC->AHB1ENR |= (1 << 7))
#define GPIOI_PCLK_EN()  (RCC->AHB1ENR |= (1 << 8))
#define CRC_PCLK_EN()    (RCC->AHB1ENR |= (1 << 12))

#define GPIOA_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 0))
#define GPIOB_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 1))
#define GPIOC_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 2))
#define GPIOD_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 3))
#define GPIOE_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 4))
#define GPIOF_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 5))
#define GPIOG_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 6))
#define GPIOH_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 7))
#define GPIOI_PCLK_DI()  (RCC->AHB1ENR &= ~(1 << 8))
#define CRC_PCLK_DI()    (RCC->AHB1ENR &= ~(1 << 12))

#define TIM2_PCLK_EN()   (RCC->APB1ENR |= (1 << 0))
#define TIM3_PCLK_EN()   (RCC->APB1ENR |= (1 << 1))
#define TIM4_PCLK_EN()   (RCC->APB1ENR |= (1 << 2))
#define TIM5_PCLK_EN()   (RCC->APB1ENR |= (1 << 3))
#define TIM6_PCLK_EN()   (RCC->APB1ENR |= (1 << 4))
#define TIM7_PCLK_EN()   (RCC->APB1ENR |= (1 << 5))
#define TIM12_PCLK_EN()  (RCC->APB1ENR |= (1 << 6))
#define TIM13_PCLK_EN()  (RCC->APB1ENR |= (1 << 7))
#define TIM14_PCLK_EN()  (RCC->APB1ENR |= (1 << 8))
#define WWDG_PCLK_EN()   (RCC->APB1ENR |= (1 << 11))
#define SPI2_PCLK_EN()   (RCC->APB1ENR |= (1 << 14))
#define SPI3_PCLK_EN()   (RCC->APB1ENR |= (1 << 15))
#define USART2_PCLK_EN() (RCC->APB1ENR |= (1 << 17))
#define USART3_PCLK_EN() (RCC->APB1ENR |= (1 << 18))
#define UART4_PCLK_EN()  (RCC->APB1ENR |= (1 << 19))
#define UART5_PCLK_EN()  (RCC->APB1ENR |= (1 << 20))
#define I2C1_PCLK_EN()   (RCC->APB1ENR |= (1 << 21))
#define I2C2_PCLK_EN()   (RCC->APB1ENR |= (1 << 22))
#define I2C3_PCLK_EN()   (RCC->APB1ENR |= (1 << 23))
#define CAN1_PCLK_EN()   (RCC->APB1ENR |= (1 << 25))
#define CAN2_PCLK_EN()   (RCC->APB1ENR |= (1 << 26))
#define PWR_PCLK_EN()    (RCC->APB1ENR |= (1 << 28))
#define DAC_PCLK_EN()    (RCC->APB1ENR |= (1 << 29))


#define TIM12_PCLK_DI()  (RCC->APB1ENR &= ~(1 << 0))


#define TIM1_PCLK_EN()   (RCC->APB2ENR |= (1 << 0))
#define TIM8_PCLK_EN()   (RCC->APB2ENR |= (1 << 1))
#define USART1_PCLK_EN() (RCC->APB2ENR |= (1 << 4))
#define USART6_PCLK_EN() (RCC->APB2ENR |= (1 << 5))
#define ADC1_PCLK_EN()   (RCC->APB2ENR |= (1 << 8))
#define ADC2_PCLK_EN()   (RCC->APB2ENR |= (1 << 9))
#define ADC3_PCLK_EN()   (RCC->APB2ENR |= (1 << 10))
#define SPI1_PCLK_EN()   (RCC->APB2ENR |= (1 << 12))
#define SYSCFG_PCLK_EN() (RCC->APB2ENR |= (1 << 14))
#define TIM9_PCLK_EN()   (RCC->APB2ENR |= (1 << 16))
#define TIM10_PCLK_EN()  (RCC->APB2ENR |= (1 << 17))
#define TIM11_PCLK_EN()  (RCC->APB2ENR |= (1 << 18))

#ifdef _cplusplus
}
#endif

#endif