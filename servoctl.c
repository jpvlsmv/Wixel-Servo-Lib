#include <cc2511_types.h>
#include <cc2511_map.h>
#include "servoctl.h"

#include <board.h>

volatile uint8 T3counter;
volatile uint8 LastServoPos=0;
	
extern SingleServo Servos[];

/*                                 t=0    t=.9      t=2.1                          t=20
 * The servo wants a waveform like ^------^------___^_________________________________
 * I've broken it up into          ALWHI TIKHI ALWAYSLOW
 * ALWAYSHIGH is the first 0.9ms, where the signal is always high.
 * Then we TICKINGHIGH for as long as necessary to position the servo
 * Then the rest of the 1.2ms to take us to a total of 2.1ms into the overall cycle (ALWAYSLOW)
 * Until we have counted 16 full time cycles, which is 19ms-ish for the overall cycle
 */
enum ServoTimerState { ALWAYSHIGH, TICKINGHIGH, ALWAYSLOW } ServoTimerState;

void T3_AlwaysHigh(void) {	// The signal has been high for 0.9ms
	register uint8 j, FirstServoPos = SERVO_NOPOS ;
	
	// We need to find the next servo to move to.
	for (j=0;j<NUMSERVOS;++j) {
		if ( (Servos[j].lastpos != SERVO_NOPOS) && (Servos[j].lastpos < FirstServoPos) ) {
			FirstServoPos = Servos[j].lastpos;
		}
	}
	
	// At this point, FirstServoPos is the smallest servo position.
	if (FirstServoPos == SERVO_NOPOS) {		// No servos have had positions set yet.
		ServoTimerState=TICKINGHIGH;
		return; 
	}
	
	// We'll set the timer to the smallest servo position, then modify
	// all the other servos to hold the relative positions
	for (j=0; j<NUMSERVOS;++j) {
		if ((Servos[j].lastpos!=0) && (Servos[j].lastpos != SERVO_NOPOS)) 
			Servos[j].lastpos -= FirstServoPos;
	}

	T3CC0 = FirstServoPos;
	ServoTimerState=TICKINGHIGH;
}

void T3_TickingHigh(void) {
	// At least one servo has met its timeout
	register uint8 j, NextServoPos=SERVO_NOPOS, pinmask; 
		
	for (j=0; j<NUMSERVOS;++j) {
		pinmask = ~(1 << Servos[j].pin);
		// Clear the pins for all servos that have lastpos == 0
		if (Servos[j].lastpos == 0) { // Then this one's time has past.  Lower it
			switch (Servos[j].port) {
				case 0: P0 &= pinmask; break;
				case 1: P1 &= pinmask; break;
				case 2: P2 &= pinmask; break;
			}
		}
		// And find the minimum non-zero lastpos
		else if ( Servos[j].lastpos < NextServoPos ) {
			NextServoPos = Servos[j].lastpos;
		}
	}
	
	if (NextServoPos == SERVO_NOPOS) {
		// Then there are no more servos to activate.  Kill the rest of the variable-time
		T3CC0 = 255 - LastServoPos;
		T3counter = 0;
		ServoTimerState=ALWAYSLOW;
		return;
	}
	
	// At least one more servo has time left
	for (j=0; j<NUMSERVOS;++j) {
		// Loop through and re-relativize the remaining posistions
		if ((Servos[j].lastpos!=0) && (Servos[j].lastpos != SERVO_NOPOS)) 
			Servos[j].lastpos -= NextServoPos;
	}
	if (NextServoPos < 10) NextServoPos = 10;
	
	T3CC0 = NextServoPos;
	ServoTimerState = TICKINGHIGH;
}

void T3_AlwaysLow(void) {
	register uint8 j;

	if (T3counter < 16) {
		T3CC0 = 209; // 1ms
		++T3counter;
	} else {

		// Here we reset the state of the servo systems data structure
		LastServoPos = 0;
		for (j=0;j<NUMSERVOS;++j) {
			Servos[j].lastpos = Servos[j].position; // Store the position (relatively atomically)

			// Do nothing else if the servo's not positioned
			if (Servos[j].lastpos == SERVO_NOPOS) continue;
			
			if (Servos[j].lastpos > LastServoPos) { // This is the highest of all servo positions
				LastServoPos = Servos[j].lastpos;	// which determines how long the post-high period is (255-this)
			}			
				
			switch (Servos[j].port) {				// And set the pin high for this cycle
				case 0: P0 |= (1 << Servos[j].pin); break;
				case 1: P1 |= (1 << Servos[j].pin); break;
				case 2: P2 |= (1 << Servos[j].pin); break;
			}
		}
		ServoTimerState=ALWAYSHIGH;
		T3counter=0;
		T3CC0 = 169; // 0.9ms
	}
}

void do_T3(void) {
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
	ServoTimerState = ALWAYSLOW;
	T3counter = 16;
	T3CC0=169; // 0.9ms
	T3IE=1;
    // DIV=   111: 1:128 prescaler
    // START=    1: Start the timer
    // OVFIM=     1: Enable the overflow interrupt.
    //             0: 
    // MODE=        10: Modulo
	T3CTL = 0b11111010;		
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
