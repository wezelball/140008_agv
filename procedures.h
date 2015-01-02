#ifndef PROCEDURES_H_INCLUDED
#define PROCEDURES_H_INCLUDED
#endif
/* ^^ these are the include guards */

/* Function prototypes */

void error(char *);		// wrapper for perror
void bitWrite(int, int);	// abstracted bit write
int bitRead(int);			// abstracted bit read
int PWMWrite(int, int);	// abstracted PWM write
void softStop(void);
void eStop(void);
int agvShutdown(void);
bool lineTrack(int);
bool sideLineTrack(int);
bool RFIDTrack(void);
bool updateMotors(void);
int getSensorsPresent(void);
int getSideSensorsPresent(void);
bool checkAlignment(void);
bool adjustAlignment(void);
void INThandler(int);
void driveStraight(int);
