#ifndef LCD_H
#define LCD_H
#include "queue.h"

void vStartLcd( unsigned portBASE_TYPE uxPriority, xQueueHandle xManQ, xQueueHandle xLcdQ );

#endif
