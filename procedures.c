#include <stdio.h>
#include <stdlib.h>
#include "140008lib.h"
#include <wiringPi.h>
#include <softPwm.h>
#include "procedures.h"

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
