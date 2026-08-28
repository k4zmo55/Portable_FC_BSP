#ifndef FC_SPI_H
#define FC_SPI_H

#include "stm32f4xx.h"
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
}SPI_Handle_t;

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

Status_t SPI_Send(SPIx_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t length);
Status_t SPI_Receive(SPIx_RegDef_t *pSPIx,uint8_t *pRxBuffer, uint32_t length);

Status_t SPI_GetFlagStatus(SPIx_RegDef_t *pSPIx, uint32_t FlagName);


#define IS_SPI(pSPIx) (((pSPIx) == SPI1) || ((pSPIx) == SPI2) || \
                       ((pSPIx) == SPI3))

#define IS_DFF(DFF) (((DFF) == DATA_8_BIT) || ((DFF) == DATA_16_BIT))

#define IS_CPOL(CPOL) (((CPOL) == CPOL_0) || ((CPOL) == CPOL_1))

#define IS_CPHA(CPHA) (((CPHA) == CPHA_0) || ((CPHA) == CPHA_1))

#define IS_DEVICE_MODE(Mode) (((Mode) == DEVICE_SLAVE) || ((Mode) == DEVICE_MASTER))

#define IS_BUS_CONFIG(Bus) (((Bus) == FULL_DUPLEX) || ((Bus) == HALF_DUPLEX) || ((Bus) == SIMPLE_RXONLY))

#endif /* FC_SPI_H */











