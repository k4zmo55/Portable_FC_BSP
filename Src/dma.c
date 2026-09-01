#include "dma.h"

/* LISR/HISR'daki 4 stream grubunun her birinin 6-bitlik alaninin
 * basladigi bit pozisyonu -- donanim bunu dogrusal (stream*6) degil,
 * bu sirayla diziyor (RM0090 DMA_LISR/HISR aciklamasi). Stream numarasi
 * 0-3 icin LISR, 4-7 icin HISR kullanilir; her ikisinde de grup indeksi
 * (StreamNumber % 4) ile bu tabloya bakilir. */
static const uint8_t DMA_StreamGroupBitOffset[4] = {0, 6, 16, 22};

static uint8_t DMA_GetFlagBitPosition(DMA_Handle_t *pDMAHandle, uint8_t FlagName)
{
    uint8_t groupIndex = pDMAHandle->StreamNumber % 4;
    return DMA_StreamGroupBitOffset[groupIndex] + FlagName;
}

void DMA_PeriClockControl(DMAx_RegDef_t *pDMAx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        if(pDMAx == DMA1)      { DMA1_PCLK_EN(); }
        else if(pDMAx == DMA2) { DMA2_PCLK_EN(); }
    }
    else
    {
        if(pDMAx == DMA1)      { DMA1_PCLK_DI(); }
        else if(pDMAx == DMA2) { DMA2_PCLK_DI(); }
    }
}

void DMA_Stop(DMA_Handle_t *pDMAHandle)
{
    /* RM0090: stream calisirken CR/NDTR/PAR/M0AR degistirilemez -- SPI'de
     * CR1 yazmadan once SPE=0 sartina birebir karsilik gelir. EN biti
     * donanim tarafindan gercekten 0'a dusene kadar beklenmeli. */
    pDMAHandle->pDMAStream->CR &= ~(1 << DMA_SxCR_EN);
    while(pDMAHandle->pDMAStream->CR & (1 << DMA_SxCR_EN));
}

Status_t DMA_Init(DMA_Handle_t *pDMAHandle)
{
    if(pDMAHandle == NULL || pDMAHandle->pDMAx == NULL || pDMAHandle->pDMAStream == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(pDMAHandle->StreamNumber > 7)
    {
        return STATUS_INVALID_PARAM;
    }

    /* Circular/MemInc/PeriphInc sadece ENABLE/DISABLE degeri alabilir --
     * baska bir deger (orn. yanlislikla yazilmis bir enum/makro) CR
     * register'ina rastgele bit olarak sizmadan burada yakalanir. */
    if((pDMAHandle->dma_config.Circular  != ENABLE && pDMAHandle->dma_config.Circular  != DISABLE) ||
       (pDMAHandle->dma_config.MemInc    != ENABLE && pDMAHandle->dma_config.MemInc    != DISABLE) ||
       (pDMAHandle->dma_config.PeriphInc != ENABLE && pDMAHandle->dma_config.PeriphInc != DISABLE))
    {
        return STATUS_INVALID_PARAM;
    }

    DMA_Stop(pDMAHandle);

    uint32_t tempreg = 0;

    tempreg |= ((uint32_t)pDMAHandle->dma_config.Channel  << DMA_SxCR_CHSEL);
    tempreg |= ((uint32_t)pDMAHandle->dma_config.Direction << DMA_SxCR_DIR);
    tempreg |= ((uint32_t)pDMAHandle->dma_config.DataSize  << DMA_SxCR_PSIZE);
    tempreg |= ((uint32_t)pDMAHandle->dma_config.DataSize  << DMA_SxCR_MSIZE);
    tempreg |= ((uint32_t)pDMAHandle->dma_config.Priority  << DMA_SxCR_PL);

    if(pDMAHandle->dma_config.Circular == ENABLE)
    {
        tempreg |= (1 << DMA_SxCR_CIRC);
    }

    if(pDMAHandle->dma_config.MemInc == ENABLE)
    {
        tempreg |= (1 << DMA_SxCR_MINC);
    }

    if(pDMAHandle->dma_config.PeriphInc == ENABLE)
    {
        tempreg |= (1 << DMA_SxCR_PINC);
    }

    /* Transfer tamamlanma / hata kesmeleri varsayilan acik -- NVIC tarafinda
     * ilgili DMAx_StreamY_IRQn ayrica NVIC_IRQInterruptConfig ile
     * etkinlestirilmedigi surece bu bitler CPU'ya ulasmaz, zararsizdir. */
    tempreg |= (1 << DMA_SxCR_TCIE);
    tempreg |= (1 << DMA_SxCR_TEIE);

    pDMAHandle->pDMAStream->CR = tempreg;

    /* Direct mod: FIFO devre disi (DMDIS=0). PSIZE=MSIZE oldugu icin
     * (tek DataSize alani) FIFO'ya hic ihtiyac yok. */
    pDMAHandle->pDMAStream->FCR = 0;

    return STATUS_OK;
}

Status_t DMA_Start(DMA_Handle_t *pDMAHandle, uint32_t PeriphAddr, uint32_t MemAddr, uint32_t length)
{
    if(pDMAHandle == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    DMA_Stop(pDMAHandle);

    /* Onceki bir transferden kalmis olabilecek butun bayraklari temizle --
     * temizlenmezse yeni transfer TC/TE kesmesi hemen, yanlissa da hicbir
     * zaman tetiklenmemis gibi gorunebilir. */
    DMA_ClearFlag(pDMAHandle, DMA_FLAG_TCIF);
    DMA_ClearFlag(pDMAHandle, DMA_FLAG_HTIF);
    DMA_ClearFlag(pDMAHandle, DMA_FLAG_TEIF);
    DMA_ClearFlag(pDMAHandle, DMA_FLAG_DMEIF);
    DMA_ClearFlag(pDMAHandle, DMA_FLAG_FEIF);

    /* PAR her zaman periferik tarafi, M0AR her zaman bellek tarafidir --
     * DIR biti sadece akis yonunu belirler, hangi register'in "kaynak"
     * hangisinin "hedef" oldugunu degistirmez. */
    pDMAHandle->pDMAStream->PAR  = PeriphAddr;
    pDMAHandle->pDMAStream->M0AR = MemAddr;
    pDMAHandle->pDMAStream->NDTR = length;

    pDMAHandle->pDMAStream->CR |= (1 << DMA_SxCR_EN);

    return STATUS_OK;
}

Status_t DMA_GetFlagStatus(DMA_Handle_t *pDMAHandle, uint8_t FlagName)
{
    uint8_t bitPos = DMA_GetFlagBitPosition(pDMAHandle, FlagName);
    uint32_t sr = (pDMAHandle->StreamNumber < 4) ? pDMAHandle->pDMAx->LISR : pDMAHandle->pDMAx->HISR;

    if(sr & (1 << bitPos))
    {
        return STATUS_OK;
    }

    return STATUS_BUSY;
}

void DMA_ClearFlag(DMA_Handle_t *pDMAHandle, uint8_t FlagName)
{
    uint8_t bitPos = DMA_GetFlagBitPosition(pDMAHandle, FlagName);

    if(pDMAHandle->StreamNumber < 4)
    {
        pDMAHandle->pDMAx->LIFCR = (1 << bitPos);
    }
    else
    {
        pDMAHandle->pDMAx->HIFCR = (1 << bitPos);
    }
}

void DMA_IRQHandling(DMA_Handle_t *pDMAHandle)
{
    if(DMA_GetFlagStatus(pDMAHandle, DMA_FLAG_TCIF) == STATUS_OK)
    {
        DMA_ClearFlag(pDMAHandle, DMA_FLAG_TCIF);
        DMA_ApplicationEventCallback(pDMAHandle, DMA_EVENT_TRANSFER_COMPLETE);
    }

    if(DMA_GetFlagStatus(pDMAHandle, DMA_FLAG_TEIF) == STATUS_OK)
    {
        DMA_ClearFlag(pDMAHandle, DMA_FLAG_TEIF);
        DMA_ApplicationEventCallback(pDMAHandle, DMA_EVENT_TRANSFER_ERROR);
    }
}

__attribute__((weak)) void DMA_ApplicationEventCallback(DMA_Handle_t *pDMAHandle, uint8_t AppEv)
{
    (void)pDMAHandle;
    (void)AppEv;
}
