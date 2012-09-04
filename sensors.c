/* 
	Sensors Task

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
#define sensorsSTACK_SIZE			( ( unsigned portBASE_TYPE ) 256 )

//Queues
static xQueueHandle xSenManQ;
static xQueueHandle xManSenQ;

//Variable to manage the LEDs
unsigned char ledData=0;


/* The Sensors task. */
static void vSensorsTask( void *pvParameters );



void vStartSensors( unsigned portBASE_TYPE uxPriority, xQueueHandle xManQ,xQueueHandle xSenQ )
{
	/* Spawn the console task . */
	xTaskCreate( vSensorsTask, ( signed char * ) "Sensors", sensorsSTACK_SIZE, NULL, uxPriority, ( xTaskHandle * ) NULL );
	xSenManQ=xManQ;
	xManSenQ=xSenQ;
}


/* Get I2C button status */
unsigned char getButtons()
{
	unsigned char ledData;

	/* Initialise */
	I20CONCLR =  I2C_AA | I2C_SI | I2C_STA | I2C_STO;
	
	/* Request send START */
	I20CONSET =  I2C_STA;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */		
	I20DAT    =  0xC0;
	I20CONCLR =  I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Send control word to read PCA9532 INPUT0 register */
	I20DAT = 0x00;
	I20CONCLR =  I2C_SI;

	/* Wait for DATA with control word to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send repeated START */
	I20CONSET =  I2C_STA;
	I20CONCLR =  I2C_SI;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */		
	I20DAT    =  0xC1;
	I20CONCLR =  I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));

	I20CONCLR = I2C_SI;

	/* Wait for DATA to be received */
	while (!(I20CONSET & I2C_SI));

	ledData = I20DAT;

	/* Request send NAQ and STOP */
	I20CONSET =  I2C_STO;
	I20CONCLR =  I2C_SI | I2C_AA;

	/* Wait for STOP to be sent */
	while (I20CONSET & I2C_STO);

	return ledData ^ 0xf;
}


/* Set I2C LEDs */
void putLights(unsigned char lights)
{
	
	if (lights==0){	  //Turn off LEDs
		ledData=0;
	}
	if (lights & 0x1)
	{
		ledData |= 0x01;
	}
	if (lights & 0x02)
	{
	 	ledData |= 0x4;
	}
	if (lights & 0x04)
	{
		ledData |= 0x10;
	}
	if (lights & 0x08)
	{
		ledData |= 0x40;
	}				  

	/* Initialise */
	I20CONCLR =  I2C_AA | I2C_SI | I2C_STA | I2C_STO;
	
	/* Request send START */
	I20CONSET =  I2C_STA;

	/* Wait for START to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send PCA9532 ADDRESS and R/W bit and clear SI */		
	I20DAT    =  0xC0;
	I20CONCLR =  I2C_SI | I2C_STA;

	/* Wait for ADDRESS and R/W to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Send control word to write PCA9532 LS2 register */
	I20DAT = 0x08;
	I20CONCLR =  I2C_SI;

	/* Wait for DATA with control word to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Send data to write PCA9532 LS2 register */
	I20DAT = ledData;
	I20CONCLR =  I2C_SI;

	/* Wait for DATA with control word to be sent */
	while (!(I20CONSET & I2C_SI));

	/* Request send NAQ and STOP */
	I20CONSET =  I2C_STO;
	I20CONCLR =  I2C_SI;

	/* Wait for STOP to be sent */
	while (I20CONSET & I2C_STO);
}


static portTASK_FUNCTION( vSensorsTask, pvParameters )
{
	//Variables to manage the button pressing
	char buttonPressed;
	char oldButtonPressed=0x0;
	char changedButton=0x0;
	int bPress;
	int manCommand;
	/* Just to stop compiler warnings. */
	( void ) pvParameters;

	
	
					 
	for( ;; )
	{
	   buttonPressed=getButtons();
	   if(buttonPressed!=oldButtonPressed){					 //Check if a button is pressed or released
	   	changedButton=oldButtonPressed^buttonPressed;		 //Get changes from the previous status
		if ((changedButton & 0x1)&&(buttonPressed & 0x1))	 //Check wich button is pressed
		{
			bPress=1;
			xQueueSend(xSenManQ,&bPress,10);
		}
		if ((changedButton & 0x02)&&(buttonPressed & 0x02))
		{
			bPress=2;
			xQueueSend(xSenManQ,&bPress,10);
		}
		if ((changedButton & 0x04)&&(buttonPressed & 0x04))
		{
			bPress=3;
			xQueueSend(xSenManQ,&bPress,10);
		}
		if ((changedButton & 0x08)&&(buttonPressed & 0x08))
		{
			bPress=4;
			xQueueSend(xSenManQ,&bPress,10);
		}
	   }
	   oldButtonPressed=buttonPressed;

	   if(xQueueReceive(xManSenQ, &manCommand, 1)){		 //Wait for command from the Managing task to turn on or off the LEDs
			if((manCommand>=0)||(manCommand<=4)){
				switch(manCommand){
					case 0: putLights(0);
							break;
					case 1: putLights(0x1);
							break;
					case 2: putLights(0x2);
							break;
					case 3: putLights(0x4);
							break;
					case 4: putLights(0x8);
							break;
				 }
	
			}
		}


	}
}
