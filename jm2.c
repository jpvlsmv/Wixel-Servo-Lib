#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#define MAX(x,y) ((x)>(y))?(x):(y)
#define MIN(x,y) ((x)<(y))?(x):(y)

#include "servoctl.h"
MAXSERVOS(1)

volatile uint8 ServoPos = 120;

#if 0
ISR(T3,1) {
	++T3count;
	switch(T3state) {
		case 0: // Always-on finished, so now we set the on-time for the servo
			// Set T3CCR to ServoPos
			T3LastPos = ServoPos;
			if (T3LastPos > 250) T3LastPos = 250;
			if (T3LastPos < 5) T3LastPos = 5;
			T3CC0 = T3LastPos;
			// Now in state 1
			LED_YELLOW(1);
			T3state=1;
			break;
		case 1: // The servo high window has passed, next we have some time to catch up to the 2.2ms point
			// T3CCR 
			T3CC0 = MIN(255 - T3LastPos,3);
			LED_YELLOW(0);
			T3state=2;
			break;
		case 2: // Delaying after Servo.  Now we wait the other 17.8 ms.
			if (T3counter < 16) {
				T3CC0 = 209;
				++T3counter;
			} else { // We're at the end of the 20ms Servo period
				T3state=0;
				T3counter=0;
				T3CC0 = 169;
				LED_YELLOW(1);
			}
			break;
	}
}
#endif // 0

int8 dir=1;

void main(void) {
	systemInit();
	usbInit();
	InitServos();
	
	T3CC0=169;
	T3IE=1;
    // DIV=111: 1:128 prescaler
    // START=1: Start the timer
    // OVFIM=1: Enable the overflow interrupt.
    // MODE=10: Modulo
	T3CTL = 0b11111010;
	
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