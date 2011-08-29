#include <cc2511_types.h>
#include <cc2511_map.h>
#include <board.h> // For LED_YELLOW() macro
#include "servoctl.h"

volatile int32 T3count;
volatile int8 T3state;
volatile uint8 T3counter;
volatile uint8 T3LastPos;
extern volatile uint8 ServoPos;
extern SingleServo Servos;

void do_T3(void) {
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
			T3CC0 = 255 - T3LastPos;
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


// Initialization routines
void InitServos(void) {
}

void SetPin(unsigned char servono, unsigned char pin) {
	servono = 1; pin=1;
}

void SetPos(unsigned char servono, unsigned char pos) {
	servono=1; pos=1;
}

void GetPos(unsigned char servono) {
	servono=1;
}
