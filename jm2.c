#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#include "servoctl.h"
NUMSERVOS(3)

int32 CODE param_speed = 10;
int32 CODE param_port = 0;
int32 CODE param_pin = 0;

void main(void) {
	uint16 	speed = param_speed;
	uint8	port = param_port, 
			pin=param_pin;
	uint8 	ServoPos = 120;
	uint8 	dir=1;
	
	systemInit();
	usbInit();
	InitServos();
	
	EA=1; // Global interrupt enabled

	SetPin(0, port, pin);
	SetPin(1, 0, 1);
	SetPin(2, 0, 2);

	while (1) {
		delayMs(speed);
		usbComService();
		
		if (dir==1) {
			++ServoPos;
		}
		else {
			--ServoPos;
		}
		if (ServoPos > 253 || ServoPos < 2) {
			dir ^=1;
		}
		SetPos(0,ServoPos);
		SetPos(1,128);
		SetPos(2,220);
	}
}