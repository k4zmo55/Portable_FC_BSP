#include "fc_drivers.h"
#include <inttypes.h>
#include <stdint.h>

static void SPI_ClearOVRFlag(SPIx_RegDef_t *pSPIx);

void SPI_PeriClockControl(SPIx_RegDef_t *pSPIx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        if(pSPIx == SPI1)
        {
            SPI1_PCLK_EN();
        }
        else if(pSPIx == SPI2)
        {
            SPI2_PCLK_EN();
        }
        else if(pSPIx == SPI3)
        {
            SPI3_PCLK_EN();
        }
    }
    else{
        if(pSPIx == SPI1)
        {
            SPI1_PCLK_DI();
        }
        else if(pSPIx == SPI2)
        {
            SPI2_PCLK_DI();
        }
        else if(pSPIx == SPI3)
        {
            SPI3_PCLK_DI();
        }
    }
}

void SPI_DeInit(SPIx_RegDef_t *pSPIx)
{
    if(pSPIx == SPI1)      { SPI1_REG_RESET(); }
    else if(pSPIx == SPI2) { SPI2_REG_RESET(); }
    else if(pSPIx == SPI3) { SPI3_REG_RESET(); }
}

Status_t SPI_Init(SPI_Handle_t *spi_handle)
{
    /* --- 1. Handle doğrulama --- */
    if (spi_handle == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if (!IS_SPI(spi_handle->pSPIx))
    {
        return STATUS_INVALID_PARAM;
    }

    /* --- 2. Konfigürasyon doğrulama (donanıma dokunmadan) --- */
    if (!IS_BUS_CONFIG(spi_handle->spi_config.BusConfig) ||
        !IS_CPHA(spi_handle->spi_config.CPHA)            ||
        !IS_CPOL(spi_handle->spi_config.CPOL)            ||
        !IS_DEVICE_MODE(spi_handle->spi_config.DeviceMode) ||
        !IS_DFF(spi_handle->spi_config.DFF))
    {
        return STATUS_INVALID_PARAM;
    }

    /* --- 3. Çevre birimi clock'unu aç --- */
    SPI_PeriClockControl(spi_handle->pSPIx, ENABLE);

    /* --- 4. SPE = 0 olduğundan emin ol --- */
    /* CR1 yazmadan önce çevre birimi kapalı olmalı */
    spi_handle->pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);

    /* --- 5. CR1'i kur --- */
    /* uint32_t tempreg = 0;
     *   DeviceMode -> MSTR
     *   BusConfig  -> BIDIMODE / RXONLY
     *   SclkSpeed  -> BR[2:0]
     *   DFF        -> DFF
     *   CPOL, CPHA -> CPOL, CPHA
     *   SSM        -> SSM / SSI
     * spi_handle->pSPIx->CR1 = tempreg;   (|= değil, = )
     */
    uint32_t tempreg = 0;

    /* Configure the SPI Device Mode*/
    tempreg |= (spi_handle->spi_config.DeviceMode << SPI_CR1_MSTR);

    /* Configure the Bus Config*/
    if(spi_handle->spi_config.BusConfig == FULL_DUPLEX)
    {
        // BIDIMODE Should be Cleared
        tempreg &= ~(1 << SPI_CR1_BIDIMODE);
    }
    else if(spi_handle->spi_config.BusConfig == HALF_DUPLEX)
    {
        //BIDIMODE Should be Set
        tempreg |= (1 << SPI_CR1_BIDIMODE);
    }
    else if(spi_handle->spi_config.BusConfig == SIMPLE_RXONLY)
    {
        //2 hatli unidirectional, sadece alma: BIDIMODE=0, RXONLY=1
        //(MISO+MOSI/SCK aktif kalir, donanim TX'e gerek kalmadan surekli saat uretir)
        tempreg &= ~(1 << SPI_CR1_BIDIMODE);
        tempreg |= (1 << SPI_CR1_RXONLY);
    }

    /* Configure the Serial Clock Speed */
    tempreg |= (spi_handle->spi_config.SclkSpeed << SPI_CR1_BR);

    /* Configure the Data Frame Format */
    tempreg |= (spi_handle->spi_config.DFF << SPI_CR1_DFF);

    /* Configure the CPOL */
    tempreg |= (spi_handle->spi_config.CPOL << SPI_CR1_CPOL);

    /* Configure the CPHA */
    tempreg |= (spi_handle->spi_config.CPHA << SPI_CR1_CPHA);

    /* Configure the Software Slave Management */
    tempreg |= (spi_handle->spi_config.SSM << SPI_CR1_SSM);

    /* SSM etkinse NSS donanim pinini yok sayar; SSI=1 verilmezse
     * master modda NSS "dusuk" gibi okunur ve MODF hatasi olusur. */
    if(spi_handle->spi_config.SSM == SSM_EN)
    {
        tempreg |= (1 << SPI_CR1_SSI);
    }

    spi_handle->pSPIx->CR1 = tempreg;

    return STATUS_OK;
}

void SPI_PeripheralControl(SPIx_RegDef_t *pSPIx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        pSPIx->CR1 |= (1 << SPI_CR1_SPE);
    }
    else
    {
        pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
    }
}

Status_t SPI_Send(SPIx_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t length)
{
    if(pSPIx == NULL || pTxBuffer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(!IS_SPI(pSPIx))
    {
        return STATUS_INVALID_PARAM;
    }

    if(length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    while(length > 0)
    {
        while(SPI_GetFlagStatus(pSPIx, SPI_SR_TXE) != STATUS_OK);


        if((pSPIx->CR1 & (1 << SPI_CR1_DFF)) != 0)
        {
            //Data frame format 16-bit
            pSPIx->DR = *((uint16_t*)pTxBuffer);
            pTxBuffer +=2;
            length -=2;
        }

        else
        {
            //Data frame format 8-bit
            pSPIx->DR = *pTxBuffer;
            pTxBuffer++;
            length--;
        }
    }

    return STATUS_OK;
}

Status_t SPI_Receive(SPIx_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t length)
{
    if(pSPIx == NULL || pRxBuffer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(!IS_SPI(pSPIx))
    {
        return STATUS_INVALID_PARAM;
    }

    if(length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    while( length > 0 )
    {
        while(SPI_GetFlagStatus(pSPIx, SPI_SR_RXNE) != STATUS_OK);

        if(SPI_GetFlagStatus(pSPIx, SPI_SR_OVR) == STATUS_OK)
        {
            SPI_ClearOVRFlag(pSPIx);
            return STATUS_OVERRUN;
        }

        if( (pSPIx->CR1 & (1 << SPI_CR1_DFF)) != 0 )
        {
            //Data frame format 16-bit
            *((uint16_t*)pRxBuffer) = (uint16_t)pSPIx->DR;
            pRxBuffer +=2;
            length -=2;
        }
        else
        {
            //Data frame format 8-bit
            *pRxBuffer = (uint8_t)pSPIx->DR;
            pRxBuffer++;
            length--;
        }
        
    }

    return STATUS_OK;

}

Status_t SPI_TransmitReceive(SPIx_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t length)
{
    if(pSPIx == NULL || pTxBuffer == NULL || pRxBuffer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(!IS_SPI(pSPIx))
    {
        return STATUS_INVALID_PARAM;
    }

    if(length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    while(length > 0)
    {
        if( (pSPIx->CR1 & (1 << SPI_CR1_DFF)) == 0)
        {
            //8-Bit
            while(SPI_GetFlagStatus(pSPIx, SPI_SR_TXE) != STATUS_OK);
            pSPIx->DR = *pTxBuffer;
            pTxBuffer++;

            while(SPI_GetFlagStatus(pSPIx, SPI_SR_RXNE) != STATUS_OK);

            if(SPI_GetFlagStatus(pSPIx, SPI_SR_OVR) == STATUS_OK)
            {
                SPI_ClearOVRFlag(pSPIx);
                return STATUS_OVERRUN;
            }

            *pRxBuffer = (uint8_t)pSPIx->DR;
            pRxBuffer++;

            length--;    
        }
        else
        {
            //16-bit
            while(SPI_GetFlagStatus(pSPIx, SPI_SR_TXE) != STATUS_OK);
            pSPIx->DR = *((uint16_t*)pTxBuffer);
            pTxBuffer +=2;
            
            while(SPI_GetFlagStatus(pSPIx, SPI_SR_RXNE) != STATUS_OK);

            if(SPI_GetFlagStatus(pSPIx, SPI_SR_OVR) == STATUS_OK)
            {
                SPI_ClearOVRFlag(pSPIx);
                return STATUS_OVERRUN;
            }

            *((uint16_t*)pRxBuffer) = (uint16_t)pSPIx->DR;
            pRxBuffer+=2;

            length -=2;
        }
        
    }

    return STATUS_OK;
}

Status_t SPI_GetFlagStatus(SPIx_RegDef_t *pSPIx, uint32_t FlagName)
{
    if(pSPIx->SR & (1 << FlagName))
    {
        return STATUS_OK;
    }

    return STATUS_BUSY;
}

static void SPI_ClearOVRFlag(SPIx_RegDef_t *pSPIx)
{
    volatile uint32_t temp;
    temp = pSPIx->DR;
    temp = pSPIx->SR;
    (void)temp;
}

/* ========================================================================= */
/*                      INTERRUPT TABANLI GONDER/AL                          */
/* ========================================================================= */

static void SPI_TXE_Interrupt_Handle(SPI_Handle_t *pSPIHandle)
{
    if((pSPIHandle->pSPIx->CR1 & (1 << SPI_CR1_DFF)) != 0)
    {
        pSPIHandle->pSPIx->DR = *((uint16_t*)pSPIHandle->pTxBuffer);
        pSPIHandle->pTxBuffer += 2;
        pSPIHandle->TxLen -= 2;
    }
    else
    {
        pSPIHandle->pSPIx->DR = *pSPIHandle->pTxBuffer;
        pSPIHandle->pTxBuffer++;
        pSPIHandle->TxLen--;
    }

    if(pSPIHandle->TxLen == 0)
    {
        SPI_CloseTransmission(pSPIHandle);
        SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
    }
}

static void SPI_RXNE_Interrupt_Handle(SPI_Handle_t *pSPIHandle)
{
    if((pSPIHandle->pSPIx->CR1 & (1 << SPI_CR1_DFF)) != 0)
    {
        *((uint16_t*)pSPIHandle->pRxBuffer) = (uint16_t)pSPIHandle->pSPIx->DR;
        pSPIHandle->pRxBuffer += 2;
        pSPIHandle->RxLen -= 2;
    }
    else
    {
        *(pSPIHandle->pRxBuffer) = (uint8_t)pSPIHandle->pSPIx->DR;
        pSPIHandle->pRxBuffer++;
        pSPIHandle->RxLen--;
    }

    if(pSPIHandle->RxLen == 0)
    {
        SPI_CloseReception(pSPIHandle);
        SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
    }
}

static void SPI_OVR_Interrupt_Handle(SPI_Handle_t *pSPIHandle)
{
    /* RM0090: TX devam ederken OVR temizlemek icin DR okumak, henuz
     * gonderilmemis veriyi kaybettirir -- sadece TX mesgul degilse temizle. */
    if(pSPIHandle->TxState != SPI_BUSY_IN_TX)
    {
        SPI_ClearOVRFlag(pSPIHandle->pSPIx);
    }

    SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_OVR_ERR);
}

Status_t SPI_SendIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t length)
{
    if(pSPIHandle == NULL || pTxBuffer == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(pSPIHandle->TxState == SPI_BUSY_IN_TX)
    {
        return STATUS_BUSY;
    }

    pSPIHandle->pTxBuffer = pTxBuffer;
    pSPIHandle->TxLen = length;
    pSPIHandle->TxState = SPI_BUSY_IN_TX;

    //Asil gonderme SPI_IRQHandling() icinde, TXE kesmesi geldikce yapilir
    pSPIHandle->pSPIx->CR2 |= (1 << SPI_CR2_TXEIE);

    return STATUS_OK;
}

Status_t SPI_ReceiveIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t length)
{
    if(pSPIHandle == NULL || pRxBuffer == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(pSPIHandle->RxState == SPI_BUSY_IN_RX)
    {
        return STATUS_BUSY;
    }

    pSPIHandle->pRxBuffer = pRxBuffer;
    pSPIHandle->RxLen = length;
    pSPIHandle->RxState = SPI_BUSY_IN_RX;

    //FULL_DUPLEX'te saat uretimi icin TXE de acilmali (bkz. SPI_Receive notu);
    //SIMPLE_RXONLY'de donanim zaten sureklidir, RXNEIE tek basina yeterlidir.
    pSPIHandle->pSPIx->CR2 |= (1 << SPI_CR2_RXNEIE);

    return STATUS_OK;
}

void SPI_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi)
{
    NVIC_IRQInterruptConfig(IRQNumber, EnOrDi);
}

void SPI_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority)
{
    NVIC_IRQPriorityConfig(IRQNumber, IRQPriority);
}

void SPI_IRQHandling(SPI_Handle_t *pSPIHandle)
{
    uint32_t sr  = pSPIHandle->pSPIx->SR;
    uint32_t cr2 = pSPIHandle->pSPIx->CR2;

    if((sr & (1 << SPI_SR_TXE)) && (cr2 & (1 << SPI_CR2_TXEIE)))
    {
        SPI_TXE_Interrupt_Handle(pSPIHandle);
    }

    if((sr & (1 << SPI_SR_RXNE)) && (cr2 & (1 << SPI_CR2_RXNEIE)))
    {
        SPI_RXNE_Interrupt_Handle(pSPIHandle);
    }

    if((sr & (1 << SPI_SR_OVR)) && (cr2 & (1 << SPI_CR2_ERRIE)))
    {
        SPI_OVR_Interrupt_Handle(pSPIHandle);
    }
}

void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle)
{
    pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_TXEIE);
    pSPIHandle->pTxBuffer = NULL;
    pSPIHandle->TxLen = 0;
    pSPIHandle->TxState = SPI_READY;
}

void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
    pSPIHandle->pSPIx->CR2 &= ~(1 << SPI_CR2_RXNEIE);
    pSPIHandle->pRxBuffer = NULL;
    pSPIHandle->RxLen = 0;
    pSPIHandle->RxState = SPI_READY;
}

__attribute__((weak)) void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv)
{
    /* Uygulama kodu ayni imzayla kendi fonksiyonunu tanimlayip bunu
     * override edebilir; varsayilan implementasyon kasitli olarak bos. */
    (void)pSPIHandle;
    (void)AppEv;
}

/* ========================================================================= */
/*                          DMA TABANLI GONDER/AL                            */
/* ========================================================================= */

Status_t SPI_SendDMA(SPI_Handle_t *pSPIHandle, DMA_Handle_t *pDMAHandle, uint8_t *pTxBuffer, uint32_t length)
{
    if(pSPIHandle == NULL || pDMAHandle == NULL || pTxBuffer == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    pSPIHandle->pSPIx->CR2 |= (1 << SPI_CR2_TXDMAEN);

    //PAR = SPI'nin DR'si (hedef), M0AR = pTxBuffer (kaynak), DIR pDMAHandle'da MEM_TO_PERIPH olmali
    return DMA_Start(pDMAHandle, (uint32_t)&pSPIHandle->pSPIx->DR, (uint32_t)pTxBuffer, length);
}

Status_t SPI_ReceiveDMA(SPI_Handle_t *pSPIHandle, DMA_Handle_t *pDMAHandle, uint8_t *pRxBuffer, uint32_t length)
{
    if(pSPIHandle == NULL || pDMAHandle == NULL || pRxBuffer == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    pSPIHandle->pSPIx->CR2 |= (1 << SPI_CR2_RXDMAEN);

    //PAR = SPI'nin DR'si (kaynak), M0AR = pRxBuffer (hedef), DIR pDMAHandle'da PERIPH_TO_MEM olmali
    return DMA_Start(pDMAHandle, (uint32_t)&pSPIHandle->pSPIx->DR, (uint32_t)pRxBuffer, length);
}