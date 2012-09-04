/* 
	Luca Meschiari
*/

/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "console.h"
#include "lcd.h"
#include "lcd_hw.h"
#include "lcd_grph.h"
#include "sensors.h"
#include "manTask.h"
#include "queue.h"
#include "Timer.h"


extern void vLCD_ISREntry( void );


//Declare all the Queues -> will be passed to the tasks
//Manager-Sensors
static xQueueHandle xSenManQ;
static xQueueHandle xManSenQ;
//Manager-Lcd
static xQueueHandle xLcdManQ;
static xQueueHandle xManLcdQ;
//Manager-Timer
static xQueueHandle xTimManQ;
static xQueueHandle xManTimQ;


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );



/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();

	//Create Queues
	xSenManQ = xQueueCreate( 10, sizeof( int ) );
	xLcdManQ = xQueueCreate( 10, sizeof( int ) );
	xManSenQ = xQueueCreate( 10, sizeof( int ) );
	xManLcdQ = xQueueCreate( 10, sizeof( int ) );
	xManTimQ = xQueueCreate( 10, sizeof( int ) );
	xTimManQ = xQueueCreate( 10, sizeof( int ) );

	 
	/* Start the sensors task */
	vStartLcd(2,xLcdManQ,xManLcdQ);

	/* Start the sensors task */
	vStartSensors(2,xSenManQ,xManSenQ);
	
	/* Start the managing Task */
	vStartManTask(2,xSenManQ,xLcdManQ,xManSenQ,xManLcdQ,xTimManQ,xManTimQ);

	//Start Timer Task
	vStartTimerTask(1,xManTimQ, xTimManQ);	   //Give the timer an higher priority to get correct timing values

	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}


static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Enable UART0. */
    PCONP   |= (1 << 3);                 /* Enable UART0 power                */
    PINSEL0 |= 0x00000050;               /* Enable TxD0 and RxD0              */


	PCONP    |=  (1 << 7);                /* Enable power for I2C0              */

	/* Initialize pins for SDA (P0.27) and SCL (P0.28) functions                */
	PINSEL1  &= ~0x03C00000;
	PINSEL1  |=  0x01400000;

	/* Clear I2C state machine                                                  */
	I20CONCLR =  I2C_AA | I2C_SI | I2C_STA | I2C_I2EN;
	
	/* Setup I2C clock speed                                                    */
	I20SCLL   =  0x80;
	I20SCLH   =  0x80;
	
	I20CONSET =  I2C_I2EN;


	/* Initialise LCD hardware */
	lcd_hw_init();

	/* Setup LCD interrupts */
	PINSEL4 |= 1 << 26;				/* Enable P2.13 for EINT3 function */
	EXTMODE |= 8;					/* EINT3 edge-sensitive mode */
	EXTPOLAR &= ~0x8;				/* Falling edge mode for EINT3 */

	/* Setup VIC for LCD interrupts */
	VICIntSelect &= ~(1 << 17);		/* Configure vector 17 (EINT3) for IRQ */
	VICVectPriority17 = 15;			/* Set priority 15 (lowest) for vector 17 */
	VICVectAddr17 = (unsigned long)vLCD_ISREntry;
									/* Set handler vector */
}
