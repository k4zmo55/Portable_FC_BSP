#include "spi.h"
#include "gpio.h"
#include "stm32f4xx.h"
#include <inttypes.h>

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
        //BIDIMODE Should be Set
        tempreg |= (1 << SPI_CR1_BIDIMODE);

        //BIDIOE Should be Cleared
        tempreg &= ~(1 << SPI_CR1_BIDIOE);
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