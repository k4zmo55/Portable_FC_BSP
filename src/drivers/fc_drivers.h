#ifndef FC_DRIVERS_H
#define FC_DRIVERS_H

/* Semsiye header -- Drivers katmanindaki tum surucu header'larini tek
 * satirda toplar. Her surucu .c dosyasi tek tek gpio.h/spi.h/i2c.h/dma.h/
 * nvic.h/rcc.h include etmek yerine sadece bunu include eder.
 *
 * stm32f4xx.h (Device katmani) burada BULUNMAZ -- her surucu header'i
 * onu zaten kendi icinde include ediyor, dolayisiyla transitive olarak
 * gelir. stm32f4xx.h'in kendisi Drivers katmanina bagimli olmamali (Device
 * katmani Drivers'tan once, Drivers'tan bagimsiz tanimlanir); onu buraya
 * eklemek gerekmez, tersini yapmak (stm32f4xx.h'a bu dosyalari eklemek)
 * bagimlilik yonunu tersine cevirir. */

#include "gpio.h"
#include "rcc.h"
#include "nvic.h"
#include "dma.h"
#include "spi.h"
#include "i2c.h"

#endif /* FC_DRIVERS_H */
