/* 
	Task used to manage the system

	Luca Meschiari
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lpc24xx.h"
#include "lcd_grph.h"
#include <stdio.h>
#include <string.h>
#include "sensors.h"

/* Maximum task stack size */
#define manTaskSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

//Queues
static xQueueHandle xSenManQ;
static xQueueHandle xLcdManQ;
static xQueueHandle xManSenQ;
static xQueueHandle xManLcdQ;
static xQueueHandle xTimManQ;
static xQueueHandle xManTimQ;

//Fixed combination for the alarm system
static int combination[4]={1,2,3,4};

/* The Sensors task. */
static void vmanTask( void *pvParameters );

void vStartManTask( unsigned portBASE_TYPE uxPriority, xQueueHandle xSenQ, xQueueHandle xLcdQ, xQueueHandle xManSQ, xQueueHandle xManLQ, xQueueHandle xTimQ,xQueueHandle xManTQ )
{
	/* Spawn the console task . */
	xTaskCreate( vmanTask, ( signed char * ) "ManTask", manTaskSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );

	xSenManQ=xSenQ;
	xLcdManQ=xLcdQ;
	xManSenQ=xManSQ;
	xManLcdQ=xManLQ;
	xTimManQ=xTimQ;
	xManTimQ=xManTQ;
}





static portTASK_FUNCTION( vmanTask, pvParameters )
{
	int bPressed=0;		 //button pressed
	int aActive=0;		 //sensor active
	int command=0;		 //variable used to send commands on the queue
	int bSeq=0;			 //Counter of the button sequence
	int bUser[4]={0,0,0,0};	 //Array to check the combination
	int state=0;	   //0 disabled, 1 enable, 2 alarm , 3 pre alarm
	int match;		   //Variable to check the matching of the code
	int i;

	/* Just to stop compiler warnings. */
	( void ) pvParameters;

	for(;;){
	    if(xQueueReceive(xSenManQ, &aActive, 1)){	  //Receive sensors data
			if (state==1){							  //If state=armed -> alarm or pre alarm
				if(aActive==1){						  //Pre alarm state
					state=3;
					bSeq=0;
					command=1;
					xQueueSend(xManTimQ, &command,1); //activate timer
					command=3;
					xQueueSend(xManLcdQ, &command,1); //update lcd
				}
				else{	//Alarm state
					state=2;
					bSeq=0;
					command=2;
					xQueueSend(xManLcdQ, &command,1);
					xQueueSend(xManSenQ, &aActive,1);
				}
			 }else if (state==2){					//If system in alarm state update the LEDs with the new sensor triggered
			 	xQueueSend(xManSenQ, &aActive,1);
			}else if (state==3){					//If the system is in pre alarm state and sensors 2-3-4 triggered -> alarm state
			 	if((aActive>1)&&(aActive<=4)){
					xQueueSend(xManSenQ, &aActive,1); //Turn on the triggered sensor LED and also the sensor 1 LED
					command=1;
					xQueueSend(xManSenQ, &command,1);
					command=0;
					xQueueSend(xManTimQ, &command,1); //disable timer
					state=2;
					bSeq=0;
					command=2;
					xQueueSend(xManLcdQ, &command,1); //Update LCD
				 }
			}
					
		}
		if(xQueueReceive(xLcdManQ, &bPressed, 1)){		  //Ged Button pressed from the LCD
			bUser[bSeq]=bPressed;
			bSeq++;
			if(bSeq==4){			  //Check the combinatioi if 4 digits entered
				bSeq=0;
				match=0;
				for (i=0; i<4;i++){
					if(bUser[i]==combination[i]){
						match++;
					}
				}
				if (match==4){	   //right sequence
					if(state==0){   //set armed state
					   	state=1;
						command=1;
						xQueueSend(xManLcdQ, &command,1);
					}
					else if((state==2)||(state==1)||(state==3)){  // set not armed state
						if (state==2){
							command=0;
							xQueueSend(xManSenQ, &command,1);
						}
						if(state==3){
							command=0;
							xQueueSend(xManTimQ, &command,1);	//disable timer
						}
						state=0;
						command=0;
						xQueueSend(xManLcdQ, &command,1);
								
					}
				}
				else{			   //Wrong Sequence
					command=4;	
					xQueueSend(xManLcdQ, &command,1);

				}
			} 
		}

		if(xQueueReceive(xTimManQ, &command, 1)){	 //Receive time remaining from the timer task
		 	if (state==3){
				if(command==0){			 //If 30 seconds expired-> alarm state ->update display+sensors LED
					state=2;
					command=2;
					xQueueSend(xManLcdQ, &command,1);
					command=1;
					xQueueSend(xManSenQ, &command,1);
				}
				else{
					command+=100;
					xQueueSend(xManLcdQ, &command,1);	//Update time remaining on the LCD
				}
			}
		}

	}

					 
	
}
