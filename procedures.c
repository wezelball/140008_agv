#include <stdio.h>
#include <stdlib.h>
#include "140008lib.h"
#include <wiringPi.h>
#include <softPwm.h>
#include "procedures.h"
#include "robotMap.h"

// These are declared in 140008_server.c
extern bool firstTimeTracking;
extern bool lineTracking;

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

/* Abstracted I/O functions*/
void bitWrite(int pin, int value)	{
	#ifdef RPI
		if (value == 0)	{
			digitalWrite(pin, LOW);
			printf("Digital Write, pin %d, value: %d\n", pin, LOW);
		}

		if (value == 1)	{
			digitalWrite(pin, HIGH);
			printf ("Digital Write, pin %d, value: %d\n", pin, HIGH);
		}
	#else
		printf("Simulated Digital Write: pin: %d, value: %d\n", pin, value);
	#endif
} 

int bitRead(int pin)	{
	#ifdef RPI
		return (digitalRead(pin));
	#else
		printf("Simulated Digital Read: pin: %d, value: unknown\n", pin);
		return(0);
	#endif
}

int PWMWrite(int pin, int value)	{
	int scaledValue = value/9 + 15;
	#ifdef RPI
		softPwmWrite(pin, scaledValue);
	#else
		printf("PWM Write: pin: %d, value: %d\n", pin, scaledValue);
	#endif
	return scaledValue;
}

/*
 * Executes acceleration ramp for a given motor
 * Needs to alert a loop callback to increment 
 * motor speed values in the proper fashion
 *  
 */
int motorRamp(int motor, int finalSpeed, int accel)	{
	printf("You asked for ramp for motor %d spped %d accel %d\n", motor, finalSpeed, accel);
	printf("Don't know what the hell to do here yet..\n");
	return 0;
}

// Stop all motors without shutting off isolation relay
void softStop(void){
	PWMWrite(DRIVE_FR, 0);
	PWMWrite(DRIVE_FL, 0);
	PWMWrite(DRIVE_RR, 0);
	PWMWrite(DRIVE_RL, 0);
}

// Stop all motors and shut off isolation relay
void eStop(void){
	PWMWrite(DRIVE_FR, 0);
	PWMWrite(DRIVE_FL, 0);
	PWMWrite(DRIVE_RR, 0);
	PWMWrite(DRIVE_RL, 0);
	bitWrite(RELAY_ENABLE, 0);
}

// Stop motion, clean up, and exit 
void agvShutdown(void) {
	lineTracking = false;
	eStop();
	printf("Server Closing\n");
	return(0);
}


/*
 *  Call this procedure if the tracking flag is set to true
 * 	Return a value of ? if successful
 */

int lineTrack(int speed) {
	// mag sensor values read once per loop iteration
	int magFL, magFR, magRL, magRR; 
	
	if (firstTimeTracking)
	{
		printf("Initializing motors\n");
		bitWrite(RELAY_ENABLE, 1);
		PWMWrite(DRIVE_FR, speed + DRIVE_FR_OFFSET);
		PWMWrite(DRIVE_FL, speed + DRIVE_FL_OFFSET);
		PWMWrite(DRIVE_RR, speed + DRIVE_RR_OFFSET);
		PWMWrite(DRIVE_RL, speed + DRIVE_RL_OFFSET);
		
		firstTimeTracking = false;
	}

	// Just read the sensors once per loop
	magFL = bitRead(MAG_FL);
	magFR = bitRead(MAG_FR);
	magRL = bitRead(MAG_RL);
	magRR = bitRead(MAG_RR);

	/*
	*  There are 16 different cases for sensors possibilities
	* 
	* 
	*/
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: proper\n");
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW\n");
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw unknown both rear sensors off\n");
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: translated left\n");
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW translated left\n");
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW\n");
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW\n");
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: translated right\n");
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated right\n");
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw unknown both front sensors off\n");
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated left\n");
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW translated right\n");
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: All sensors off\n");
	}
}
