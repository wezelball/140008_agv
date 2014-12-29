/*
* The code is released under the GNU General Public License.
* Developed by Mark Williams
* A guide to this code can be found here; http://ozzmaker.com/2013/04/22/845/
* Created 28th April 2013
*/


#include <unistd.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "L3G.h"
#include "LSM303.h"
#include "sensor.c"
#include "i2c-dev.h"
#include "140008lib.h"
#include "gyroprocedures.h"
#include "robotMap.h"


#define X   0
#define Y   1
#define Z   2

#define DT 0.02         // [s/loop] loop period. 20ms
#define AA 0.98         // complementary filter constant

#define A_GAIN 0.0573      // [deg/LSB]
#define G_GAIN 0.070     // [deg/s/LSB]
#define RAD_TO_DEG 57.29578
#define M_PI 3.14159265358979323846

extern robotPosition robot;

int mymillis()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000 + (tv.tv_usec)/1000;
}

int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;
    return (diff<0);
}

void updateAngles()
{

	float rate_gyr_y = 0.0;   // [deg/s]
	float rate_gyr_x = 0.0;    // [deg/s]
	float rate_gyr_z = 0.0;     // [deg/s]


	int  *Pacc_raw;
	int  *Pmag_raw;
	int  *Pgyr_raw;
	int  acc_raw[3];
	int  mag_raw[3];
	int  gyr_raw[3];

	Pacc_raw = acc_raw;
	Pmag_raw = mag_raw;
	Pgyr_raw = gyr_raw;


	float gyroXangle = 0.0;
	float gyroYangle = 0.0;
	float gyroZangle = 0.0;
	float AccYangle = 0.0;
	float AccXangle = 0.0;
	float CFangleX = 0.0;
	float CFangleY = 0.0;

	int startInt  = mymillis();
	struct  timeval tvBegin, tvEnd,tvDiff;

	signed int acc_y = 0;
	signed int acc_x = 0;
	signed int acc_z = 0;
	signed int gyr_x = 0;
	signed int gyr_y = 0;
	signed int gyr_z = 0;
	

	enableIMU();

	gettimeofday(&tvBegin, NULL);



	while(1)
	{
	startInt = mymillis();


	//read ACC and GYR data
	readMAG(Pmag_raw);
	readACC(Pacc_raw);
	readGYR(Pgyr_raw);

	//Convert Gyro raw to degrees per second
	rate_gyr_x = (float) *gyr_raw * G_GAIN;
	rate_gyr_y = (float) *(gyr_raw+1) * G_GAIN;
	rate_gyr_z = (float) *(gyr_raw+2) * G_GAIN;



	//Calculate the angles from the gyro
	gyroXangle+=rate_gyr_x*DT;
	gyroYangle+=rate_gyr_y*DT;
	gyroZangle+=rate_gyr_z*DT;




	//Convert Accelerometer values to degrees
	AccXangle = (float) (atan2(*(acc_raw+1),*(acc_raw+2))+M_PI)*RAD_TO_DEG;
	AccYangle = (float) (atan2(*(acc_raw+2),*acc_raw)+M_PI)*RAD_TO_DEG;


	//Change the rotation value of the accelerometer to -/+ 180
	if (AccXangle >180)
	{
		AccXangle -= (float)360.0;
	}
	if (AccYangle >180)
		AccYangle -= (float)360.0;

//      Complementary filter used to combine the accelerometer and gyro values.
	CFangleX=AA*(CFangleX+rate_gyr_x*DT) +(1 - AA) * AccXangle;
	CFangleY=AA*(CFangleY+rate_gyr_y*DT) +(1 - AA) * AccYangle;

	robot.gyroXangle = gyroXangle;
	robot.AccXangle = AccXangle;
	robot.CFangleX = CFangleX;
	robot.gyroYangle = gyroYangle;
	robot.AccYangle = AccYangle;
	robot.CFangleY = CFangleY;
	
	
	//fflush(stdout);
	//Each loop should be at least 20ms.
        while(mymillis() - startInt < 100)
        {
            usleep(100000);
        }

	//printf("Loop Time %d\t", mymillis()- startInt);
    }
}

void outputRobotPos() {
    printf("GyroX:  %f\n", robot.gyroXangle);
    printf("AccXangle:  %f\n", robot.AccXangle);
    printf("CFangleX: %f\n", robot.CFangleX);
    printf("GyroY:  %f\n", robot.gyroYangle);
    printf("AccYangle: %f\n", robot.AccYangle);
    printf("CFangleY: %f\n", robot.CFangleY);
}