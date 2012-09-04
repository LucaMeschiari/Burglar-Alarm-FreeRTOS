/* 
	Task to control the timer for the 30 seconds delay
	It will send the amount of time remaining every 5 seconds

	Luca Meschiari
*/

#include "FreeRTOS.h"
#include "task.h"
#include "lpc24xx.h"
#include "queue.h"


/* Maximum task stack size */
#define TimerTaskSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

//Queue
static xQueueHandle xTimManQ;
static xQueueHandle xManTimQ;

/* The Timer task. */
static void vTimerTask( void *pvParameters );


void vStartTimerTask( unsigned portBASE_TYPE uxPriority,xQueueHandle xManQ, xQueueHandle xTimQ )
{
	/* Spawn the Timer task . */
	xTaskCreate( vTimerTask, ( signed char * ) "Timer", TimerTaskSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );
	xTimManQ=xTimQ;
	xManTimQ=xManQ;
}


static portTASK_FUNCTION( vTimerTask, pvParameters )
{
	portTickType xLastWakeTime;
	int command;
	int count=30;  //counter for the timer

	while(1)
	{
		if(xQueueReceive(xManTimQ, &command, 1)){	  //Receive command from the managing task to activate the timer
			if(command==1) 		//start conter
			count=30;								  //Reset 30 seconds counter
			xLastWakeTime = xTaskGetTickCount();	  //Get Tick to avoid uncorrect timing
			do{
				vTaskDelayUntil( &xLastWakeTime, 5000);  //wait 5 seconds
				xLastWakeTime = xTaskGetTickCount();  //Get Tick to avoid uncorrect timing while doing other things
				if(xQueueReceive(xManTimQ, &command, 1)){  //If stop timer command received break the loop
					if (command==0){	
						break;
					}
				}
				count-=5;
				xQueueSend(xTimManQ, &count,1);			  //Send the amount of seconds remaining to the managing task
			}while(count>0);	
			//xQueueSend(xTimManQ, &count,1);
		}

	}
}
