/*
 * main.c
 */

/* low level */
#include "inc/lm4f120h5qr.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
/* driverlib */
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
/* addons */
#include "utils/uartstdio.h"
#include "boards/ek-lm4f120xl/drivers/buttons.h"
/* project */
#include "inc/scheduler.h"



#define TICKS_PER_SECOND 		1000

void init(void);
void vTaskLED1(void);
void vTaskLED2(void);
void vTaskLED3(void);
void vTaskButton(void);
void vTaskUARTReceive(void);

int main(void) {

	init();

	tidCreateTask(vTaskLED1,1000);
	tidCreateTask(vTaskLED2,500);
	tidCreateTask(vTaskLED3,2000);
	tidCreateTask(vTaskButton,25);
	tidCreateTask(vTaskUARTReceive,50);

	vStartScheduler();
	return 0;
}


void vTaskLED1(void)
{
	GPIO_PORTF_DATA_R ^= 0x02;
}

void vTaskLED2(void)
{
	GPIO_PORTF_DATA_R ^= 0x04;
}

void vTaskLED3(void)
{
	GPIO_PORTF_DATA_R ^= 0x08;
}

void vTaskButton(void)
{
	static unsigned char delta, state;
	unsigned long bstate = ButtonsPoll(&delta,&state);

	if(delta & bstate & LEFT_BUTTON)
	{
		UARTprintf("Left\n");
	}
	else if(delta & bstate & RIGHT_BUTTON)
	{
		UARTprintf("Right\n");
	}
}

void vTaskUARTReceive(void)
{
	unsigned char rxb;

	while(UARTCharsAvail(UART0_BASE))
	{
		rxb = UARTCharGet(UART0_BASE);
		switch(rxb)
		{
		case '1':
			UARTprintf("Hello\n");
			break;

		}
	}
}

void InitConsole(void)
{

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioConfig(0, 9600, SysCtlClockGet());
}


void vInitLED(void)
{
    volatile unsigned long ulLoop;

    SYSCTL_RCGC2_R = SYSCTL_RCGC2_GPIOF;
    ulLoop = SYSCTL_RCGC2_R;

    GPIO_PORTF_DIR_R = 0x08;
    GPIO_PORTF_DEN_R = 0x08;

    GPIO_PORTF_DIR_R |= 0x04;
    GPIO_PORTF_DEN_R |= 0x04;

    GPIO_PORTF_DIR_R |= 0x02;
    GPIO_PORTF_DEN_R |= 0x02;
}

void init(void)
{

    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    InitConsole();

    vInitLED();

    ButtonsInit();

    SysTickPeriodSet(SysCtlClockGet()/TICKS_PER_SECOND);

    IntMasterEnable();

    SysTickIntEnable();

    SysTickEnable();
}


void SysTickIntHandler(void)
{
	vScdlTick1ms();
}
