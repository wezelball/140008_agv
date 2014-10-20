#ifndef PROCEDURES_H_INCLUDED
#define PROCEDURES_H_INCLUDED
#endif
/* ^^ these are the include guards */

/* Function prototypes */

void error(char *);		// wrapper for perror
void bitWrite(int, int);	// abstracted bit write
int bitRead(int);			// abstracted bit read
int PWMWrite(int, int);	// abstracted PWM write
PI_THREAD (myThread);		// thread that runs loop timers