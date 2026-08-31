#ifndef FC_DMA_H
#define FC_DMA_H

#include "stm32f4xx.h"
#include <stdint.h>

/* Periferikten bagimsiz DMA katmani -- SPI, ileride I2C/UART DMA transferi
 * de bunu kullanabilir (bkz. nvic.c'nin GPIO/SPI arasinda paylasilmasiyla
 * ayni mantik). Kesme yapilandirmasi icin ayri bir DMA_IRQ* wrapper'i
 * eklenmedi; cagiran kod dogrudan nvic.h'daki NVIC_IRQInterruptConfig /
 * NVIC_IRQPriorityConfig'i kullanir (bkz. Docs/DMA.md acik madde notu). */

typedef struct{
    uint8_t Channel;    // 0-7, donanimsal istek eslemesi RM0090 Tablo 42/43 -- Refer @DMA_Channel
    uint8_t Direction;  // Refer @DMA_Direction
    uint8_t Circular;   // ENABLE: NDTR sifirlaninca transfer otomatik yeniden baslar. Refer @DMA_Circular
    uint8_t MemInc;     // ENABLE: her transferde bellek adresi artar. Refer @DMA_MemInc
    uint8_t PeriphInc;  // Genelde DISABLE (periferik adresi hep DR). Refer @DMA_PeriphInc
    uint8_t DataSize;   // PSIZE ve MSIZE icin ortak deger. Refer @DMA_DataSize
    uint8_t Priority;   // Refer @DMA_Priority
}DMA_Config_t;

typedef struct{
    DMAx_RegDef_t        *pDMAx;        // DMA1 ya da DMA2 -- LISR/HISR/LIFCR/HIFCR erisimi icin
    DMA_Stream_RegDef_t  *pDMAStream;   // Ilgili stream (CR/NDTR/PAR/M0AR/FCR erisimi icin)
    uint8_t                StreamNumber; // 0-7, bayrak bit pozisyonu hesaplamasi icin gerekli
    DMA_Config_t            dma_config;
}DMA_Handle_t;

/* @DMA_Direction -- SPI_CR1_MSTR gibi degil, register alanina birebir eslesir */
#define DMA_DIR_PERIPH_TO_MEM  0x0
#define DMA_DIR_MEM_TO_PERIPH  0x1
#define DMA_DIR_MEM_TO_MEM     0x2

/* @DMA_DataSize */
typedef enum { DMA_DATA_SIZE_BYTE = 0x0, DMA_DATA_SIZE_HALFWORD, DMA_DATA_SIZE_WORD } DMA_DataSize_t;

/* @DMA_Priority */
typedef enum { DMA_PRIORITY_LOW = 0x0, DMA_PRIORITY_MEDIUM, DMA_PRIORITY_HIGH, DMA_PRIORITY_VERY_HIGH } DMA_Priority_t;

/* SPI_ApplicationEventCallback ile ayni desen */
#define DMA_EVENT_TRANSFER_COMPLETE  1
#define DMA_EVENT_TRANSFER_ERROR     2

/* @DMA_Flag -- LISR/HISR icindeki bir stream grubunun 6 bitlik alani
 * icinde goreli konum (bit1 donanimda rezerve, bilerek atlaniyor) */
#define DMA_FLAG_FEIF   0
#define DMA_FLAG_DMEIF  2
#define DMA_FLAG_TEIF   3
#define DMA_FLAG_HTIF   4
#define DMA_FLAG_TCIF   5

void     DMA_PeriClockControl(DMAx_RegDef_t *pDMAx, uint8_t EnOrDi);

Status_t DMA_Init(DMA_Handle_t *pDMAHandle);
Status_t DMA_Start(DMA_Handle_t *pDMAHandle, uint32_t PeriphAddr, uint32_t MemAddr, uint32_t length);
void     DMA_Stop(DMA_Handle_t *pDMAHandle);

Status_t DMA_GetFlagStatus(DMA_Handle_t *pDMAHandle, uint8_t FlagName);
void     DMA_ClearFlag(DMA_Handle_t *pDMAHandle, uint8_t FlagName);

void     DMA_IRQHandling(DMA_Handle_t *pDMAHandle);

/* Zayif (weak) varsayilan implementasyon -- SPI_ApplicationEventCallback ile
 * ayni desen: uygulama kodu ayni imzayla override edebilir. */
void     DMA_ApplicationEventCallback(DMA_Handle_t *pDMAHandle, uint8_t AppEv);

#endif /* FC_DMA_H */
