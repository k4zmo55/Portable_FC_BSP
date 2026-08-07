#include "rcc.h"
#include <stddef.h>

#define CLOCK_TIMEOUT   500000UL
#define HSI_VALUE_HZ    16000000UL

/* AHB/APB prescaler alan degerinden (0-15 / 0-7) vardiya (shift) miktarina
   cevrim tablosu -- alanlar dogrusal olmadigi icin (/32 atlaniyor) gerekli.
   Bkz. RM0090 RCC_CFGR aciklamasi / CMSIS system_stm32f4xx.c AHBPrescTable. */
static const uint8_t AHBPrescShiftTable[16] = {0,0,0,0,0,0,0,0,1,2,3,4,6,7,8,9};
static const uint8_t APBPrescShiftTable[8]  = {0,0,0,0,1,2,3,4};

/* RCC_Init HSE ile cagrildiginda kaydedilir; RCC_GetSystemClock() PLL/HSE
   frekansini geri hesaplarken kullanir (register'lar frekansi degil sadece
   carpan/bolen degerlerini tutar). */
static uint32_t s_HSEFreqHz = 0;

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

RCC_Status_t RCC_Init(RCC_Config_t *pRCCConfig)
{
    RCC_Status_t status;
    RCC_SysClkSource_t targetSrc;

    if(pRCCConfig == NULL                                  ||
       !IS_RCC_OSC_SOURCE(pRCCConfig->OscSource)            ||
       !IS_RCC_AHB_PRESCALER(pRCCConfig->AHBPrescaler)      ||
       !IS_RCC_APB_PRESCALER(pRCCConfig->APB1Prescaler)     ||
       !IS_RCC_APB_PRESCALER(pRCCConfig->APB2Prescaler))
    {
        return RCC_ERROR;
    }

    if(pRCCConfig->UsePLL &&
       (!IS_RCC_PLL_M(pRCCConfig->PLL.PLL_M) ||
        !IS_RCC_PLL_N(pRCCConfig->PLL.PLL_N) ||
        !IS_RCC_PLL_P(pRCCConfig->PLL.PLL_P) ||
        !IS_RCC_PLL_Q(pRCCConfig->PLL.PLL_Q)))
    {
        return RCC_ERROR;
    }

    /* 1) Secilen osilatoru ac, hazir olmasini bekle */
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

    if(status != RCC_OK)
    {
        return status;
    }

    /* 2) Flash gecikmesi - saat hizlanmadan ONCE ayarlanmali */
    FLASH->ACR = (pRCCConfig->FlashLatency << 0) /* LATENCY */
               | (1 << 8)                        /* PRFTEN */
               | (1 << 9)                        /* ICEN */
               | (1 << 10);                      /* DCEN */

    if(pRCCConfig->UsePLL)
    {
        /* 3) PLL kaynagini ve carpanlarini yaz */
        RCC->PLLCFGR = (pRCCConfig->PLL.PLL_M << 0)
                     | (pRCCConfig->PLL.PLL_N << 6)
                     | (((pRCCConfig->PLL.PLL_P / 2) - 1) << 16)
                     | (pRCCConfig->OscSource << 22)
                     | (pRCCConfig->PLL.PLL_Q << 24);

        /* 4) PLL'i ac, kilitlenmesini bekle */
        RCC->CR |= (1 << 24); /* PLLON */
        status = WaitFlag(&RCC->CR, (1 << 25)); /* PLLRDY */
        if(status != RCC_OK)
        {
            return status;
        }
    }

    /* 5) Bus prescaler'lari */
    RCC->CFGR &= ~(0xF << 4);
    RCC->CFGR |=  (pRCCConfig->AHBPrescaler << 4);
    RCC->CFGR &= ~(0x7 << 10);
    RCC->CFGR |=  (pRCCConfig->APB1Prescaler << 10);
    RCC->CFGR &= ~(0x7 << 13);
    RCC->CFGR |=  (pRCCConfig->APB2Prescaler << 13);

    /* 6) SYSCLK kaynagini sec ve donanimin gecis yaptigini dogrula */
    targetSrc = pRCCConfig->UsePLL ? RCC_SYSCLK_SRC_PLL :
                (pRCCConfig->OscSource == RCC_OSC_HSE ? RCC_SYSCLK_SRC_HSE : RCC_SYSCLK_SRC_HSI);

    RCC->CFGR &= ~(0x3 << 0);
    RCC->CFGR |=  (targetSrc << 0);

    uint32_t timeout = 0;
    while(((RCC->CFGR >> 2) & 0x3) != (uint32_t)targetSrc)
    {
        if(++timeout > CLOCK_TIMEOUT)
        {
            return RCC_TIMEOUT;
        }
    }

    return RCC_OK;
}

uint32_t RCC_GetSystemClock(void)
{
    uint32_t sysclk;
    uint8_t  clkSrc = (RCC->CFGR >> 2) & 0x3;

    if(clkSrc == RCC_SYSCLK_SRC_HSI)
    {
        sysclk = HSI_VALUE_HZ;
    }
    else if(clkSrc == RCC_SYSCLK_SRC_HSE)
    {
        sysclk = s_HSEFreqHz;
    }
    else /* RCC_SYSCLK_SRC_PLL */
    {
        uint32_t pllSrcIsHSE = (RCC->PLLCFGR >> 22) & 0x1;
        uint32_t pllM        = (RCC->PLLCFGR >> 0)  & 0x3F;
        uint32_t pllN        = (RCC->PLLCFGR >> 6)  & 0x1FF;
        uint32_t pllP        = (((RCC->PLLCFGR >> 16) & 0x3) + 1) * 2;
        uint32_t pllInputHz  = pllSrcIsHSE ? s_HSEFreqHz : HSI_VALUE_HZ;

        sysclk = ((pllInputHz / pllM) * pllN) / pllP;
    }

    return sysclk;
}

uint32_t RCC_GetPCLK1Value(void)
{
    uint32_t sysclk    = RCC_GetSystemClock();
    uint8_t  ahbShift   = AHBPrescShiftTable[(RCC->CFGR >> 4)  & 0xF];
    uint8_t  apb1Shift  = APBPrescShiftTable[(RCC->CFGR >> 10) & 0x7];

    return (sysclk >> ahbShift) >> apb1Shift;
}

uint32_t RCC_GetPCLK2Value(void)
{
    uint32_t sysclk    = RCC_GetSystemClock();
    uint8_t  ahbShift   = AHBPrescShiftTable[(RCC->CFGR >> 4)  & 0xF];
    uint8_t  apb2Shift  = APBPrescShiftTable[(RCC->CFGR >> 13) & 0x7];

    return (sysclk >> ahbShift) >> apb2Shift;
}
