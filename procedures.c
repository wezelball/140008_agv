#include <stdlib.h>
#include <time.h>
#include "140008lib.h"
#include <wiringPi.h>
#include <softPwm.h>
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
		printf("Digital Read: pin: %d, value: unknown\n", pin);
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
* This is where the timing loop for the master clock is generated
* it currently calls one control loop, but could be 
* extended in the future
*/
PI_THREAD (myThread)	{
	clock_t startClock, finishClock; // for checking elapsed time
	double elapsedTime; 	// time in seconds for master clock
	bool timing = false;	// timing flag 
	float loopTime = 0.050;	// Control loop time
	int i_thread = 0;	// test variable
	while(true)
	{
		if (timing == false){
			startClock = clock();
			timing = true;
		}
		finishClock = clock();
		elapsedTime = ((double)(finishClock - startClock)/CLOCKS_PER_SEC);
		if (elapsedTime >= loopTime) {
			//printf("%f\n", elapsedTime);
			loopTimerCallback();
			timing = false;
		}
	}
}
