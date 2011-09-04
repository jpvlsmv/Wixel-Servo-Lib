#include <cc2511_types.h>
#include <cc2511_map.h>
#include "servoctl.h"

#include <board.h>

// Clear Timer3 so that it starts counting down again
#define T3SET(x) T3CC0=(x); T3CTL |= 0b00000100;		


volatile DATA uint8 T3counter;
volatile DATA uint8 HighServoPos=0;
	
extern SingleServo Servos[];

/*                                 t=0    t=.9      t=2.1                          t=20
 * The servo wants a waveform like ^------^------___^_________________________________
 * I've broken it up into          ALWHI TIKHI ALWAYSLOW
 * ALWAYSHIGH is the first 0.9ms, where the signal is always high.
 * Then we TICKINGHIGH for as long as necessary to position the servo
 * Then the rest of the 1.2ms to take us to a total of 2.1ms into the overall cycle (ALWAYSLOW)
 * Until we have counted 16 full time cycles, which is 19ms-ish for the overall cycle
 */
register enum ServoTimerState { ALWAYSHIGH, TICKINGHIGH, ALWAYSLOW } ServoTimerState;

void T3_AlwaysHigh(void) {	// The signal has been high for 0.9ms
	register uint8 j, FirstServoPos;
	FirstServoPos = SERVO_NOPOS ;
	
	// We need to find the next servo to move to.
	for (j=0;j<NUMSERVOS;++j) {
		if ( (Servos[j].ticksremaining != SERVO_NOPOS) && (Servos[j].ticksremaining < FirstServoPos) ) {
			FirstServoPos = Servos[j].ticksremaining;
		}
	}
	
	// At this point, FirstServoPos is the smallest servo position.
	if (FirstServoPos == SERVO_NOPOS) {		// No servos have had positions set yet.
		ServoTimerState=TICKINGHIGH;
		HighServoPos=255;
		T3SET(255);
		return; 
	}
	
	// We'll set the timer to the smallest servo position, then modify
	// all the other servos to hold the relative positions
	for (j=0; j<NUMSERVOS;++j) {
		if ((Servos[j].ticksremaining!=0) && (Servos[j].ticksremaining != SERVO_NOPOS)) 
			Servos[j].ticksremaining -= FirstServoPos;
	}

	ServoTimerState=TICKINGHIGH;
	T3SET(FirstServoPos);
}

void T3_TickingHigh(void) {
	// At least one servo has met its timeout
	register uint8 j, NextServoPos=SERVO_NOPOS, pinmask; 
		
	for (j=0; j<NUMSERVOS;++j) {
		pinmask = ~(1 << Servos[j].pin);
		// Clear the pins for all servos that have ticksremaining == 0
		if (Servos[j].ticksremaining == 0) { // Then this one's time has past.  Lower it
			switch (Servos[j].port) {
				case 0: P0 &= pinmask; break;
				case 1: P1 &= pinmask; break;
				case 2: P2 &= pinmask; break;
			}
		}
		// And find the minimum non-zero ticksremaining
		else if ( Servos[j].ticksremaining < NextServoPos ) {
			NextServoPos = Servos[j].ticksremaining;
		}
	}
	
	if (NextServoPos == SERVO_NOPOS) {
		// Then there are no more servos to activate.  Kill the rest of the variable-time
		T3counter = 0;
		ServoTimerState=ALWAYSLOW;
		T3SET(255-HighServoPos);		
		return;
	}
	
	// At least one more servo has time left
	for (j=0; j<NUMSERVOS;++j) {
		// Loop through and re-relativize the remaining posistions
		if ((Servos[j].ticksremaining!=0) && (Servos[j].ticksremaining != SERVO_NOPOS)) 
			Servos[j].ticksremaining -= NextServoPos;
	}
	
	ServoTimerState = TICKINGHIGH;
	T3SET(NextServoPos);		
}

void T3_AlwaysLow(void) {
	register uint8 j, pinmask;

	if (T3counter < 16) {
		++T3counter;
		T3SET(209);		
		
	} else {

		// Here we reset the state of the servo systems data structure
		HighServoPos = 0;
		for (j=0;j<NUMSERVOS;++j) {
			Servos[j].ticksremaining = Servos[j].position; // Store the position (relatively atomically)

			// If the servo has been positioned,
			if (Servos[j].ticksremaining != SERVO_NOPOS) {
				
				// Find the largest position.  That'll be used to determine how long to go before
				// counting whole milliseconds in the LOW state
				if (Servos[j].ticksremaining > HighServoPos) { // This is the highest of all servo positions
					HighServoPos = Servos[j].ticksremaining;   // which determines how long the post-high period is (255-this)
				}			
					
				pinmask = 1 << Servos[j].pin;
				switch (Servos[j].port) {				// And set the pin high for this cycle
					case 0: P0 |= pinmask; break;
					case 1: P1 |= pinmask; break;
					case 2: P2 |= pinmask; break;
				}
			}
		}
		ServoTimerState=ALWAYSHIGH;
		T3counter=0;
		T3SET(169);		
	}
}

void do_T3(void) {
	LED_YELLOW(1);
	switch(ServoTimerState) {
		case ALWAYSHIGH:
			T3_AlwaysHigh();
			break;
		case TICKINGHIGH:
			T3_TickingHigh();
			break;
		case ALWAYSLOW:
			T3_AlwaysLow();
			break;
	}
	LED_YELLOW(0);
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
	
	ServoTimerState = ALWAYSLOW;
	T3counter = 16;
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
