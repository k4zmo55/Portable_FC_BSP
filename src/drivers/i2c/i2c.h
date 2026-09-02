#ifndef FC_I2C_H
#define FC_I2C_H

#include "stm32f4xx.h"
#include "dma.h"
#include <stdint.h>

typedef struct{
    uint32_t SclSpeed;      /* I2C Serial Clock Speed (Hz) Refer @I2C_SclSpeed */
    uint8_t  DeviceAddress; /* I2C Own Address (7-bit, slave modda kullanilir) */
    uint8_t  ACKControl;    /* I2C Acknowledge Control Refer @I2C_ACKControl */
    uint8_t  FMDutyCycle;   /* Fast mode SCL duty cycle Refer @I2C_FMDutyCycle */
}I2C_Config_t;

typedef struct{
    I2Cx_RegDef_t *pI2Cx;
    I2C_Config_t   i2c_config;

    /* Interrupt tabanli I2C_MasterSendIT/I2C_MasterReceiveIT icin durum
     * bilgisi. Blocking I2C_MasterSend/I2C_MasterReceive bu alanlari
     * kullanmaz. */
    uint8_t  *pTxBuffer;
    uint8_t  *pRxBuffer;
    uint32_t TxLen;
    uint32_t RxLen;
    uint8_t  TxRxState;  // Refer @I2C_State
    uint8_t  DevAddr;    // Guncel islemin hedef slave adresi (7-bit)
    uint8_t  Sr;         // Repeated start: ENABLE/DISABLE. Refer @I2C_RepeatedStart
}I2C_Handle_t;

/* @I2C_State */
typedef enum { I2C_READY = 0, I2C_BUSY_IN_TX, I2C_BUSY_IN_RX } I2C_State_t;

/* I2C_ApplicationEventCallback'e gecilen olay/hata kodlari */
#define I2C_EVENT_TX_CMPLT   1
#define I2C_EVENT_RX_CMPLT   2
#define I2C_EVENT_STOP       3
#define I2C_EVENT_DATA_REQ   4  // Slave modda master veri istiyor (TXE)
#define I2C_EVENT_DATA_RCV   5  // Slave modda veri geldi (RXNE)
#define I2C_ERROR_BERR       6
#define I2C_ERROR_ARLO       7
#define I2C_ERROR_AF         8
#define I2C_ERROR_OVR        9
#define I2C_ERROR_TIMEOUT    10

/* @I2C_SclSpeed (Hz) */
#define I2C_SPEED_STANDARD  100000U
#define I2C_SPEED_FAST      400000U

/* @I2C_ACKControl -- CR1.ACK biti; PE=0 oldugunda donaniminca temizlenir */
#define I2C_ACK_ENABLE   ENABLE
#define I2C_ACK_DISABLE  DISABLE

/* @I2C_FMDutyCycle -- CCR.DUTY biti (sadece fast mode icin anlamli) */
#define I2C_FM_DUTY_2      0
#define I2C_FM_DUTY_16_9   1

/* @I2C_RepeatedStart -- Send/Receive sonunda STOP yerine tekrar START */
#define I2C_DISABLE_SR   0
#define I2C_ENABLE_SR    1

Status_t I2C_Init(I2C_Handle_t *pI2CHandle);
void     I2C_DeInit(I2Cx_RegDef_t *pI2Cx);

void I2C_PeriClockControl(I2Cx_RegDef_t *pI2Cx, uint8_t EnOrDi);
void I2C_PeripheralControl(I2Cx_RegDef_t *pI2Cx, uint8_t EnOrDi);

Status_t I2C_MasterSend(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr);
Status_t I2C_MasterReceive(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr);

Status_t I2C_GetFlagStatus(I2Cx_RegDef_t *pI2Cx, uint32_t FlagName);

/* Register/bellek adresli okuma-yazma (MPU6050 gibi sensorler icin) --
 * MemRead: yaz(MemAddr, Sr=ENABLE) + repeated START + oku(Sr=DISABLE)
 * ikilisini I2C_MasterSend/I2C_MasterReceive uzerinden zincirler.
 * MemWrite bunun aksine TEK bir kesintisiz yazma fazi gerektirir (MemAddr +
 * veri baytlari ayni START..STOP arasinda gitmeli), bu yuzden kendi START/
 * STOP akisini yurutur. */
Status_t I2C_MemRead(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr, uint8_t MemAddr, uint8_t *pRxBuffer, uint32_t length);
Status_t I2C_MemWrite(I2C_Handle_t *pI2CHandle, uint8_t SlaveAddr, uint8_t MemAddr, uint8_t *pTxBuffer, uint32_t length);

/* Interrupt tabanli, non-blocking gonderme/alma. Fonksiyon transferi
 * baslatir ve hemen doner; gercek byte aktarimi I2Cx_EV_IRQHandler() /
 * I2Cx_ER_IRQHandler() icinden cagrilacak I2C_EV_IRQHandling() /
 * I2C_ER_IRQHandling() tarafindan yapilir. Transfer bitince
 * I2C_ApplicationEventCallback() cagrilir. */
Status_t I2C_MasterSendIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr);
Status_t I2C_MasterReceiveIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr);

void I2C_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi);
void I2C_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority);
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle);
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle);

void I2C_CloseSendData(I2C_Handle_t *pI2CHandle);
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle);

/* Slave modda tek byte gonderme/alma -- EV_IRQHandling icinden ADDR/TXE/RXNE
 * olaylarina cevaben cagrilmasi amaclanir. */
void    I2C_SlaveSendData(I2Cx_RegDef_t *pI2Cx, uint8_t data);
uint8_t I2C_SlaveReceiveData(I2Cx_RegDef_t *pI2Cx);
void    I2C_SlaveEnableDisableCallbackEvents(I2Cx_RegDef_t *pI2Cx, uint8_t EnOrDi);

/* Zayif (weak) varsayilan implementasyon -- SPI_ApplicationEventCallback ile
 * ayni desen: uygulama kodu ayni imzayla override edebilir. */
void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv);

/* DMA tabanli gonderme/alma. pDMAHandle onceden DMA_Init() ile hazirlanmis
 * olmalidir (bkz. spi.h'daki ayni desen / Docs/DMA.md). */
Status_t I2C_MasterSendDMA(I2C_Handle_t *pI2CHandle, DMA_Handle_t *pDMAHandle, uint8_t *pTxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr);
Status_t I2C_MasterReceiveDMA(I2C_Handle_t *pI2CHandle, DMA_Handle_t *pDMAHandle, uint8_t *pRxBuffer, uint32_t length, uint8_t SlaveAddr, uint8_t Sr);

void I2C_Generate_Start_Condition(I2Cx_RegDef_t *pI2Cx);
void I2C_Generate_Stop_Condition(I2Cx_RegDef_t *pI2Cx);

void I2C_ExecuteAddressPhaseWrite(I2Cx_RegDef_t *pI2Cx, uint8_t addr);

#define IS_I2C(pI2Cx) (((pI2Cx) == I2C1) || ((pI2Cx) == I2C2) || ((pI2Cx) == I2C3))

#define IS_I2C_SCL_SPEED(Speed) (((Speed) > 0U) && ((Speed) <= I2C_SPEED_FAST))

#define IS_I2C_ACK(Ack) (((Ack) == I2C_ACK_ENABLE) || ((Ack) == I2C_ACK_DISABLE))

#define IS_I2C_FM_DUTY(Duty) (((Duty) == I2C_FM_DUTY_2) || ((Duty) == I2C_FM_DUTY_16_9))

#define IS_I2C_SR(Sr) (((Sr) == I2C_DISABLE_SR) || ((Sr) == I2C_ENABLE_SR))

#define IS_I2C_ADDRESS(Addr) ((Addr) <= 0x7FU)

#endif /*FC_I2C_H*/
