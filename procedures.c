#include <stdio.h>
#include <stdlib.h>

#include "140008lib.h"
#ifdef RPI
	#include <wiringPi.h>
	#include <softPwm.h>
#endif
#include "procedures.h"
#include "robotMap.h"

/* These are declared in 140008_server.c */
extern bool firstTimeTracking;	// First iteration of line tracking
extern bool lineTracking;			// We are line tracking
/* 
 * Found in 140008lib.h, this is a struct that defines robot
 * in 2D space position according to the IMU 
 */
extern robotPosition robot;		

/*
 * Some of these variables should be boolean, would make code 
 * easier to read
 */ 

/* 
 * 0 if no yaw error (all sensors on)
 * 1 if yaw error clockwise
 * -1 if yaw error counterclockwise
 * 
 * This is okay to have 3 states, but defining them would make code
 * clearer - see my comment in 140008lib.h
 */
int yaw = 0;					// 0 if no yaw error, 1 if error
									// should name yawError instead
int yawCounter = 0;			// increments how drastic the yaw is
int translated = 0;			// same as yaw variable, 0 if no error, 1 if right, -1 if left.
int translateCounter = 0;	// increments how drastic the translation error is

/* 
 * 0 if no line error (all sensors on)
 * 1 is all sensors off
 * -1 if both rear sensors off (why this state?)
 * should be boolean
 */
int lineError = 0;										
int lineErrorSent = 0;		// Please add definition here - not 100% sure what this does
int prevLineSpeed = 0;		// defines what the prev line tracking rate was, and if
				//different reset the counters.
int motorArray[31][2];		// holds current motor rates and desired motor rates
				///only logically used in updateMotors(), and reset in softStop()

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(1);
}

/* Abstracted I/O functions*/

/* Write a single bit to an ouput */
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

/* Read a single bit from an input */
int bitRead(int pin)	{
	#ifdef RPI
		return (digitalRead(pin));
	#else
		printf("Simulated Digital Read: pin: %d, value: unknown\n", pin);
		return(0);
	#endif
}

/* Write an "analog" value to a PWM output */
int PWMWrite(int pin, int value)	{
	int scaledValue = value/9 + 15;
	#ifdef RPI
		softPwmWrite(pin, scaledValue);
	#else
		printf("PWM Write: pin: %d, value: %d\n", pin, scaledValue);
	#endif
	return scaledValue;
}

/* Stop all motors without shutting off isolation relay */
void softStop(void){
	PWMWrite(DRIVE_FR, 0);
	PWMWrite(DRIVE_FL, 0);
	PWMWrite(DRIVE_RR, 0);
	PWMWrite(DRIVE_RL, 0);
	int i = 0;
	
	/* resets the whole motorArray so if updateMotors is called
	 * it won't accidentally send a previous value.
	 */
	while(i < 32)
	{
		motorArray[i][0] = 0;
		motorArray[i][1] = 0;
		i++;
	}
	printf("Soft stopped the robot\n");
}

/* Stop all motors and shut off isolation relay */
void eStop(void){
	PWMWrite(DRIVE_FR, 0);
	PWMWrite(DRIVE_FL, 0);
	PWMWrite(DRIVE_RR, 0);
	PWMWrite(DRIVE_RL, 0);
	bitWrite(RELAY_ENABLE, 0);
}

/* Stop motion, clean up, and exit */
int agvShutdown(void) {
	lineTracking = false;
	firstTimeTracking = true;
	eStop();
	printf("Server Closing\n");
	return(0);
}

/*
 *  Call this procedure if the tracking flag is set to true
 *  Return a value of true if successful
 */

bool lineTrack(int speed) {
	// mag sensor values read once per loop iteration
	int magFL, magFR, magRL, magRR;
	// these are how much to add/subtract from each motor,
	//and tester holds the most negative value of the four
	// - if tester is negative, add it to all so all mod's are the same sign.
	int FLmod, FRmod, RLmod, RRmod, tester;
	int fwd = 1;	// robot is tracking in forward direction
	
	/* If you choose to track at a different rate without resetting,
	 * this resets all the counters to 0. 
	 * 
	 */
	if(prevLineSpeed != speed)
	{
		yawCounter = 0;
		yaw = 0;
		translateCounter = 0;
		translated = 0;
		lineError = 0;
	}
	
	/* Please explain the function of the following code 
	 * block 
	 */
	if (firstTimeTracking)
	{
		printf("Initializing motors\n");
		bitWrite(RELAY_ENABLE, 1);
		PWMWrite(DRIVE_FR, speed + DRIVE_FR_OFFSET);
		PWMWrite(DRIVE_FL, speed + DRIVE_FL_OFFSET);
		PWMWrite(DRIVE_RR, speed + DRIVE_RR_OFFSET);
		PWMWrite(DRIVE_RL, speed + DRIVE_RL_OFFSET);

		firstTimeTracking = false;
	}//this code is unnecessary now.
	
	// Just read the sensors once per loop
	
	/* going forward, read the sensors as defined */
	if(fwd == 1)
	{
		magFL = bitRead(MAG_FL);
		magFR = bitRead(MAG_FR);
		magRL = bitRead(MAG_RL);
		magRR = bitRead(MAG_RR);
	}
	/* going backwards, redefine the sensors in reverse order */
	else
	{
		magRR = bitRead(MAG_FL);	// Rear Right Rear now Front Left
		magRL = bitRead(MAG_FR);	// Rear Left now Front Right
		magFR = bitRead(MAG_RL);	// Front Right not Rear Left
		magFL = bitRead(MAG_RR);	// Front left now Rear Right
	}

	/*
	*  There are 16 different cases for sensors possibilities
	*
	*
	*/
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: proper\n");
		/* There is no line error, so set false - why not boolean? */
		if(lineError != 0)
		{
			printf("Line error fixed\n");
			lineError = 0;
		}
		if(yaw != 0)
		{
			printf("Yaw corrected\n");
			yaw = 0;
			yawCounter = 0;
		}
		if(translated != 0)
		{
			printf("Translation Corrected\n");
			translated = 0;
			translateCounter = 0;

		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw unknown both rear sensors off\n");
		if(yaw != 0)
		{
			if(yawCounter < abs(speed))
			{
				yawCounter = yawCounter + 1 + (abs(speed)/30);
			}
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: translated left\n");
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW translated left\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}

		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: translated right\n");
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated right\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw unknown both front sensors off\n");
		if(yaw != 0)
		{
			if(yawCounter < abs(speed))
			{
				yawCounter = yawCounter + 1 + (abs(speed)/30);
			}
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated left\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}

		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW translated right\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: All sensors off\n");
		lineError = 1;
		softStop();
		return false;
	}
	if(fwd == 1)
	{
		FRmod = yawCounter*yaw + translateCounter*translated;
		FLmod = -1*yawCounter*yaw - translateCounter*translated;
		RRmod = yawCounter*yaw - translateCounter*translated;
		RLmod = -1*yawCounter*yaw + translateCounter*translated;
		printf("FR: %d\nFL: %d\nRR: %d\nRL: %d\n", FRmod, FLmod, RRmod, RLmod);
		tester = FRmod;
		if(FLmod < tester)
		{
		tester = FLmod;
		}
		if(RRmod < tester)
		{
			tester = RRmod;
		}
		if(RLmod < tester)
		{
			tester = RLmod;
		}
		if(tester < 0)
		{
			FRmod = FRmod + tester;
			FLmod = FLmod + tester;
			RRmod = RRmod + tester;
			RLmod = RLmod + tester;
		}
		if(lineError != 0)
		{
			printf("lineError!\n");
			PWMWrite(DRIVE_FR, 0);
			PWMWrite(DRIVE_FL, 0);
			PWMWrite(DRIVE_RR, 0);
			PWMWrite(DRIVE_RL, 0);
		}
		else
		{
			printf("driving away\n");
			motorArray[DRIVE_FR][1] = speed + DRIVE_FR_OFFSET + FRmod;
			motorArray[DRIVE_FL][1] = speed + DRIVE_FL_OFFSET + FLmod;
			motorArray[DRIVE_RR][1] = speed + DRIVE_RR_OFFSET + RRmod;
			motorArray[DRIVE_RL][1] = speed + DRIVE_RL_OFFSET + RLmod;
		}
	}
	else
	{
		RLmod = yawCounter*yaw + translateCounter*translated;
		RRmod = -1*yawCounter*yaw - translateCounter*translated;
		FLmod = yawCounter*yaw - translateCounter*translated;
		FRmod = -1*yawCounter*yaw + translateCounter*translated;
		printf("FR: %d\nFL: %d\nRR: %d\nRL: %d\n", FRmod, FLmod, RRmod, RLmod);
		tester = FRmod;
		if(FLmod > tester)
		{
			tester = FLmod;
		}
		if(RRmod > tester)
		{
			tester = RRmod;
		}
		if(RLmod > tester)
		{
			tester = RLmod;
		}
		if(tester > 0)
		{
			FRmod = FRmod - tester;
			FLmod = FLmod - tester;
			RRmod = RRmod - tester;
			RLmod = RLmod - tester;
		}
		if(lineError != 0)
		{
			printf("lineError!\n");
			PWMWrite(DRIVE_FR, 0);
			PWMWrite(DRIVE_FL, 0);
			PWMWrite(DRIVE_RR, 0);
			PWMWrite(DRIVE_RL, 0);
		}
		else
		{
			motorArray[DRIVE_FR][1] = speed - DRIVE_FR_OFFSET - FRmod;
			motorArray[DRIVE_FL][1] = speed - DRIVE_FL_OFFSET - FLmod;
			motorArray[DRIVE_RR][1] = speed - DRIVE_RR_OFFSET - RRmod;
			motorArray[DRIVE_RL][1] = speed - DRIVE_RL_OFFSET - RLmod;
		}
	}
	prevLineSpeed = speed;
	updateMotors();
}


bool sideLineTrack(int speed) {
	// mag sensor values read once per loop iteration
	int magFL, magFR, magRL, magRR;
	int FLmod, FRmod, RLmod, RRmod, tester;
	int fwd = 1;
	if(prevLineSpeed != speed)
	{
		yawCounter = 0;
		yaw = 0;
		translateCounter = 0;
		translated = 0;
		lineError = 0;
	}
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
	if(fwd == 1)
	{
		magFL = bitRead(MAG_SFR);
		magFR = bitRead(MAG_SRR);
		magRL = bitRead(MAG_SFL);
		magRR = bitRead(MAG_SRL);
	}
	else
	{
		magRR = bitRead(MAG_SFR);
		magRL = bitRead(MAG_SRR);
		magFR = bitRead(MAG_SFL);
		magFL = bitRead(MAG_SRL);
	}

	/*
	*  There are 16 different cases for sensors possibilities
	*
	*
	*/
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: proper\n");
		if(lineError != 0)
		{
			printf("error fixed\n");
			lineError = 0;
		}
		if(yaw != 0)
		{
			printf("Yaw corrected\n");
			yaw = 0;
			yawCounter = 0;
		}
		if(translated != 0)
		{
			printf("Translation Corrected\n");
			translated = 0;
			translateCounter = 0;

		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw unknown both rear sensors off\n");
		if(yaw != 0)
		{
			if(yawCounter < abs(speed))
			{
				yawCounter = yawCounter + 1 + (abs(speed)/30);
			}
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: translated left\n");
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW translated left\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}

		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: translated right\n");
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated right\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw unknown both front sensors off\n");
		if(yaw != 0)
		{
			if(yawCounter < abs(speed))
			{
				yawCounter = yawCounter + 1 + (abs(speed)/30);
			}
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated left\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}

		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW translated right\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: All sensors off\n");
		lineError = 1;
		softStop();
		return false;
	}
	if(fwd == 1)
	{
		FRmod = -1*yawCounter*yaw - translateCounter*translated;
		FLmod = yawCounter*yaw + translateCounter*translated;
		RRmod = -1*yawCounter*yaw + translateCounter*translated;
		RLmod = yawCounter*yaw - translateCounter*translated;
		printf("FR: %d\nFL: %d\nRR: %d\nRL: %d\n", FRmod, FLmod, RRmod, RLmod);
		tester = FRmod;
		if(FLmod < tester)
		{
		tester = FLmod;
		}
		if(RRmod < tester)
		{
			tester = RRmod;
		}
		if(RLmod < tester)
		{
			tester = RLmod;
		}
		if(tester < 0)
		{
			FRmod = FRmod + tester;
			FLmod = FLmod + tester;
			RRmod = RRmod + tester;
			RLmod = RLmod + tester;
		}
		if(lineError != 0)
		{
			printf("lineError!\n");
			PWMWrite(DRIVE_FR, 0);
			PWMWrite(DRIVE_FL, 0);
			PWMWrite(DRIVE_RR, 0);
			PWMWrite(DRIVE_RL, 0);
		}
		else
		{
			motorArray[DRIVE_RR][1] = -1*(speed + DRIVE_FR_OFFSET + FRmod);
			motorArray[DRIVE_FR][1] = speed + DRIVE_FL_OFFSET + FLmod;
			motorArray[DRIVE_RL][1] = speed + DRIVE_RR_OFFSET + RRmod;
			motorArray[DRIVE_FL][1] = -1*(speed + DRIVE_RL_OFFSET + RLmod);
		}
	}
	else
	{
		RLmod = yawCounter*yaw - translateCounter*translated;
		RRmod = -1*yawCounter*yaw + translateCounter*translated;
		FLmod = yawCounter*yaw + translateCounter*translated;
		FRmod = -1*yawCounter*yaw - translateCounter*translated;
		printf("FR: %d\nFL: %d\nRR: %d\nRL: %d\n", FRmod, FLmod, RRmod, RLmod);
		tester = FRmod;
		if(FLmod > tester)
		{
			tester = FLmod;
		}
		if(RRmod > tester)
		{
			tester = RRmod;
		}
		if(RLmod > tester)
		{
			tester = RLmod;
		}
		if(tester > 0)
		{
			FRmod = FRmod - tester;
			FLmod = FLmod - tester;
			RRmod = RRmod - tester;
			RLmod = RLmod - tester;
		}
		if(lineError != 0)
		{
			printf("lineError!\n");
			PWMWrite(DRIVE_FR, 0);
			PWMWrite(DRIVE_FL, 0);
			PWMWrite(DRIVE_RR, 0);
			PWMWrite(DRIVE_RL, 0);
		}
		else
		{
			motorArray[DRIVE_RR][1] = -1*(speed - DRIVE_FR_OFFSET - FRmod);
			motorArray[DRIVE_FR][1] = speed - DRIVE_FL_OFFSET - FLmod;
			motorArray[DRIVE_RL][1] = speed - DRIVE_RR_OFFSET - RRmod;
			motorArray[DRIVE_FL][1] = -1*(speed - DRIVE_RL_OFFSET - RLmod);
		}
	}
	prevLineSpeed = speed;
	updateMotors();
}


bool RFIDTrack () {
	lineTrack(40);
	if(bitRead(999) == 1)
	{
		return false;//yay you found a tag! thats all it is for now
	}
	else
	{
		return true;
	}
}

bool updateMotors() {
	int i = 0;
	bool motorsUpdated = false;
	printf("update motors\n");
	while (i < 32)
	{
		if(abs(motorArray[i][0] - motorArray[i][1]) > 8)
		{
			if(motorArray[i][0] < motorArray[i][1])
			{
				motorArray[i][0] = motorArray[i][0] + 9;
			}
			else
			{
				motorArray[i][0] = motorArray[i][0] - 9;
			}
		}
		else
		{
			motorArray[i][0] = motorArray[i][1];
			motorsUpdated = true;
		}
		i++;
	}
	//currently set up for a 31 size array for ease of
	//access (so we can use drive port constants) whenever a motor
	//is added, just make sure you add a line here for the motor
	//output.
	if(lineError == 0)
	{
		PWMWrite(DRIVE_FR, motorArray[DRIVE_FR][0]);
		PWMWrite(DRIVE_FL, motorArray[DRIVE_FL][0]);
		PWMWrite(DRIVE_RR, motorArray[DRIVE_RR][0]);
		PWMWrite(DRIVE_RL, motorArray[DRIVE_RL][0]);
	}
	else
	{
		PWMWrite(DRIVE_FR, 0);
		PWMWrite(DRIVE_FL, 0);
		PWMWrite(DRIVE_RR, 0);
		PWMWrite(DRIVE_RL, 0);
		i = 0;
		while(i < 32)
		{
			motorArray[i][0] = 0;
			i++;
		}
	}
	printf("finished updating motors\n");
	return motorsUpdated;
}

int getSensorsPresent () {
	int sensorsTrue = 4;
	sensorsTrue = sensorsTrue - bitRead(MAG_FL) - bitRead(MAG_FR) - bitRead(MAG_RL) - bitRead(MAG_RR);
	return sensorsTrue;
}

int getSideSensorsPresent () {
	int sensorsTrue = 4;
	sensorsTrue = sensorsTrue - bitRead(MAG_SFL) - bitRead(MAG_SFR) - bitRead(MAG_SRL) - bitRead(MAG_SRR);
	return sensorsTrue;
}

bool checkAlignment() {
	int magFL, magFR, magRR, magRL;
	magFL = bitRead(MAG_FL);
	magFR = bitRead(MAG_FR);
	magRL = bitRead(MAG_RL);
	magRR = bitRead(MAG_RR);
	if(magFL == PRESENT && magFR == PRESENT && magRL == PRESENT && magRR == PRESENT)
	{
		return true;
		softStop();
	}
	else
	{
		return false;
	}
}

bool adjustAlignment () {
	int speed = 30;
	// mag sensor values read once per loop iteration
	int magFL, magFR, magRL, magRR;
	int FLmod, FRmod, RLmod, RRmod, tester;
	int fwd = 1;
	int parallelError = 0;
	if(prevLineSpeed != speed)
	{
		yawCounter = 0;
		yaw = 0;
		translateCounter = 0;
		translated = 0;
		lineError = 0;
	}
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
	if(fwd == 1)
	{
		magFL = bitRead(MAG_FL);
		magFR = bitRead(MAG_FR);
		magRL = bitRead(MAG_RL);
		magRR = bitRead(MAG_RR);
	}
	else
	{
		magRR = bitRead(MAG_FL);
		magRL = bitRead(MAG_FR);
		magFR = bitRead(MAG_RL);
		magFL = bitRead(MAG_RR);
	}

	/*
	*  There are 16 different cases for sensors possibilities
	*
	*
	*/
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: proper\n");
		if(lineError != 0)
		{
			printf("error fixed\n");
			lineError = 0;
		}
		if(yaw != 0)
		{
			printf("Yaw corrected\n");
			yaw = 0;
			yawCounter = 0;
		}
		if(translated != 0)
		{
			printf("Translation Corrected\n");
			translated = 0;
			translateCounter = 0;

		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw unknown both rear sensors off\n");
		if(fwd == -1)
		{
			/* 
			 * This is the only case where line error is set to -1
			 *	Should be boolean 
			 */
			lineError = -1;
		}
		parallelError = 1;
		printf("no this is parallelError!: %d\n", parallelError);
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: translated left\n");
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == PRESENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CW translated left\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}

		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: translated right\n");
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == PRESENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated right\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw unknown both front sensors off\n");
		if(fwd == 1)
		{
			lineError = 1;
		}
		parallelError = -1;
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == PRESENT && magRL == ABSENT)
	{
		printf("Sensor alignment: yaw CCW translated left\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != -1)
		{
			yaw = -1;
			yawCounter = 0;
		}

		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != -1)
		{
			translated = -1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == PRESENT)
	{
		printf("Sensor alignment: yaw CW translated right\n");
		if(yawCounter < abs(speed))
		{
			yawCounter = yawCounter + 1 + (abs(speed)/30);
		}
		if(yaw != 1)
		{
			yaw = 1;
			yawCounter = 0;
		}
		if(translateCounter < abs(speed))
		{
			translateCounter = translateCounter + 1 + (abs(speed)/30);
		}
		if(translated != 1)
		{
			translated = 1;
			translateCounter = 0;
		}
	}
	if (magFR == ABSENT && magFL == ABSENT && magRR == ABSENT && magRL == ABSENT)
	{
		printf("Sensor alignment: All sensors off\n");
		lineError = 1;
		softStop();
		return false;
	}
	FRmod = yawCounter*yaw + translateCounter*translated;
	FLmod = -1*yawCounter*yaw - translateCounter*translated;
	RRmod = yawCounter*yaw - translateCounter*translated;
	RLmod = -1*yawCounter*yaw + translateCounter*translated;
	printf("FR: %d\nFL: %d\nRR: %d\nRL: %d\n", FRmod, FLmod, RRmod, RLmod);
	if(lineError != 0)
	{
		printf("lineError!\n");
		PWMWrite(DRIVE_FR, 0);
		PWMWrite(DRIVE_FL, 0);
		PWMWrite(DRIVE_RR, 0);
		PWMWrite(DRIVE_RL, 0);
	}
	else
	{
		if(parallelError == 0)
		{
			motorArray[DRIVE_FR][1] = DRIVE_FR_OFFSET + FRmod;
			motorArray[DRIVE_FL][1] = DRIVE_FL_OFFSET + FLmod;
			motorArray[DRIVE_RR][1] = DRIVE_RR_OFFSET + RRmod;
			motorArray[DRIVE_RL][1] = DRIVE_RL_OFFSET + RLmod;
		}
		else
		{
			motorArray[DRIVE_FR][1] = parallelError*speed;
			motorArray[DRIVE_FL][1] = parallelError*speed;
			motorArray[DRIVE_RR][1] = parallelError*speed;
			motorArray[DRIVE_RL][1] = parallelError*speed;
		}
	}
	prevLineSpeed = speed;
	updateMotors();
	return checkAlignment();
}

void INThandler(int sig)
{
	eStop();
	printf("Emergency Shutdown Activated!\n");
	exit(0);
}
