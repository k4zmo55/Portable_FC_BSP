#include "nvic.h"

void NVIC_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi)
{
    uint8_t index = IRQNumber / 32;
    uint8_t pos = IRQNumber % 32;

    if(EnOrDi == ENABLE)
    {
        NVIC->ISER[index] = (1 << pos);
    }
    else
    {
        NVIC->ICER[index] = (1 << pos);
    }
}

void NVIC_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority)
{
    NVIC->IP[IRQNumber] = (uint8_t)(IRQPriority << 4);
}
