#ifndef FC_NVIC_H
#define FC_NVIC_H

#include "stm32f4xx.h"

/* Cortex-M NVIC kesme yapilandirmasi. Bu, herhangi bir periferige ozgu
 * degildir -- sadece IRQn_Type numarasina gore ISER/ICER/IP register'larina
 * yazar. GPIO, SPI ve ileride eklenecek diger periferik suruculeri (I2C,
 * USART, DMA...) IRQ etkinlestirme/oncelik ayarlarini buradan yapmali;
 * her surucu icinde ayni NVIC mantigini tekrar yazmaktan kacinmak icindir. */

void NVIC_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi);
void NVIC_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority);

#endif /* FC_NVIC_H */
