#ifndef FC_GPIO_H
#define FC_GPIO_H

#include "stm32f407xx.h"
#include "stm32f4xx.h"
#include <stdint.h>

#define ENABLE  1
#define DISABLE 0

typedef struct{
    uint8_t PinNumber;
    uint8_t PinMode;
    uint8_t PinSpeed;
    uint8_t PinPuPdControl;
    uint8_t PinOPType;
    uint8_t PinAltFunMode;
}GPIO_PinConfig_t;

typedef struct{
    GPIOx_RegDef_t   *pGPIOx;     /*!< Handle'in bagli oldugu GPIO portu (GPIOA, GPIOB, ...) */
    GPIO_PinConfig_t  PinConfig;  /*!< Pin konfigurasyon ayarlari */
}GPIO_Handle_t;


/* ========================================================================= */
/*                              GPIO ENUM TYPES                              */
/* ========================================================================= */

/**
 * @brief Pin Numaraları (0 - 15)
 */
typedef enum {
    GPIO_PIN_NO_0 = 0,
    GPIO_PIN_NO_1,
    GPIO_PIN_NO_2,
    GPIO_PIN_NO_3,
    GPIO_PIN_NO_4,
    GPIO_PIN_NO_5,
    GPIO_PIN_NO_6,
    GPIO_PIN_NO_7,
    GPIO_PIN_NO_8,
    GPIO_PIN_NO_9,
    GPIO_PIN_NO_10,
    GPIO_PIN_NO_11,
    GPIO_PIN_NO_12,
    GPIO_PIN_NO_13,
    GPIO_PIN_NO_14,
    GPIO_PIN_NO_15
} GPIO_PinNumber_t;

/**
 * @brief Pin Çalışma Modları (Input, Output, AF, Analog, Interrupt)
 */
typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT,
    GPIO_MODE_ALTERNATE_FUNCTION,
    GPIO_MODE_ANALOG,
    GPIO_MODE_IT_RT,    /**< Rising Edge Trigger Interrupt */
    GPIO_MODE_IT_FT,    /**< Falling Edge Trigger Interrupt */
    GPIO_MODE_IT_RFT   /**< Rising/Falling Edge Trigger Interrupt */
} GPIO_Mode_t;

/**
 * @brief Pin Çıkış Hızı (Speed)
 */
typedef enum {
    GPIO_SPEED_LOW = 0,
    GPIO_SPEED_MEDIUM,
    GPIO_SPEED_HIGH,
    GPIO_SPEED_VERY_HIGH
} GPIO_Speed_t;

/**
 * @brief Pull-Up / Pull-Down Direnç Konfigürasyonu
 */
typedef enum {
    GPIO_NO_PUPD = 0,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} GPIO_PUPD_t;

/**
 * @brief Çıkış Tipi (Push-Pull / Open-Drain)
 */
typedef enum {
    GPIO_OUTPUT_PUSH_PULL = 0,
    GPIO_OUTPUT_OPEN_DRAIN
} GPIO_OType_t;

/**
 * @brief Alternatif Fonksiyon Seçenekleri (AF0 - AF15)
 */
typedef enum {
    GPIO_AF0 = 0,
    GPIO_AF1,
    GPIO_AF2,
    GPIO_AF3,
    GPIO_AF4,
    GPIO_AF5,
    GPIO_AF6,
    GPIO_AF7,
    GPIO_AF8,
    GPIO_AF9,
    GPIO_AF10,
    GPIO_AF11,
    GPIO_AF12,
    GPIO_AF13,
    GPIO_AF14,
    GPIO_AF15
} GPIO_AlternateFunction_t;

/**
 * @brief Fiziksel Pin Durumu (Lojik Seviye)
 */
typedef enum {
    GPIO_PIN_RESET = 0, /**< Lojik LOW  (0V) */
    GPIO_PIN_SET   = 1  /**< Lojik HIGH (3.3V / 5V) */
} GPIO_PinState_t;

/**
 * @brief API Fonksiyon Dönüş Durumları (İşlem Sonucu)
 */
typedef enum {
    GPIO_OK      =  0,  /**< İşlem başarılı */
    GPIO_ERROR   = -1,  /**< Genel/Donanım hatası */
    GPIO_BUSY    = -2,  /**< Hat meşgul */
    GPIO_TIMEOUT = -3   /**< Zaman aşımı */
} GPIO_Status_t;


#define IS_GPIO(pGPIOx) (((pGPIOx) == GPIOA) || ((pGPIOx) == GPIOB) || \
                          ((pGPIOx) == GPIOC) || ((pGPIOx) == GPIOD) || \
                          ((pGPIOx) == GPIOE) || ((pGPIOx) == GPIOF) || \
                          ((pGPIOx) == GPIOG) || ((pGPIOx) == GPIOH) || \
                          ((pGPIOx) == GPIOI))

#define IS_GPIO_PIN(Pin) ((Pin) <= GPIO_PIN_NO_15)

#define IS_GPIO_MODE(Mode) (((Mode) == GPIO_MODE_INPUT)              || \
                             ((Mode) == GPIO_MODE_OUTPUT)             || \
                             ((Mode) == GPIO_MODE_ALTERNATE_FUNCTION) || \
                             ((Mode) == GPIO_MODE_ANALOG)             || \
                             ((Mode) == GPIO_MODE_IT_RT)              || \
                             ((Mode) == GPIO_MODE_IT_FT)              || \
                             ((Mode) == GPIO_MODE_IT_RFT))

#define IS_GPIO_SPEED(Speed) (((Speed) == GPIO_SPEED_LOW)    || \
                               ((Speed) == GPIO_SPEED_MEDIUM) || \
                               ((Speed) == GPIO_SPEED_HIGH)   || \
                               ((Speed) == GPIO_SPEED_VERY_HIGH))

#define IS_GPIO_PUPD(PuPd) (((PuPd) == GPIO_NO_PUPD)   || \
                             ((PuPd) == GPIO_PULL_UP)   || \
                             ((PuPd) == GPIO_PULL_DOWN))

#define IS_GPIO_OTYPE(OType) (((OType) == GPIO_OUTPUT_PUSH_PULL) || \
                               ((OType) == GPIO_OUTPUT_OPEN_DRAIN))

#define IS_GPIO_AF(AF) ((AF) <= GPIO_AF15)

#define IS_GPIO_PIN_STATE(State) (((State) == GPIO_PIN_RESET) || ((State) == GPIO_PIN_SET))

#define IS_FUNCTIONAL_STATE(State) (((State) == ENABLE) || ((State) == DISABLE))

/* ========================================================================= */
/*                            GPIO API FONKSIYONLARI                         */
/* ========================================================================= */

void            GPIO_PeriClockControl(GPIOx_RegDef_t *pGPIOx, uint8_t EnOrDi);

GPIO_Status_t   GPIO_Init(GPIO_Handle_t *pGPIOHandle);
void            GPIO_DeInit(GPIOx_RegDef_t *pGPIOx);

GPIO_Status_t GPIO_ReadFromPin(GPIOx_RegDef_t *pGPIOx, uint8_t PinNumber);
uint16_t        GPIO_ReadFromPort(GPIOx_RegDef_t *pGPIOx);

GPIO_Status_t   GPIO_WriteToPin(GPIOx_RegDef_t *pGPIOx, uint8_t PinNumber, GPIO_PinState_t PinState);
GPIO_Status_t   GPIO_WriteToPort(GPIOx_RegDef_t *pGPIOx, uint16_t Value);

void   GPIO_TogglePin(GPIOx_RegDef_t *pGPIOx, uint8_t PinNumber);

void   GPIO_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi);
void   GPIO_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority);
void   GPIO_IRQHandling(uint8_t PinNumber);

#endif /* FC_GPIO_H */

