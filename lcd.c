/* 
	Task that initialises the EA QVGA LCD display
	with touch screen controller and processes touch screen
	interrupt events.

	Luca Meschiari
*/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include <stdio.h>
#include <string.h>

/* Maximum task stack size */
#define lcdSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

//Declare queues
static xQueueHandle xLcdManQ;
static xQueueHandle xManLcdQ;
static xQueueHandle xLCDQ;

//Variable used to check the 30 seconds delay
int seconds=30;

/* Interrupt handlers */
extern void vLCD_ISREntry( void );
void vLCD_ISRHandler( void );

/* The LCD task. */
static void vLcdTask( void *pvParameters );

//Functions used to manage the Lcd
int button_pressed(int x, int y);
void restore_button(int button);

void vStartLcd( unsigned portBASE_TYPE uxPriority, xQueueHandle xManQ, xQueueHandle xLcdQ )
{
	/* Spawn the console task . */
	xTaskCreate( vLcdTask, ( signed char * ) "Lcd", lcdSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );

	//Create queues
	xLCDQ = xQueueCreate( 10, sizeof( unsigned portLONG ) );
	xLcdManQ=xManQ;
	xManLcdQ=xLcdQ;
}

//Function to manage the button pressing -- return the value of the button pressed and highlioght the button to give feedback
int button_pressed(int x, int y){
	//First col
	if((x>=10)&&(x<=70)){
		if((y>=10)&&(y<=70)){
			lcd_fillRect(10, 10, 70, 70, YELLOW);
			lcd_putChar(38, 38, '1');
			return 1;
		}else if((y>=80)&&(y<=140)){
		    lcd_fillRect(10, 80, 70, 140, YELLOW);
			lcd_putChar(38, 108, '4');
			return 4;
		}else if((y>=150)&&(y<=210)){
			lcd_fillRect(10, 150, 70, 210, YELLOW);
			lcd_putChar(38, 178, '7');
			return 7;
		}
	}
	//Second col
	else if((x>80)&&(x<140)){
		if((y>=10)&&(y<=70)){
		    lcd_fillRect(80, 10, 140, 70, YELLOW);
			lcd_putChar(108, 38, '2');
			return 2;
		}else if((y>=80)&&(y<=140)){
			lcd_fillRect(80, 80, 140, 140, YELLOW);
			lcd_putChar(108, 108, '5');
			return 5;
		}else if((y>=150)&&(y<=210)){
			lcd_fillRect(80, 150, 140, 210, YELLOW);
			lcd_putChar(108, 178, '8');
			return 8;
		}else if((y>=220)&&(y<=280)){
			lcd_fillRect(80, 220, 140, 280, YELLOW);
			lcd_putChar(108, 248, '0');
			return 0;
		}
	}
	//Third col
	else if((x>150)&&(x<210)){
		if((y>=10)&&(y<=70)){
			lcd_fillRect(150, 10, 210, 70, YELLOW);
			lcd_putChar(178, 38, '3');
			return 3;
		}else if((y>=80)&&(y<=140)){
		    lcd_fillRect(150, 80, 210, 140, YELLOW);
			lcd_putChar(178, 108, '6');
			return 6;
		}else if((y>=150)&&(y<=210)){
			lcd_fillRect(150, 150, 210, 210, YELLOW);
			lcd_putChar(178, 178, '9');
			return 9;
		}
	}

	//Lcd touched outside buttons boundaries
	return -1;
}

//Restore button after touch
void restore_button(int button){
 	switch(button){
	case 0: lcd_fillRect(80, 220, 140, 280, BLUE);
			lcd_putChar(108, 248, '0');
			return;
	case 1: lcd_fillRect(10, 10, 70, 70, BLUE);
			lcd_putChar(38, 38, '1');
			return;
	case 2: lcd_fillRect(80, 10, 140, 70, BLACK);
			lcd_putChar(108, 38, '2');
			return;
    case 3: lcd_fillRect(150, 10, 210, 70, BLUE);
			lcd_putChar(178, 38, '3');
			return;
	case 4: lcd_fillRect(10, 80, 70, 140, BLACK);
			lcd_putChar(38, 108, '4');
			return;
	case 5: lcd_fillRect(80, 80, 140, 140, BLUE);
			lcd_putChar(108, 108, '5');
			return;
	case 6: lcd_fillRect(150, 80, 210, 140, BLACK);
			lcd_putChar(178, 108, '6');
			return;
	case 7: lcd_fillRect(10, 150, 70, 210, BLUE);
			lcd_putChar(38, 178, '7');
			return;
	case 8: lcd_fillRect(80, 150, 140, 210, BLACK);
			lcd_putChar(108, 178, '8');
			return;
	case 9: lcd_fillRect(150, 150, 210, 210, BLUE);
			lcd_putChar(178, 178, '9');
			return;
	}
}

//Draw the rectangle at the bottom of the Lcd and write the status of the system and other useful informations
void writeState(int state,int wr){	   //0 disarmed, 1 armed, 2 alarm, 3 pre alarm mode	
	char string[50];
	if (state<3){
		lcd_fillRect(10, 290, 210, 318, BLACK); 
	}
	else{  //if pre alarm mode draw also a red rectangle
		lcd_fillRect(10, 290, 210, 308, RED);
		lcd_fillRect(10, 308, 210, 318, BLACK);
	}

	//Write the state of the system
	if(state==0){
		sprintf(string,"Alarm disarmed");
	}
	if(state==1){
		sprintf(string,"Alarm armed");
	}
	if(state==2){
		sprintf(string,"ALARM!!");
	}
	if(state==3){ //if pre alarm mode write also the countdowm before the alarm state
		sprintf(string,"Pre alarm - You have  %2d  sec",seconds);
	}

	lcd_putString(20,295,(unsigned char*)string);
	sprintf(string,"Code:");
	lcd_putString(20,310,(unsigned char*)string);

	//If code is incorrect notify the user
	if(wr==1){
		sprintf(string,"Wrong Code!");
		lcd_putString(120,310,(unsigned char*)string);
	}
 }




static portTASK_FUNCTION( vLcdTask, pvParameters )
{
	//Variable used for the interrupt Lcd queue
	unsigned portLONG ulVar = 10UL;

	unsigned int touch_x;
	unsigned int touch_y;
	unsigned int touch_pr;
	int bPress=-1;	//button pressed
	int bSeq=0;		//Position in the 4 sequence code
	char buf[15];
	int command=0;	//variable used to send commands on the queues
	int state=0;	//state of the system

	portTickType xLastPollTime;
	
	
	/* Just to stop compiler warnings. */
	( void ) pvParameters;


	/* Initialise LCD display */
	/* NOTE: We needed to delay calling lcd_init() until here because it uses
	 * xTaskDelay to implement a delay and, as a result, can only be called from
	 * a task */
	lcd_init();

	//Draw the interface
	lcd_fillScreen(MAROON);
	
	lcd_fillRect(10, 10, 70, 70, BLUE);
	lcd_putChar(38, 38, '1');
	lcd_fillRect(80, 10, 140, 70, BLACK);
	lcd_putChar(108, 38, '2');
	lcd_fillRect(150, 10, 210, 70, BLUE);
	lcd_putChar(178, 38, '3');

	lcd_fillRect(10, 80, 70, 140, BLACK);
	lcd_putChar(38, 108, '4');
	lcd_fillRect(80, 80, 140, 140, BLUE);
	lcd_putChar(108, 108, '5');
	lcd_fillRect(150, 80, 210, 140, BLACK);
	lcd_putChar(178, 108, '6');

	lcd_fillRect(10, 150, 70, 210, BLUE);
	lcd_putChar(38, 178, '7');
	lcd_fillRect(80, 150, 140, 210, BLACK);
	lcd_putChar(108, 178, '8');
	lcd_fillRect(150, 150, 210, 210, BLUE);
	lcd_putChar(178, 178, '9');

	lcd_fillRect(80, 220, 140, 280, BLUE);
	lcd_putChar(108, 248, '0');
	
	writeState(state,0);
	

	/* Infinite loop blocks waiting for a touch screen interrupt event from
	 * the queue. */
	for( ;; )
	{
		//Reset interrupt
		EXTINT = 8;	
		//Enable TS interrupt																								+
		VICIntEnable |= 1 << 17;		/* Enable interrupts on vector 17 */
		//Deselect Button
		if(bPress>=0){
			restore_button(bPress);
			bPress=-1;
		}
		//Read from the interrupt LCD queue
		if( xQueueReceive( xLCDQ, &ulVar, 2) )
        {
			xLastPollTime = xTaskGetTickCount();  //get current tick count
			VICIntEnClr |= 1 << 17;		/* Disable interrupts on vector 17 */
			getTouch(&touch_x,&touch_y,&touch_pr);	 //Get touch values
			bPress=button_pressed(touch_x, touch_y); //Higlight the button+get the button pressed
			if((bPress>=0)&&(bSeq<=3)){				 //Manage the touch of the button
				if(bSeq==0){						 //Delete the "wrong code" sentence from the screen
					sprintf(buf,"            ");
				  	lcd_putString(120, 310, (unsigned char*)buf);
				}	
				if(xQueueSend(xLcdManQ, &bPress,5)){ //If the button pressed is sent to the managing task write the value on the screen
				   sprintf(buf,"%d",bPress);
				   lcd_putString((60+6*bSeq), 310, (unsigned char*)buf);
				   bSeq++;							 //increase the button sequence counter
				}
			}
					
			do{										  //wait until the button is released
				getTouch(&touch_x,&touch_y,&touch_pr);
			}while(touch_pr>0);	 	
			vTaskDelayUntil( &xLastPollTime, 50); //Delay to avoid bouncing on the Lcd

        }

		if( xQueueReceive( xManLcdQ, &command, 2) )	{ //Receive command from to the managing task to update the screen
				if (command==0){
					bSeq=0;
					state=0;
					writeState(state,0);
				}else if (command==1){
					bSeq=0;
					state=1;
					writeState(state,0);
				}else if (command==2){
					bSeq=0;
					state=2;
					writeState(state,0);
				}else if (command==3){
					bSeq=0;
					state=3;
					seconds=30;
					writeState(state,0);
				}else if (command==4){
					bSeq=0;
					writeState(state,1);
				}else if(command>100){	 //write time for countdown
					seconds=command-100;
					sprintf(buf,"%2d",seconds);
				   	lcd_putString(151, 295, (unsigned char*)buf);
				}
		}			
	}
}

void vLCD_ISRHandler( void )
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    unsigned portLONG ulVar = 10UL;

	EXTINT = 8;					/* Reset EINT3 */
	VICVectAddr = 0;			/* Clear VIC interrupt */

	/* Process the touchscreen interrupt */
	/* We would want to indicate to the task above that an event has occurred */
	
	xQueueSendToBackFromISR( xLCDQ, ( void * ) &ulVar, &xHigherPriorityTaskWoken );

	/* Exit the ISR.  If a task was woken by either a character being received
	or transmitted then a context switch will occur. */
	portEXIT_SWITCHING_ISR( xHigherPriorityTaskWoken );
}


