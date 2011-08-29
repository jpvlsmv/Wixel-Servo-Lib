#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#include "servoctl.h"
MAXSERVOS(1)

volatile uint8 ServoPos = 120;

uint8 dir=1;

void main(void) {
	systemInit();
	usbInit();
	InitServos();
	
	EA=1; // Global interrupt enabled
		
	while (1) {
		delayMs(20);
		LED_RED(!LED_RED_STATE);
		if (dir==1)
			++ServoPos;
		if (dir==0)
			--ServoPos;
		if (ServoPos > 250 || ServoPos < 5) dir ^=1;
		boardService();
		usbComService();
	}
}