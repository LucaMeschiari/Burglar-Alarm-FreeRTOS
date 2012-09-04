#ifndef MANTASK_H
#define MANTASK_H
#include "queue.h"



void vStartManTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xSenQ, xQueueHandle xLcdQ, xQueueHandle xManSQ, xQueueHandle xManLQ, xQueueHandle xTimQ,xQueueHandle xManTQ );

#endif
