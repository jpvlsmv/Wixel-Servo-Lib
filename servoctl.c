#include <cc2511_types.h>
#include <cc2511_map.h>
#include "servoctl.h"

// Clear Timer3 so that it starts counting down again
#define T3SET(x) T3CC0=(x); T3CTL |= 0b00000100;		


volatile DATA uint8 T3counter;
volatile DATA uint8 TicksAfterServos;
	
extern SingleServo Servos[];

/*                                 t=0    t=.9      t=2.1                          t=20
 * The servo wants a waveform like ^------^------___^______________[...]______________^
 * I've broken it up into T3counter(c)==17     c--  c==16 ...                      c==0
 */

void do_T3(void) {
	register uint8 j, minfound;
	if (T3counter == 0) { // It's the end of the servo pulse cycle, reset everything to the default
		for (j=0; j<NUMSERVOS ; ++j) {
			Servos[j].ticksremaining = Servos[j].position;
			if (Servos[j].ticksremaining == 255) {
				// Set the pin to input and turn off pullups
				switch (Servos[j].port) {
					case 0: P0DIR&=~(1<<Servos[j].pin);P0&=~(1<<Servos[j].pin);break;
					case 1: P1DIR&=~(1<<Servos[j].pin);P1&=~(1<<Servos[j].pin);break;
					case 2: P2DIR&=~(1<<Servos[j].pin);P2&=~(1<<Servos[j].pin);break;
				}
			} else {
				// Set the pin to output and high
				switch (Servos[j].port) {
					case 0: P0DIR|=(1<<Servos[j].pin);P0|=(1<<Servos[j].pin);break;
					case 1: P1DIR|=(1<<Servos[j].pin);P1|=(1<<Servos[j].pin);break;
					case 2: P2DIR|=(1<<Servos[j].pin);P2|=(1<<Servos[j].pin);break;
				}
			}
		}
		TicksAfterServos = 255;
		T3counter = 17;
		// Set the timer for 0.9ms and done
		T3SET(169);
	} else if (T3counter == 17) { // We're in the high part of the cycle.  And will stay here until there are
	                              // are no more active servos
	    minfound=SERVO_NOPOS;
		for (j=0;j<NUMSERVOS;++j) {
			if (Servos[j].ticksremaining == 0) {
				switch (Servos[j].port) {
					// Bring the pin low
					case 0: P0&= ~(1<<Servos[j].pin); break;
					case 1: P1&= ~(1<<Servos[j].pin); break;
					case 2: P2&= ~(1<<Servos[j].pin); break;
				}
				// And ignore it the rest of the time we're looking
				Servos[j].ticksremaining = SERVO_NOPOS;
			}
			// Note that if it was 0 before, now it's NOPOS so it can't be the min
			if (Servos[j].ticksremaining < minfound) {
				minfound = Servos[j].ticksremaining;
			}
		}
		if (minfound != SERVO_NOPOS) {
			for (j=0;j<NUMSERVOS;++j) {
				if (Servos[j].ticksremaining != SERVO_NOPOS)
					Servos[j].ticksremaining -= minfound;
			}
			TicksAfterServos -= minfound;
			T3SET(minfound);
		} else {
			T3counter--; // We're going into the low period.
			T3SET(TicksAfterServos); // Finish to the 2.1ms point in the cycle
		}
	} else {
		// We're in the later low-time period. 0 < T3counter < 17
		--T3counter;
		T3SET(209); // Check back in 1ms
	}
}


void InitServos(void) {
	uint8 i;
	// Clear the contents of the structure
	for (i=0; i<NUMSERVOS; ++i) {
		Servos[i].position=SERVO_NOPOS;
		Servos[i].pin=SERVO_NOPOS;
		Servos[i].port=SERVO_NOPOS;
	}

	// Set up the servo timer
	// DIV=   111: 1:128 prescaler
    // START=    1: Start the timer
    // OVFIM=     1: Enable the overflow interrupt.
    // START=      0: Don't reset the timer right now
    // MODE=        10: Modulo
	T3CTL = 0b11111010;
	
//	ServoTimerState = ALWAYSLOW;
	T3counter = 17;
	T3IE=1;
	T3SET(169);		
}

void SetPin(uint8 servono, uint8 port, uint8 pin) {
	Servos[servono].port = port;
	Servos[servono].pin = pin;
	switch(port) {
		case 0: P0DIR |= (1<<pin); break;
		case 1: P1DIR |= (1<<pin); break;
		case 2: P2DIR |= (1<<pin); break;
	}		
}

void SetPos(uint8 servono, uint8 pos) {
	if (pos < 2) pos = 2;
	Servos[servono].position= pos;
}

uint8 GetPos(uint8 servono) {
	return Servos[servono].position;
}
