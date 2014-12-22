#include <stdio.h>
#include <stdlib.h>

#include "140008lib.h"
#ifdef RPI
	#include <wiringPi.h>
	#include <softPwm.h>
#endif
#include "procedures.h"
#include "robotMap.h"

// These are declared in 140008_server.c
extern bool firstTimeTracking;
extern bool lineTracking;

int yaw = 0;
int yawCounter = 0;
int translated = 0;
int translateCounter = 0;
int lineError = 0;
int lineErrorSent = 0;
int prevLineSpeed = 0;
int motorArray[31][2];

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
	int i = 0;
	while(i < 32)
	{
		motorArray[i][0] = 0;
		motorArray[i][1] = 0;
		i++;
	}
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
int agvShutdown(void) {
	lineTracking = false;
	firstTimeTracking = true;
	eStop();
	printf("Server Closing\n");
	return(0);
}


/*
 *Just a basic mecanum drive, all drive should go through
 *this function.
 *
 *@param xSpeed how much the robot should move in the X
 * direction
 *
 *@param ySpeed how much the robot should move in the Y
 * direction
 *
 *@param twist at what rate the robot should be twisting
 */
//void mecanumDrive (int xSpeed, int ySpeed, int twist) {
	//you are on a new level of quietness today
	//int magnitude = (sqrt(xSpeed) + sqrt(ySpeed)^2;
	//int theta = tan(x/y);
	//if(x > 0 && y > 0)
	//{
		/*im just gonna write some pseudo code
		
		front right and rear left = magnitude;
		front left and rear right = magnitude*(1 - theta/45);
		
		*/
	//}
	//else
	//{
		/*
		
		front left and rear right = magnitude;
		front right and rear left = magnitude*(1 - theta/45);		
		*/
	//}
	/*
	front left = front left + twist;
	rear left = rear left + twist;
	front right = front right - twist;
	rear right = rear right - twist;
	if any of the above four numbers are now greater than 100 or
	less than -100, then scale them all back so that that 
	respective number is either -100 or 100
	*/
//}


/*
 *  Call this procedure if the tracking flag is set to true
 * 	Return a value of ? if successful
 */

bool lineTrack(int speed) {
	// mag sensor values read once per loop iteration
	if(lineTracking)
	{
		printf("line tracking true in proc\n");
	}
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
	if(speed < 0)
	{
		fwd = -1;
		printf("I think we're going reverse because fwd = %d\n", fwd);
	}
	else
	{
		printf("I think we're going forwards because fwd = %d\n", fwd);
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
			lineError = -1;
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
		//translate to the left
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
		printf("driving forward! and fwd = %d\n", fwd);
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
		printf("driving reverse!\n");
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
			
			printf("driving away\n");
			motorArray[DRIVE_FR][1] = speed - DRIVE_FR_OFFSET - FRmod;
			motorArray[DRIVE_FL][1] = speed - DRIVE_FL_OFFSET - FLmod;
			motorArray[DRIVE_RR][1] = speed - DRIVE_RR_OFFSET - RRmod;
			motorArray[DRIVE_RL][1] = speed - DRIVE_RL_OFFSET - RLmod;
		}
	}
	prevLineSpeed = speed;
	updateMotors();
	if(lineTracking == true)
	{
		printf("line tracking true at end of proc\n");
	}
	else
	{
		printf("line tracking false at end of proc\n");
	}
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

void updateMotors() {
	int i = 0;
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
		}
	}
	printf("finished updating motors\n");
}