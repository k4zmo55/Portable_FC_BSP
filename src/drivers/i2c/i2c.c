#include "fc_drivers.h"
#include <inttypes.h>
#include <stdint.h>

void I2C_PeriClockControl(I2Cx_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        if(pI2Cx == I2C1)
        {
            I2C1_PCLK_EN();
        }
        else if(pI2Cx == I2C2)
        {
            I2C2_PCLK_EN();
        }
        else if(pI2Cx == I2C3)
        {
            I2C3_PCLK_EN();
        }
    }
    else {

        if(pI2Cx == I2C1)
        {
            I2C1_PCLK_DI();
        }
        else if(pI2Cx == I2C2)
        {
            I2C2_PCLK_DI();
        }
        else if(pI2Cx == I2C3)
        {
            I2C3_PCLK_DI();
        }
    }
}

void I2C_PeripheralControl(I2Cx_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        pI2Cx->CR1 |= (1 << I2C_CR1_PE);
    }
    else {
        pI2Cx->CR1 &= ~(1 << I2C_CR1_PE);
    }
}


Status_t I2C_Init(I2C_Handle_t *pI2CHandle)
{
    uint32_t pclk1_hz = 0;
    uint32_t freq_mhz = 0;
    uint32_t ccr_value = 0;
    uint32_t trise_value = 0;
    uint32_t tempreg = 0;

    if(pI2CHandle == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(!IS_I2C(pI2CHandle->pI2Cx))
    {
        return STATUS_INVALID_PARAM;
    }

    if(!IS_I2C_SCL_SPEED(pI2CHandle->i2c_config.SclSpeed) ||
       !IS_I2C_ADDRESS(pI2CHandle->i2c_config.DeviceAddress) ||
       !IS_I2C_ACK(pI2CHandle->i2c_config.ACKControl) ||
       !IS_I2C_FM_DUTY(pI2CHandle->i2c_config.FMDutyCycle))
    {
        return STATUS_INVALID_PARAM;
    }

    pclk1_hz = RCC_GetPCLK1Value();
    freq_mhz = pclk1_hz / 1000000U;

    if(freq_mhz < 2 || freq_mhz > 50)
    {
        return STATUS_ERROR;
    }

    I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

    // PE=0 kapalı olduğundan emin ol
    pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_PE);

    if(pI2CHandle->i2c_config.ACKControl == I2C_ACK_ENABLE)
    {
        pI2CHandle->pI2Cx->CR1 |= (1 << I2C_CR1_ACK);
    }
    else {
        pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
    }

    pI2CHandle->pI2Cx->CR2 = (pI2CHandle->pI2Cx->CR2 & ~(0x3F << I2C_CR2_FREQ))
                              | (freq_mhz << I2C_CR2_FREQ);

    /* OAR1: 7-bit own adres ADD[7:1]'de baslar (bit0 degil); bit14
     * donanim geregi yazilimca her zaman 1 tutulmali (RM0090) */
    pI2CHandle->pI2Cx->OAR1 = (1 << 14)
                              | (pI2CHandle->i2c_config.DeviceAddress << I2C_OAR1_ADD71);

    if(pI2CHandle->i2c_config.SclSpeed <= I2C_SPEED_STANDARD)
    {
        /* Standard mode: F/S=0, CCR = PCLK1 / (2 * SCL) */
        ccr_value = pclk1_hz / (2U * pI2CHandle->i2c_config.SclSpeed);

        //These bits must be programmed with the maximum SCL rise time given in the I2C bus
        //specification, incremented by 1
        trise_value = freq_mhz + 1U;
    }
    else
    {
        /* Fast mode: F/S=1, DUTY'e gore CCR formulu farkli */
        if(pI2CHandle->i2c_config.FMDutyCycle == I2C_FM_DUTY_2)
        {
            ccr_value = pclk1_hz / (3U * pI2CHandle->i2c_config.SclSpeed);
        }
        else
        {
            ccr_value = pclk1_hz / (25U * pI2CHandle->i2c_config.SclSpeed);
        }

        trise_value = ((freq_mhz * 300U) / 1000U) + 1U;
    }

    /* RM0090: CCR en az 0x04 olmali (hem standart hem fast modda) */
    if(ccr_value < 4U)
    {
        ccr_value = 4U;
    }

    tempreg = (ccr_value & 0xFFF);
    if(pI2CHandle->i2c_config.SclSpeed > I2C_SPEED_STANDARD)
    {
        tempreg |= (1 << I2C_CCR_FS) | (pI2CHandle->i2c_config.FMDutyCycle << I2C_CCR_DUTY);
    }

    pI2CHandle->pI2Cx->CCR   = tempreg;
    pI2CHandle->pI2Cx->TRISE = (trise_value & 0x3F);

    return STATUS_OK;
}

Status_t I2C_GetFlagStatus(I2Cx_RegDef_t *pI2Cx, uint32_t FlagName)
{
    if(pI2Cx->SR1 & (1 << FlagName))
    {
        return STATUS_OK;
    }

    return STATUS_BUSY;
}

void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
    pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);
    pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);

    pI2CHandle->TxRxState = I2C_READY;
    pI2CHandle->pTxBuffer = NULL;
    pI2CHandle->TxLen = 0;
}

void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
    pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);
    pI2CHandle->pI2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);

    pI2CHandle->TxRxState = I2C_READY;
    pI2CHandle->pRxBuffer = NULL;
    pI2CHandle->RxLen = 0;
}

void I2C_ExecuteAddressPhaseWrite(I2Cx_RegDef_t *pI2Cx, uint8_t addr)
{
    pI2Cx->DR = addr;
}

void I2C_Generate_Start_Condition(I2Cx_RegDef_t *pI2Cx)
{
    pI2Cx->CR1 |= (1 << I2C_CR1_START);
}

void I2C_Generate_Stop_Condition(I2Cx_RegDef_t *pI2Cx)
{
    pI2Cx->CR1 |= (1 << I2C_CR1_STOP);
}

/* Master transmit ortak on-fazi: START -> SB bekle -> adres+W(0) gonder ->
 * ADDR bekle -> ADDR temizle (SR1->SR2). I2C_MasterSend ve I2C_MemWrite
 * bu fazi birebir ayni sekilde yasiyor, bu yuzden burada tek yerde. */
static void I2C_MasterAddressPhaseWrite(I2Cx_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
    I2C_Generate_Start_Condition(pI2Cx);
    while(I2C_GetFlagStatus(pI2Cx, I2C_SR1_SB) != STATUS_OK);

    uint8_t addr = (SlaveAddr << 1) & ~(1);  // R/W=0: yazma
    I2C_ExecuteAddressPhaseWrite(pI2Cx, addr);

    while(I2C_GetFlagStatus(pI2Cx, I2C_SR1_ADDR) != STATUS_OK);

    uint32_t dummy = pI2Cx->SR1;
    dummy = pI2Cx->SR2;
    (void)dummy;
}

/* pData'daki length baytini, her biri icin TXE bekleyerek DR'a yazar. */
static void I2C_MasterWriteBytes(I2Cx_RegDef_t *pI2Cx, const uint8_t *pData, uint32_t length)
{
    while(length > 0)
    {
        while(I2C_GetFlagStatus(pI2Cx, I2C_SR1_TXE) != STATUS_OK);

        pI2Cx->DR = *pData;
        pData++;
        length--;
    }
}

/* Son byte fiziksel olarak hatta cikana kadar bekler (TXE + BTF). */
static void I2C_MasterWaitTransferComplete(I2Cx_RegDef_t *pI2Cx)
{
    while(I2C_GetFlagStatus(pI2Cx, I2C_SR1_TXE) != STATUS_OK);
    while(I2C_GetFlagStatus(pI2Cx, I2C_SR1_BTF) != STATUS_OK);
}

Status_t I2C_MasterSend(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr)
{
    if(pI2CHandle == NULL || pTxBuffer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(!IS_I2C_SR(Sr) || !IS_I2C_ADDRESS(SlaveAddr))
    {
        return STATUS_INVALID_PARAM;
    }

    I2C_MasterAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr);
    I2C_MasterWriteBytes(pI2CHandle->pI2Cx, pTxBuffer, length);
    I2C_MasterWaitTransferComplete(pI2CHandle->pI2Cx);

    if(Sr == I2C_DISABLE_SR)
    {
        I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);
    }

    return STATUS_OK;
}

Status_t I2C_MasterReceive(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr)
{
    if(pI2CHandle == NULL || pRxBuffer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(!IS_I2C_ADDRESS(SlaveAddr) || !IS_I2C_SR(Sr))
    {
        return STATUS_INVALID_PARAM;
    }

    I2C_Generate_Start_Condition(pI2CHandle->pI2Cx);

    while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_SB) != STATUS_OK);

    uint8_t addr = (SlaveAddr << 1) | (1);
    I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, addr);

    while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_ADDR) != STATUS_OK);

    /* 1 byte'lik okumada NACK, ADDR temizlenmeden ONCE ayarlanmali (RM0090 6.2) */
    if(length == 1)
    {
        pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
    }

    uint32_t dummy = pI2CHandle->pI2Cx->SR1;
    dummy = pI2CHandle->pI2Cx->SR2;
    (void)dummy;

    if(length == 1 && Sr == I2C_DISABLE_SR)
    {
        I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);
    }

    for(uint32_t i = 0; i < length; i++)
    {
        while(I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_SR1_RXNE) != STATUS_OK);

        if(length > 1 && i == (length - 2))
        {
            /* Sondan bir onceki byte alinirken: ACK=0 ve (Sr kapaliysa) STOP,
             * son byte'in RXNE ile okunmasindan ONCE ayarlanmali (RM0090 6.2) */
            pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);

            if(Sr == I2C_DISABLE_SR)
            {
                I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);
            }
        }

        *pRxBuffer = (uint8_t)pI2CHandle->pI2Cx->DR;
        pRxBuffer++;
    }

    /* Sonraki cagriya temiz baslamak icin ACK'i kullanici konfigurasyonuna gore geri getir */
    if(pI2CHandle->i2c_config.ACKControl == I2C_ACK_ENABLE)
    {
        pI2CHandle->pI2Cx->CR1 |= (1 << I2C_CR1_ACK);
    }

    return STATUS_OK;
}

void I2C_SlaveSendData(I2Cx_RegDef_t *pI2Cx, uint8_t data)
{
    pI2Cx->DR = data;
}

uint8_t I2C_SlaveReceiveData(I2Cx_RegDef_t *pI2Cx)
{
    return (uint8_t)pI2Cx->DR;
}

void I2C_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi)
{
    NVIC_IRQInterruptConfig(IRQNumber, EnOrDi);
}

void I2C_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority)
{
    NVIC_IRQPriorityConfig(IRQNumber, IRQPriority);
}

__attribute__((weak)) void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
    /* Uygulama kodu ayni imzayla kendi fonksiyonunu tanimlayip bunu
     * override edebilir; varsayilan implementasyon kasitli olarak bos. */
    (void)pI2CHandle;
    (void)AppEv;
}

void I2C_SlaveEnableDisableCallbackEvents(I2Cx_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
        pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
        pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
    }
    else
    {
        pI2Cx->CR2 &= ~(1 << I2C_CR2_ITEVTEN);
        pI2Cx->CR2 &= ~(1 << I2C_CR2_ITBUFEN);
        pI2Cx->CR2 &= ~(1 << I2C_CR2_ITERREN);
    }
}

Status_t I2C_MasterSendIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr)
{
    if(pI2CHandle == NULL || pTxBuffer == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(!IS_I2C_SR(Sr) || !IS_I2C_ADDRESS(SlaveAddr))
    {
        return STATUS_INVALID_PARAM;
    }

    if(pI2CHandle->TxRxState != I2C_READY)
    {
        return STATUS_BUSY;
    }

    pI2CHandle->pTxBuffer = pTxBuffer;
    pI2CHandle->TxLen     = length;
    pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
    pI2CHandle->DevAddr   = SlaveAddr;
    pI2CHandle->Sr        = Sr;

    // Asil aktarim I2C_EV_IRQHandling() icinde, kesmeler geldikce yapilir
    I2C_Generate_Start_Condition(pI2CHandle->pI2Cx);

    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);

    return STATUS_OK;
}

Status_t I2C_MasterReceiveIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr)
{
    if(pI2CHandle == NULL || pRxBuffer == NULL || length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(!IS_I2C_SR(Sr) || !IS_I2C_ADDRESS(SlaveAddr))
    {
        return STATUS_INVALID_PARAM;
    }

    if(pI2CHandle->TxRxState != I2C_READY)
    {
        return STATUS_BUSY;
    }

    pI2CHandle->pRxBuffer = pRxBuffer;
    pI2CHandle->RxLen     = length;
    pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
    pI2CHandle->DevAddr   = SlaveAddr;
    pI2CHandle->Sr        = Sr;

    I2C_Generate_Start_Condition(pI2CHandle->pI2Cx);

    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);
    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);
    pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);

    return STATUS_OK;
}

/* ========================================================================= */
/*                 INTERRUPT TABANLI DURUM MAKINESI (EV/ER)                  */
/* ========================================================================= */

static void I2C_SB_Interrupt_Handle(I2C_Handle_t *pI2CHandle)
{
    uint8_t addr;

    if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
    {
        addr = (pI2CHandle->DevAddr << 1) & ~(1);  // R/W=0: yazma
    }
    else
    {
        addr = (pI2CHandle->DevAddr << 1) | 1;      // R/W=1: okuma
    }

    I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, addr);
}

static void I2C_ADDR_Interrupt_Handle(I2C_Handle_t *pI2CHandle)
{
    /* 1 byte'lik okumada NACK, ADDR temizlenmeden ONCE ayarlanmali (RM0090
     * 6.2 -- blocking I2C_MasterReceive'deki length==1 daliyla ayni kural).
     * SR1, I2C_EV_IRQHandling icinde zaten okundu; burada sadece SR2'yi
     * okuyup ADDR'i temizliyoruz. */
    if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX && pI2CHandle->RxLen == 1)
    {
        pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
    }

    uint32_t dummy = pI2CHandle->pI2Cx->SR2;
    (void)dummy;

    if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX && pI2CHandle->RxLen == 1 &&
       pI2CHandle->Sr == I2C_DISABLE_SR)
    {
        I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);
    }
}

static void I2C_TXE_Interrupt_Handle(I2C_Handle_t *pI2CHandle)
{
    if(pI2CHandle->TxRxState != I2C_BUSY_IN_TX)
    {
        return;
    }

    if(pI2CHandle->TxLen > 0)
    {
        pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);
        pI2CHandle->pTxBuffer++;
        pI2CHandle->TxLen--;
    }
}

static void I2C_RXNE_Interrupt_Handle(I2C_Handle_t *pI2CHandle)
{
    if(pI2CHandle->TxRxState != I2C_BUSY_IN_RX)
    {
        return;
    }

    /* N>=2 durumunda: RxLen==2 iken (bu byte okunduktan sonra 1 byte
     * kalacak, yani "sondan bir onceki byte") ACK=0 ve STOP -- blocking
     * I2C_MasterReceive'deki i==length-2 kuralinin IT karsiligi. */
    if(pI2CHandle->RxLen == 2)
    {
        pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);

        if(pI2CHandle->Sr == I2C_DISABLE_SR)
        {
            I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);
        }
    }

    *(pI2CHandle->pRxBuffer) = (uint8_t)pI2CHandle->pI2Cx->DR;
    pI2CHandle->pRxBuffer++;
    pI2CHandle->RxLen--;

    if(pI2CHandle->RxLen == 0)
    {
        I2C_CloseReceiveData(pI2CHandle);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_RX_CMPLT);
    }
}

static void I2C_BTF_Interrupt_Handle(I2C_Handle_t *pI2CHandle)
{
    if(pI2CHandle->TxRxState != I2C_BUSY_IN_TX)
    {
        return;
    }

    /* TXE de set VE gonderilecek veri kalmadiysa transfer gercekten bitti
     * (blocking I2C_MasterSend'deki "TXE sonra BTF bekle" mantiginin IT
     * karsiligi). */
    if((pI2CHandle->pI2Cx->SR1 & (1 << I2C_SR1_TXE)) && pI2CHandle->TxLen == 0)
    {
        if(pI2CHandle->Sr == I2C_DISABLE_SR)
        {
            I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);
        }

        I2C_CloseSendData(pI2CHandle);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_TX_CMPLT);
    }
}

static void I2C_STOPF_Interrupt_Handle(I2C_Handle_t *pI2CHandle)
{
    /* STOPF: SR1 zaten okundu, CR1'e (bos da olsa) yazmak RM0090'a gore
     * temizlemeyi tamamlar. */
    pI2CHandle->pI2Cx->CR1 |= 0x0000;

    I2C_ApplicationEventCallback(pI2CHandle, I2C_EVENT_STOP);
}

void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    uint32_t sr1 = pI2CHandle->pI2Cx->SR1;
    uint32_t cr2 = pI2CHandle->pI2Cx->CR2;

    uint8_t itevten = (cr2 & (1 << I2C_CR2_ITEVTEN)) ? 1 : 0;
    uint8_t itbufen = (cr2 & (1 << I2C_CR2_ITBUFEN)) ? 1 : 0;

    if(itevten && (sr1 & (1 << I2C_SR1_SB)))
    {
        I2C_SB_Interrupt_Handle(pI2CHandle);
    }

    if(itevten && (sr1 & (1 << I2C_SR1_ADDR)))
    {
        I2C_ADDR_Interrupt_Handle(pI2CHandle);
    }

    if(itevten && (sr1 & (1 << I2C_SR1_BTF)))
    {
        I2C_BTF_Interrupt_Handle(pI2CHandle);
    }

    if(itevten && (sr1 & (1 << I2C_SR1_STOPF)))
    {
        I2C_STOPF_Interrupt_Handle(pI2CHandle);
    }

    if(itevten && itbufen && (sr1 & (1 << I2C_SR1_TXE)))
    {
        I2C_TXE_Interrupt_Handle(pI2CHandle);
    }

    if(itevten && itbufen && (sr1 & (1 << I2C_SR1_RXNE)))
    {
        I2C_RXNE_Interrupt_Handle(pI2CHandle);
    }
}

void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
    I2Cx_RegDef_t *pI2Cx = pI2CHandle->pI2Cx;
    uint32_t sr1 = pI2Cx->SR1;
    uint32_t cr2 = pI2Cx->CR2;

    if((cr2 & (1 << I2C_CR2_ITERREN)) == 0)
    {
        return;
    }

    if(sr1 & (1 << I2C_SR1_BERR))
    {
        pI2Cx->SR1 &= ~(1 << I2C_SR1_BERR);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_BERR);
    }

    if(sr1 & (1 << I2C_SR1_ARLO))
    {
        pI2Cx->SR1 &= ~(1 << I2C_SR1_ARLO);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_ARLO);
    }

    if(sr1 & (1 << I2C_SR1_AF))
    {
        pI2Cx->SR1 &= ~(1 << I2C_SR1_AF);

        /* Adres NACK'lendi -- handle'i BUSY'de sonsuza kilitli birakmamak
         * icin state'i sifirla, yoksa bir sonraki IT cagrisi hep
         * STATUS_BUSY doner. */
        if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
        {
            I2C_CloseSendData(pI2CHandle);
        }
        else if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
        {
            I2C_CloseReceiveData(pI2CHandle);
        }

        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_AF);
    }

    if(sr1 & (1 << I2C_SR1_OVR))
    {
        pI2Cx->SR1 &= ~(1 << I2C_SR1_OVR);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_OVR);
    }

    if(sr1 & (1 << I2C_SR1_TIMEOUT))
    {
        pI2Cx->SR1 &= ~(1 << I2C_SR1_TIMEOUT);
        I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_TIMEOUT);
    }
}

Status_t I2C_MemRead(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr, uint8_t MemAddr, uint8_t *pRxBuffer, uint32_t length)
{
    Status_t status = I2C_MasterSend(pI2CHandle, &MemAddr, 1, SlaveAddr, I2C_ENABLE_SR);
    if(status != STATUS_OK)
    {
        return status;
    }

    return I2C_MasterReceive(pI2CHandle, pRxBuffer, length, SlaveAddr, I2C_DISABLE_SR);
}

Status_t I2C_MemWrite(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr, uint8_t MemAddr, uint8_t *pTxBuffer, uint32_t length)
{
    if(pI2CHandle == NULL || pTxBuffer == NULL)
    {
        return STATUS_NULL_POINTER;
    }

    if(length == 0)
    {
        return STATUS_INVALID_PARAM;
    }

    if(!IS_I2C_ADDRESS(SlaveAddr))
    {
        return STATUS_INVALID_PARAM;
    }

    I2C_MasterAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr);

    /* MemAddr, veri baytlarindan once, AYNI kesintisiz yazma fazinda gider --
     * araya START/STOP girerse slave bunu yeni bir "register adresi" olarak
     * yorumlar ve veri yanlis yere yazilir. */
    I2C_MasterWriteBytes(pI2CHandle->pI2Cx, &MemAddr, 1);
    I2C_MasterWriteBytes(pI2CHandle->pI2Cx, pTxBuffer, length);
    I2C_MasterWaitTransferComplete(pI2CHandle->pI2Cx);

    I2C_Generate_Stop_Condition(pI2CHandle->pI2Cx);

    return STATUS_OK;
}

void I2C_DeInit(I2Cx_RegDef_t *pI2Cx)
{
    if(pI2Cx == I2C1)
    {
        I2C1_REG_RESET();
    }
    else if(pI2Cx == I2C2)
    {
        I2C2_REG_RESET();
    }
    else if(pI2Cx == I2C3)
    {
        I2C3_REG_RESET();
    }
}