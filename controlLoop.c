#include "controlLoop.h"
#include "robotMap.h"

/*
 * This is the main loop callback
 * 
 */
void masterLoopCallback(int leftSpeed, int rightSpeed)
{
	printf("Loop timer callback\n");
	PWMWrite(D_FR_LT, leftSpeed);
	printf("leftSpeed: %d rightSpeed: %d\n", leftSpeed, rightSpeed);
	PWMWrite(D_FR_RT, rightSpeed);
	PWMWrite(D_RR_LT, leftSpeed);
	PWMWrite(D_RR_RT, rightSpeed);
}
