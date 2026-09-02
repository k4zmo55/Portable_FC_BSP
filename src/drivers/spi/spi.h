#ifndef FC_SPI_H
#define FC_SPI_H

#include "stm32f4xx.h"
#include "dma.h"
#include <stdint.h>

typedef struct{
    uint8_t DeviceMode;   // Refer @SPI_Devicemode
    uint8_t BusConfig;    // Refer @SPI_BusConfiguration0
    uint8_t SclkSpeed;    // Refer @SPI_ClockSpeed
    uint8_t DFF;          // Refer @SPI_DFF
    uint8_t CPHA;         // Refer @SPI_CHPA
    uint8_t CPOL;         // Refer @SPI_CPOL
    uint8_t SSM;          // Refer @SPI_SSM
}SPI_Config_t;


typedef struct{
    SPIx_RegDef_t *pSPIx;
    SPI_Config_t spi_config;

    /* Interrupt tabanli SPI_SendIT/SPI_ReceiveIT icin durum bilgisi.
     * Blocking SPI_Send/SPI_Receive/SPI_TransmitReceive bu alanlari
     * kullanmaz. */
    uint8_t  *pTxBuffer;
    uint8_t  *pRxBuffer;
    uint32_t TxLen;
    uint32_t RxLen;
    uint8_t  TxState;   // Refer @SPI_State
    uint8_t  RxState;   // Refer @SPI_State
}SPI_Handle_t;

/* @SPI_State */
typedef enum { SPI_READY = 0, SPI_BUSY_IN_TX, SPI_BUSY_IN_RX } SPI_State_t;

/* SPI_ApplicationEventCallback'e gecilen olay kodlari */
#define SPI_EVENT_TX_CMPLT   1
#define SPI_EVENT_RX_CMPLT   2
#define SPI_EVENT_OVR_ERR    3

/* @SPI_Devicemode */
typedef enum{ DEVICE_SLAVE  = 0x0, DEVICE_MASTER  }SPI_Devicemode;

/* @SPI_CPOL */
typedef enum { CPOL_0 = 0x0, CPOL_1} SPI_CPOL;

/* @SPI_CPHA */
typedef enum { CPHA_0 = 0x0, CPHA_1} SPI_CPHA;

/* @SPI_DFF */
typedef enum { DATA_8_BIT = 0x0, DATA_16_BIT} SPI_DFF;

/* @SPI_BusConfiguration */
typedef enum { FULL_DUPLEX = 0x0, HALF_DUPLEX , SIMPLE_RXONLY} SPI_BusConfiguration;

/* @SPI_SSM */
typedef enum { SSM_DI = 0x0, SSM_EN} SPI_SSM; 

/* @SPI_SCLK_DIV */
typedef enum { SCLK_DIV2 = 0x0, SCLK_DIV4, SCLK_DIV8, SCLK_DIV16,
               SCLK_DIV32, SCLK_DIV64, SCLK_DIV128, SCLK_DIV256} SPI_Sclk_Div;

Status_t SPI_Init(SPI_Handle_t *spi_handle);
void SPI_DeInit(SPIx_RegDef_t *pSPIx);

void SPI_PeriClockControl(SPIx_RegDef_t *pSPIx, uint8_t EnOrDi);
void SPI_PeripheralControl(SPIx_RegDef_t *pSPIx, uint8_t EnOrDi);

Status_t SPI_Send(SPIx_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t length);
Status_t SPI_Receive(SPIx_RegDef_t *pSPIx,uint8_t *pRxBuffer, uint32_t length);
Status_t SPI_TransmitReceive(SPIx_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint8_t *pRxBuffer, uint32_t length);

Status_t SPI_GetFlagStatus(SPIx_RegDef_t *pSPIx, uint32_t FlagName);

/* Interrupt tabanli, non-blocking gonderme/alma. Fonksiyon transferi
 * baslatir ve hemen doner; gercek byte aktarimi SPIx_IRQHandler()
 * icinden cagrilacak SPI_IRQHandling() tarafindan yapilir. Transfer
 * bitince SPI_ApplicationEventCallback() cagrilir. */
Status_t SPI_SendIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t length);
Status_t SPI_ReceiveIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t length);

void SPI_IRQInterruptConfig(IRQn_Type IRQNumber, uint8_t EnOrDi);
void SPI_IRQPriorityConfig(IRQn_Type IRQNumber, uint8_t IRQPriority);
void SPI_IRQHandling(SPI_Handle_t *pSPIHandle);

void SPI_CloseTransmission(SPI_Handle_t *pSPIHandle);
void SPI_CloseReception(SPI_Handle_t *pSPIHandle);

/* Zayif (weak) varsayilan implementasyon -- uygulama kodu ayni imzayla
 * kendi fonksiyonunu tanimlayarak override edebilir. */
void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv);

/* DMA tabanli gonderme/alma. pDMAHandle onceden DMA_Init() ile hazirlanmis
 * olmalidir (Channel/Direction/Circular vb. -- bkz. Docs/DMA.md). Bu
 * fonksiyonlar sadece CR2'deki ilgili DMA-enable bitini acip
 * DMA_Start()'i cagirir; asil transfer donanim tarafindan, CPU
 * mudahalesi olmadan yurutulur. */
Status_t SPI_SendDMA(SPI_Handle_t *pSPIHandle, DMA_Handle_t *pDMAHandle, uint8_t *pTxBuffer, uint32_t length);

/* NOT: FULL_DUPLEX modda -- tipki SPI_Receive/SPI_ReceiveIT gibi -- bu
 * fonksiyon tek basina saat uretmez (SPI_CR2_RXDMAEN sadece "veri gelirse
 * DMA'ya ver" der, MOSI'ye hicbir sey yazilmadigi surece TXE/saat hic
 * olusmaz). Full-duplex bir okuma icin SPI_SendDMA'yi (dummy buffer ile)
 * ve SPI_ReceiveDMA'yi AYNI ANDA, iki farkli DMA_Handle_t (TX icin bir
 * stream, RX icin baska bir stream) ile baslatmak gerekir. */
Status_t SPI_ReceiveDMA(SPI_Handle_t *pSPIHandle, DMA_Handle_t *pDMAHandle, uint8_t *pRxBuffer, uint32_t length);

#define IS_SPI(pSPIx) (((pSPIx) == SPI1) || ((pSPIx) == SPI2) || \
                       ((pSPIx) == SPI3))

#define IS_DFF(DFF) (((DFF) == DATA_8_BIT) || ((DFF) == DATA_16_BIT))

#define IS_CPOL(CPOL) (((CPOL) == CPOL_0) || ((CPOL) == CPOL_1))

#define IS_CPHA(CPHA) (((CPHA) == CPHA_0) || ((CPHA) == CPHA_1))

#define IS_DEVICE_MODE(Mode) (((Mode) == DEVICE_SLAVE) || ((Mode) == DEVICE_MASTER))

#define IS_BUS_CONFIG(Bus) (((Bus) == FULL_DUPLEX) || ((Bus) == HALF_DUPLEX) || ((Bus) == SIMPLE_RXONLY))

#endif /* FC_SPI_H */











