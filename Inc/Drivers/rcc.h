#ifndef FC_RCC_H
#define FC_RCC_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ========================================================================= */
/*                              RCC ENUM TYPES                               */
/* ========================================================================= */

/**
 * @brief Osilator Kaynagi (PLL girisi ya da dogrudan SYSCLK icin)
 *        Deger, RCC->PLLCFGR PLLSRC bitiyle birebir eslesir.
 */
typedef enum {
    RCC_OSC_HSI = 0, /**< Dahili 16 MHz RC osilator */
    RCC_OSC_HSE = 1  /**< Harici kristal/osilator */
} RCC_OscSource_t;

/**
 * @brief Sistem Saati (SYSCLK) Kaynagi
 *        Deger, RCC->CFGR SW/SWS alanlarıyla birebir eslesir.
 */
typedef enum {
    RCC_SYSCLK_SRC_HSI = 0x0,
    RCC_SYSCLK_SRC_HSE = 0x1,
    RCC_SYSCLK_SRC_PLL = 0x2
} RCC_SysClkSource_t;

/**
 * @brief AHB Prescaler (HPRE)
 *        Deger, RCC->CFGR HPRE alaniyla birebir eslesir (dogrusal degil, /32 atlanir).
 */
typedef enum {
    RCC_AHB_DIV_1   = 0x0,
    RCC_AHB_DIV_2   = 0x8,
    RCC_AHB_DIV_4   = 0x9,
    RCC_AHB_DIV_8   = 0xA,
    RCC_AHB_DIV_16  = 0xB,
    RCC_AHB_DIV_64  = 0xC,
    RCC_AHB_DIV_128 = 0xD,
    RCC_AHB_DIV_256 = 0xE,
    RCC_AHB_DIV_512 = 0xF
} RCC_AHBPrescaler_t;

/**
 * @brief APB Prescaler (PPRE1 / PPRE2 icin ortak)
 *        Deger, RCC->CFGR PPREx alaniyla birebir eslesir.
 */
typedef enum {
    RCC_APB_DIV_1  = 0x0,
    RCC_APB_DIV_2  = 0x4,
    RCC_APB_DIV_4  = 0x5,
    RCC_APB_DIV_8  = 0x6,
    RCC_APB_DIV_16 = 0x7
} RCC_APBPrescaler_t;

/**
 * @brief API Fonksiyon Donus Durumlari
 */
typedef enum {
    RCC_OK      =  0, /**< Islem basarili */
    RCC_ERROR   = -1, /**< Gecersiz konfigurasyon */
    RCC_TIMEOUT = -2  /**< HSE/PLL/SYSCLK gecisi zaman asimina ugradi */
} RCC_Status_t;

/* ========================================================================= */
/*                            RCC CONFIG STRUCT'LARI                         */
/* ========================================================================= */

/**
 * @brief PLL Carpan/Bolen Parametreleri
 *        Butun degerler RCC->PLLCFGR alanlarina dogrudan yazilir (register
 *        semantigiyle birebir), boylece herhangi bir HSE/HSI frekansi ve
 *        hedef SYSCLK icin ayni RCC_Init() kullanilabilir.
 */
typedef struct {
    uint32_t PLL_M; /**< 2-63:   PLL giris bolen (VCO girisi 1-2 MHz araliginda olmali) */
    uint32_t PLL_N; /**< 50-432: VCO carpan (VCO cikisi 100-432 MHz araliginda olmali) */
    uint32_t PLL_P; /**< 2/4/6/8: SYSCLK bolen (SYSCLK = VCO_out / PLL_P) */
    uint32_t PLL_Q; /**< 2-15:   USB OTG FS / SDIO / RNG bolen (hedef 48 MHz) */
} RCC_PLLConfig_t;

/**
 * @brief Genel RCC Konfigurasyonu (islemciden bagimsiz mantik)
 *
 * Bu struct'i dolduran kod (main.c ya da board init) hicbir register
 * adresi/bit pozisyonu bilmek zorunda degil; sadece "hangi kaynak,
 * hangi hiz, hangi bolenler" sorusuna cevap verir. Register'a yazma
 * islemi RCC_Init() icinde, MCU ailesine ozel rcc.c dosyasinda yapilir.
 * Ayni struct STM32F4 disindaki bir Cortex-M ailesi icin de (kendi
 * rcc.c'siyle) yeniden kullanilabilir -- degisen sadece RCC_Init()'in
 * govdesidir, cagiran kod degil.
 */
typedef struct {
    RCC_OscSource_t     OscSource;     /**< PLL girisi / dogrudan SYSCLK icin HSI ya da HSE */
    uint32_t            OscFreqHz;     /**< OscSource frekansi (Hz) -- HSE ise kristal degeri */

    uint8_t              UsePLL;        /**< 1: SYSCLK = PLL cikisi, 0: SYSCLK = OscSource */
    RCC_PLLConfig_t      PLL;           /**< UsePLL=1 ise kullanilir */

    RCC_AHBPrescaler_t   AHBPrescaler;
    RCC_APBPrescaler_t   APB1Prescaler; /**< Dusuk hizli APB (STM32F4'te <=42 MHz) */
    RCC_APBPrescaler_t   APB2Prescaler; /**< Yuksek hizli APB (STM32F4'te <=84 MHz) */

    uint8_t              FlashLatency;  /**< Hedef SYSCLK + Vcore'a gore hesaplanmis wait-state sayisi */
} RCC_Config_t;

#define IS_RCC_OSC_SOURCE(Src)     (((Src) == RCC_OSC_HSI) || ((Src) == RCC_OSC_HSE))

#define IS_RCC_AHB_PRESCALER(P)    (((P) == RCC_AHB_DIV_1)   || ((P) == RCC_AHB_DIV_2)   || \
                                     ((P) == RCC_AHB_DIV_4)   || ((P) == RCC_AHB_DIV_8)   || \
                                     ((P) == RCC_AHB_DIV_16)  || ((P) == RCC_AHB_DIV_64)  || \
                                     ((P) == RCC_AHB_DIV_128) || ((P) == RCC_AHB_DIV_256) || \
                                     ((P) == RCC_AHB_DIV_512))

#define IS_RCC_APB_PRESCALER(P)    (((P) == RCC_APB_DIV_1) || ((P) == RCC_APB_DIV_2) || \
                                     ((P) == RCC_APB_DIV_4) || ((P) == RCC_APB_DIV_8) || \
                                     ((P) == RCC_APB_DIV_16))

#define IS_RCC_PLL_M(M)            (((M) >= 2U)  && ((M) <= 63U))
#define IS_RCC_PLL_N(N)            (((N) >= 50U) && ((N) <= 432U))
#define IS_RCC_PLL_P(P)            (((P) == 2U) || ((P) == 4U) || ((P) == 6U) || ((P) == 8U))
#define IS_RCC_PLL_Q(Q)            (((Q) >= 2U) && ((Q) <= 15U))

/* ========================================================================= */
/*                             RCC API FONKSIYONLARI                         */
/* ========================================================================= */

RCC_Status_t RCC_Init(RCC_Config_t *pRCCConfig);

uint32_t     RCC_GetSystemClock(void);
uint32_t     RCC_GetPCLK1Value(void);
uint32_t     RCC_GetPCLK2Value(void);

#endif /* FC_RCC_H */
