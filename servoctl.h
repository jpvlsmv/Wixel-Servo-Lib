/**************************************************************
 * Servo Control library
 * (C) Joe Moore 2011
 * 
 * ************************************************************
 * This library controls one or more R/C servos from a wixel.
 * 
 * Each R/C servo is driven by a control pin.  PWM duty cycle determines
 * the position of the servo.  Normal servos (like the ones I have) 
 * move from 0 - 180 degrees based on a high-pulse of 0.9 - 2.1ms, out
 * of a 20ms cycle time.
 * 
 * This library takes over use of TIMER3, the only other 8-bit timer in the Wixel.
 * (Timer4 is used by time.h)
 * 
 * Basic use:
 * 		#define NUMSERVOS
 * 		#include servoctl.h
 * 		Call InitServos() to set up
 * 		Tell the library where the servos are connected with SetPin()
 * 
 * 		Set the position of each servo, with SetPos()
 */

#ifndef SERVOCTL_H_
#define SERVOCTL_H_


#define NUMSERVOS(x) 	DATA const unsigned char NUMSERVOS = x; SingleServo Servos[x]; \
						extern void do_T3(void); ISR(T3,1){do_T3();}
						
#define SERVO_NOPOS 255

typedef struct {
	volatile uint8 port;
	volatile uint8 pin;		// GPIO port to use
	volatile uint8 position;	// Next position the servo should be
	uint8 ticksremaining;	// Where the servo is moving to (do not use, maintained by the interrupt handler
} SingleServo;

extern DATA const uint8 NUMSERVOS;
extern SingleServo Servos[];	// Global servo variable

//------------------------------------
// Initialization routines
void InitServos(void);			// Set up the timer and initialize data structures, but do not start
								// outputting the PWM until positions are set.
void SetPin(uint8 servono, uint8 port, uint8 pin);
								// Associate a servo number to a GPIO pin.  Pass in P0_0 for example.

//------------------------------------
// General servo movement functions
void SetPos(uint8 servono, uint8 pos);
								// Tell servo to move to a particular location
								// Setting the position to SERVO_NOPOS will disable the servo channel.
								// Note that until the first time this is called, no PWM will go to any servo.
uint8 GetPos(uint8 servono);
								// Return the last SetPos.  This does not query the servo for position.

#endif /*SERVOCTL_H_*/
