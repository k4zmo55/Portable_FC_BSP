#include "gpio.h"
#include "stm32f4xx.h"
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

        //Rising Edge Detection
        if(pGPIOHandle->PinConfig.PinMode == GPIO_MODE_IT_RT)
        {
            EXTI->FTSR &= ~(1 << pGPIOHandle->PinConfig.PinNumber);
            EXTI->RTSR |= (1 << pGPIOHandle->PinConfig.PinNumber);
        }
        //Falling Edge Detection
        else if(pGPIOHandle->PinConfig.PinMode == GPIO_MODE_IT_FT)
        {
            EXTI->RTSR &= ~(1 << pGPIOHandle->PinConfig.PinNumber);
            EXTI->FTSR |= (1 << pGPIOHandle->PinConfig.PinNumber);
        }
        //Rising and Falling Edges Detection
        else if(pGPIOHandle->PinConfig.PinMode == GPIO_MODE_IT_RFT)
        {
            EXTI->FTSR |= (1 << pGPIOHandle->PinConfig.PinNumber);
            EXTI->RTSR |= (1 << pGPIOHandle->PinConfig.PinNumber);
        }

        uint8_t exti_index = (pGPIOHandle->PinConfig.PinNumber / 4);
        uint8_t exti_pos = (pGPIOHandle->PinConfig.PinNumber % 4);

        uint8_t port_code = GPIO_BASEADDR_TO_CODE(pGPIOHandle->pGPIOx);
        SYSCFG_PCLK_EN();
        SYSCFG->EXTICR[exti_index] &= ~(0xF << (exti_pos * 4));
        SYSCFG->EXTICR[exti_index] |= (port_code << (exti_pos * 4));
        EXTI->IMR |= (1 << pGPIOHandle->PinConfig.PinNumber);
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

void GPIO_DeInit(GPIOx_RegDef_t *pGPIOx)
{
    if(pGPIOx == GPIOA) 
    { 
        GPIOA_REG_RESET();
    }
    else if(pGPIOx == GPIOB)
    {
        GPIOB_REG_RESET();
    }
    else if(pGPIOx == GPIOC)
    {
        GPIOC_REG_RESET();
    }
    else if(pGPIOx == GPIOD)
    {
        GPIOD_REG_RESET();
    }
    else if(pGPIOx == GPIOE)
    {
        GPIOE_REG_RESET();
    }
    else if(pGPIOx == GPIOF)
    {
        GPIOF_REG_RESET();
    }
    else if(pGPIOx == GPIOG)
    {
        GPIOG_REG_RESET();
    }
    else if(pGPIOx == GPIOH)
    {
        GPIOH_REG_RESET();
    }
    else if(pGPIOx == GPIOI)
    {
        GPIOI_REG_RESET();
    }

}

GPIO_Status_t GPIO_ReadFromPin(GPIOx_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    if(!IS_GPIO(pGPIOx) || !IS_GPIO_PIN(PinNumber))
    {
        return GPIO_ERROR;
    }

    uint8_t value = 0;
    value = (uint8_t)((pGPIOx->IDR >> PinNumber) & 0x00000001);
    return value;
}

uint16_t GPIO_ReadFromPort(GPIOx_RegDef_t *pGPIOx)
{
    uint16_t value = 0;
    value = (uint16_t)pGPIOx->IDR;
    return value;
}

GPIO_Status_t GPIO_WriteToPin(GPIOx_RegDef_t *pGPIOx, uint8_t PinNumber, GPIO_PinState_t PinState)
{
    if(!IS_GPIO(pGPIOx) || !IS_GPIO_PIN_STATE(PinState) || ! IS_GPIO_PIN(PinNumber))
    {
        return GPIO_ERROR;
    }

    if(PinState == ENABLE)
    {
        pGPIOx->ODR |= (1 << PinNumber);
    }
    else
    {
        pGPIOx->ODR &= ~(1 << PinNumber);
    }

    return GPIO_OK;
}

GPIO_Status_t GPIO_WriteToPort(GPIOx_RegDef_t *pGPIOx, uint16_t Value)
{
    if(!IS_GPIO(pGPIOx) || !IS_GPIO_PIN_STATE(Value))
    {
        return GPIO_ERROR;
    }

    pGPIOx->ODR = Value;

    return GPIO_OK;
}

void GPIO_TogglePin(GPIOx_RegDef_t *pGPIOx, uint8_t PinNumber)
{
    pGPIOx->ODR ^= (1 << PinNumber);
}

void GPIO_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi)
{
    uint8_t index = IRQNumber / 32;
    uint8_t pos = IRQNumber % 32;

    if(EnOrDi == ENABLE)
    {
        NVIC->ISER[index] = (1 << pos);
    }
    else {
        NVIC->ICER[index] = (1 << pos);
    }
}

void   GPIO_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority)
{
    NVIC->IP[IRQNumber] = (uint8_t)(IRQPriority << 4);
}

void GPIO_IRQHandling(uint8_t PinNumber)
{
    if(EXTI->PR & (1U << PinNumber))
    {
        EXTI->PR |= (1U << PinNumber); //Clear
    }
}