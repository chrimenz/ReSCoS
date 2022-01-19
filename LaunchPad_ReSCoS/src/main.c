/**************************************************************************************************
  Filename:       main.c
  Revised:        $Date: 2013-02-20 17:37:33 +0100 (Mi, 20 Feb 2013) $
  Revision:       $Revision: 54 $
  Author:         $Author: Menz $

  Description:    minimal example for scheduler on MSP launchpad

**************************************************************************************************/

/*! @file main.c */

#include <msp430.h>

#include "inc/scheduler.h"
#include "inc/vcom.h"


/* local tasks */
static void Task1(void);
static void Task2(void);
/* init */
static void msp_init(void);
static void vInitSystickTimer(void);
/* handler for received bytes -> must be set with "setByteReceivedHandler(...)" defined in vcom */
static void ByteReceived(unsigned char b);

/* task ids can be used to manipulate tasks, set to SCDL_NA to prevent faults */
taskID_t tidTask2 = SCDL_NA;

/*! **********************************************************************************
 * @fn		main
 *
 * @brief	minimal example for presenting how the scheduler works on the MSP-LaunchPad
 * 			Task1 toggles LED1 with period of 1000ms
 * 			Task2 toggles LED2 with period of 250ms
 * 			vTaskVCOMBuffered handles bytes sent on debug uart
 * 			- a received handler is set, which invokes some functions depending on the received command ('1','2','3')
 * 			- '1' - Task2 is paused
 * 			- '2' - Task2 is set to ready
 * 			- '3' - Task2 is starts again in 2s
 * 			- '4' - Task2 period is set to 500ms
 *
 * @return	exit code (shoul not happen)
 */
int main(void)
{
	/* init wdt and osc */
	msp_init();
	/* init timer to generate a 1ms interrupt */
	vInitSystickTimer();
	/* enable interrupts */
	__bis_SR_register(GIE);


	/* create some tasks */
	tidCreateTask(Task1, 1000);
	tidTask2 = tidCreateTask(Task2, 250);
	tidCreateTask(vTaskVCOMBuffered, 50);
	/* set an event handler for receiving bytes */
	setByteReceivedHandler(ByteReceived);

	/* start the scheduler */
	vStartScheduler();
	

	/* should not get here */
	return 1;
}

/* toggle LED1*/
void Task1(void)
{
	P1DIR |= 0x01;
	P1OUT ^= 0x01;
}

/* toggle LED2*/
void Task2(void)
{
	P1DIR |= 0x40;
	P1OUT ^= 0x40;
}

/* byte reveived event handler */
static void ByteReceived(unsigned char b)
{
	switch(b)
	{
	case '1':
		vTaskSetState(tidTask2, OFF);
		ucVCOM_LogString("OFF\n",4);
		break;
	case '2':
		vTaskSetState(tidTask2, READY);
		ucVCOM_LogString("READY\n",6);
		break;
	case '3':
		vTaskInvokeDelayed(tidTask2, 2000);
		ucVCOM_LogString("DELAY\n",6);
		break;
	case '4':
		vTaskSetPeriod(tidTask2, 500);
		ucVCOM_LogString("PERIOD\n",7);
		break;
	}
}

void msp_init(void)
{
	/* Stop WDT */
	WDTCTL = WDTPW + WDTHOLD;

	/* configure internal digitally controlled oscillator to 8MHz*/
	DCOCTL  = CALDCO_8MHZ;
	BCSCTL1 = CALBC1_8MHZ;
}

void vInitSystickTimer(void)
{
  	//compare interrupt
	TACCTL0 |= CCIE;
	//set timer compare to 1000d -> 1/8MHz * 8 * 1000 = 1ms
	TACCR0 = 1000;
	//configure timer A with subsystemclock and up-mode
	TACTL = TASSEL_2 | MC_1 | ID0 | ID1;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0 (void)
{
	/* function must be called every ms */
	vScdlTick1ms();
}
