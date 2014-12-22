#ifndef PROCEDURES_H_INCLUDED
#define PROCEDURES_H_INCLUDED
#endif
/* ^^ these are the include guards */

/* Function prototypes */

void error(char *);		// wrapper for perror
void bitWrite(int, int);	// abstracted bit write
int bitRead(int);			// abstracted bit read
int PWMWrite(int, int);	// abstracted PWM write
int motorRamp(int, int, int); //motor speed ramp
bool lineTrack(int);
void softStop(void);
void eStop(void);
int agvShutdown(void);
void updateMotors(void);

