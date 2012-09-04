#ifndef SENSORS_H
#define SENSORS_H
#include "queue.h"


#define I2C_AA      0x00000004
#define I2C_SI      0x00000008
#define I2C_STO     0x00000010
#define I2C_STA     0x00000020
#define I2C_I2EN    0x00000040

void vStartSensors( unsigned portBASE_TYPE uxPriority, xQueueHandle xManQ,xQueueHandle xSenQ );

#endif
