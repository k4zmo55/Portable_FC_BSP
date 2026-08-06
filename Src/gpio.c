#include "gpio.h"
#include <stdint.h>

void GPIO_PeriClockControl(GPIOx_RegDef_t *pGPIOx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        if(pGPIOx == GPIOA)      { GPIOA_PCLK_EN(); }
        else if(pGPIOx == GPIOB) { GPIOB_PCLK_EN(); }
        else if(pGPIOx == GPIOC) { GPIOC_PCLK_EN(); }
        else if(pGPIOx == GPIOD) { GPIOD_PCLK_EN(); }
        else if(pGPIOx == GPIOE) { GPIOE_PCLK_EN(); }
        else if(pGPIOx == GPIOF) { GPIOF_PCLK_EN(); }
        else if(pGPIOx == GPIOG) { GPIOG_PCLK_EN(); }
        else if(pGPIOx == GPIOH) { GPIOH_PCLK_EN(); }
        else if(pGPIOx == GPIOI) { GPIOI_PCLK_EN(); }
    }
    else
    {
        if(pGPIOx == GPIOA)      { GPIOA_PCLK_DI(); }
        else if(pGPIOx == GPIOB) { GPIOB_PCLK_DI(); }
        else if(pGPIOx == GPIOC) { GPIOC_PCLK_DI(); }
        else if(pGPIOx == GPIOD) { GPIOD_PCLK_DI(); }
        else if(pGPIOx == GPIOE) { GPIOE_PCLK_DI(); }
        else if(pGPIOx == GPIOF) { GPIOF_PCLK_DI(); }
        else if(pGPIOx == GPIOG) { GPIOG_PCLK_DI(); }
        else if(pGPIOx == GPIOH) { GPIOH_PCLK_DI(); }
        else if(pGPIOx == GPIOI) { GPIOI_PCLK_DI(); }
    }
}

GPIO_Status_t GPIO_Init(GPIO_Handle_t *pGPIOHandle)
{
    if(pGPIOHandle == NULL || !IS_GPIO(pGPIOHandle->pGPIOx))
    {
        return GPIO_ERROR;
    }

    if(!IS_GPIO_PIN(pGPIOHandle->PinConfig.PinNumber)   || 
       !IS_GPIO_MODE(pGPIOHandle->PinConfig.PinMode)    ||
       !IS_GPIO_OTYPE(pGPIOHandle->PinConfig.PinOPType) ||
       !IS_GPIO_SPEED(pGPIOHandle->PinConfig.PinSpeed)  ||
       !IS_GPIO_PUPD(pGPIOHandle->PinConfig.PinPuPdControl))
    {
        return GPIO_ERROR;
    }

    if(!IS_GPIO_AF(pGPIOHandle->PinConfig.PinAltFunMode))
    {
        return GPIO_ERROR;
    }
    //GPIO Mode Configuration

    if(pGPIOHandle->PinConfig.PinMode <= GPIO_MODE_ANALOG)
    {
        pGPIOHandle->pGPIOx->MODER &= ~(0x3 << (pGPIOHandle->PinConfig.PinNumber * 2));
        pGPIOHandle->pGPIOx->MODER |= (pGPIOHandle->PinConfig.PinMode << (pGPIOHandle->PinConfig.PinNumber * 2));
    } 
    else
    {
        //Interrupt Mode
    }

    //GPIO Speed Configuration
    pGPIOHandle->pGPIOx->OSPEEDR &= ~(0x3 << (pGPIOHandle->PinConfig.PinNumber * 2));
    pGPIOHandle->pGPIOx->OSPEEDR |= (pGPIOHandle->PinConfig.PinSpeed << (pGPIOHandle->PinConfig.PinNumber * 2));

    //GPIO Output Type Confuratiın
    pGPIOHandle->pGPIOx->OTYPER &= ~(0x1 << (pGPIOHandle->PinConfig.PinOPType));
    pGPIOHandle->pGPIOx->OTYPER |= (pGPIOHandle->PinConfig.PinOPType << pGPIOHandle->PinConfig.PinNumber);

    //GPIO Pull-Up/Pull-Down Control Configuration  
    pGPIOHandle->pGPIOx->PUPDR &= ~(0x3 << (pGPIOHandle->PinConfig.PinNumber * 2));
    pGPIOHandle->pGPIOx->PUPDR |= (pGPIOHandle->PinConfig.PinPuPdControl << (pGPIOHandle->PinConfig.PinNumber * 2));
    
    if(pGPIOHandle->PinConfig.PinMode == GPIO_MODE_ALTERNATE_FUNCTION)
    {
        //GPIO Alternate Function Configuration
        uint8_t afr_index = 0;
        uint8_t afr_pos   = 0;

        afr_index = pGPIOHandle->PinConfig.PinNumber / 8;
        afr_pos   = pGPIOHandle->PinConfig.PinNumber % 8;

        pGPIOHandle->pGPIOx->AFR[afr_index] &= ~(0xF << (afr_pos * 4));
        pGPIOHandle->pGPIOx->AFR[afr_index] |= (pGPIOHandle->PinConfig.PinAltFunMode << (afr_pos * 4));
    }

    return GPIO_OK;
}