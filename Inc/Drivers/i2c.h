#ifndef FC_I2C_H
#define FC_I2C_H

#include "stm32f4xx.h"
#include "dma.h"
#include <stdint.h>

typedef struct{
    uint8_t SclSpeed;       /* I2C Serial Clock Speed Refer @I2C_SclSpeed*/
    uint8_t DeviceAddress;  /* I2C Device Address */
    uint8_t ACKControl;     /* I2C Acknowledge Control Refer @I2C_ACKControl*/
    uint8_t FMDutyCycle;
}I2C_Config_t;

/* @I2C_SclSpeed */
#define I2C_SPEED_STANDARD 1000000U
#define I2C_SPEED_FAST     4000000U




#endif /*FC_I2C_H*/