/* 
 * 140008_server.c - A simple TCP echo server 
 * usage: 140008_server <port>
 */

/* System includes */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

/* Local includes */
#include "robotMap.h"
#include "140008lib.h"
#include "procedures.h"

/* Includes for RPi only*/
#ifdef RPI
	#include <wiringPi.h>
	#include <softPwm.h>
#endif

#define BUFSIZE 1024
#define MSGSIZE 32

/* Global variables (thou art evil) */
int lt_motor_speed = 0; //front left motor
int rt_motor_speed = 0; //front right motor
// True if we are line tracking
bool lineTracking = false;
// Need to initialize tracking for first scan
bool firstTimeTracking = true;
// Now running under joystick control
bool joystickControl = false;
int lineTrackSpeed = 0; // the speed to run linetrack function at
bool sideLineTracking = false;
int sideLineTrackSpeed = 0;
bool RFIDTracking = false; //whether the cart is currently RFID tracking
bool properAlignment = true;
clock_t joyStartClock;
/*
* This is where the timing loop for the master clock is generated
* it currently calls one control loop, but could be 
* extended in the future
*/
PI_THREAD (myThread)	{
	clock_t joyFinishClock; // for checking elapsed time
	double joyElapsedTime; 	// time in seconds for master clock
	float joyMasterLoopTime = 0.150;	// Control loop time
	int i_thread = 0;	// test variable
	printf("hello!\n");
	joyStartClock = clock();
	while(true)
	{
		joyFinishClock = clock();
		joyElapsedTime = ((double)(joyFinishClock - joyStartClock)/CLOCKS_PER_SEC);
		if (joyElapsedTime >= joyMasterLoopTime) {
			// The usleep() command below screws this up
			if(joystickControl == true)
			{
				softStop();
				joystickControl = false;
				printf("lost connection\n");
			}
		}
		if (lineTracking)
		{
			lineTracking = lineTrack(lineTrackSpeed);
			printf("line tracking \n");
		}
		if(sideLineTracking)
		{
			sideLineTracking = sideLineTrack(sideLineTrackSpeed);
			printf("Side line tracking\n");
		}
		if(properAlignment != true)
		{
			properAlignment = adjustAlignment();
		}
		/*
		 * This is required to prevent thread from consuming 100% CPU
		 * so far, min. value seems to be 100,000 - below that and 
		 * CPU gobble occurs
		 */
		 usleep(100000); 
	}
}

int main(int argc, char **argv) {
	int parentfd; /* parent socket */
	int childfd; /* child socket */
	int portno; /* port to listen on */
	int clientlen; /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char buf[BUFSIZE]; /* message buffer */
	char input[BUFSIZE]; /*modifiable copy of buf*/
	char* param[4]; /*holds seperated values*/
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
	int command = 0; /*command type for agv*/
	int comaddr = 0; /*addr for command to go*/
	int comval = 0; /*value of command*/
	int comaux = 0; /* auxiliary parameter for joystick control */
	char reply[MSGSIZE]; /*send reply to command*/
	fd_set fds; /*not 100% what this line does, but is nec*/
	bool I_AM_PI; /* true if raspberry pi */
	signed int joyX, joyY, joyZ; /* Joystick X, Y, Z components */
	
	
	/* Determine if I am a Raspberry Pi*/
	#ifdef NOPI
		I_AM_PI = false;
	#endif

	/* 
	 * If this is a RPi, set up the gpio pins
	 * 
	 */
	#ifdef RPI
		printf("RPI setup found\n");
		wiringPiSetup();
		
		// Magnetic tape sensors
		pinMode(MAG_FL, INPUT);
		pinMode(MAG_FR, INPUT);
		pinMode(MAG_RL, INPUT);
		pinMode(MAG_RR, INPUT);
		pinMode(MAG_SLND, OUTPUT);
		pinMode(RELAY_ENABLE, OUTPUT);  // motor isolation relay
		
		softPwmCreate(DRIVE_FL, 0, 100);
		softPwmCreate(DRIVE_FR, 0, 100);
		softPwmCreate(DRIVE_RL, 0, 100);
		softPwmCreate(DRIVE_RR, 0, 100);
		piThreadCreate(myThread);
	#endif

	/* 
	 * check command line arguments 
	 */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}
	portno = atoi(argv[1]);

	/* 
	 * socket: create the parent socket 
	 */
	parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0) 
		error("ERROR opening socket");

	/*
	 * build the server's Internet address
	 */
	bzero((char *) &serveraddr, sizeof(serveraddr));

	/* this is an Internet address */
	serveraddr.sin_family = AF_INET;

	/* let the system figure out our IP address */
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* this is the port we will listen on */
	serveraddr.sin_port = htons((unsigned short)portno);

	/* 
	 * bind: associate the parent socket with a port 
	 */
	if (bind(parentfd, (struct sockaddr *) &serveraddr, 
		sizeof(serveraddr)) < 0) 
		error("ERROR on binding");

	/* setsockopt: Handy debugging trick that lets 
	 * us rerun the server immediately after we kill it; 
	 * otherwise we have to wait about 20 secs. 
	 * Eliminates "ERROR on binding: Address already in use" error.
	 * 
	 * dcohen:
	 * This problem has not been fully solved.  When server is
	 * shut down, still need to wait TIME_WAIT seconds before
	 * trying to reconnect, or same error will occur
	 */
	optval = 1;
	setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
			 (const void *)&optval , sizeof(int));

	/* 
	 * listen: make this socket ready to accept connection requests 
	 */
	if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
		error("ERROR on listen");

	/* 
	 * main loop: wait for a connection request, echo input line, 
	 * then close connection.
	 */
	clientlen = sizeof(clientaddr);

	while (1) {
		/* 
		 * accept: wait for a connection request 
		 */
		childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
		if (childfd < 0) 
			error("ERROR on accept");
    
		/* 
		 * gethostbyaddr: determine who sent the message
		 * 
		 * dcohen - I have disabled this currently as this breaks on 
		 * a remote PC - need to find out why (see commented out below)
		 */
		 
		 /*
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
				sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server established connection with %s (%s)\n", hostp->h_name, hostaddrp);
		*/ 
    
		/* 
		 * read: read input string from the client
		 * 
		 * NOTE:
		 * Need to fix to allow arbitrary number of arguments
		 * 
		 */
		bzero(buf, BUFSIZE);
		bzero(input, BUFSIZE);
	
		n = read(childfd, buf, BUFSIZE);
		if (n < 0) 
			error("ERROR reading from socket");
		printf("server received %d bytes: %s", n, buf);
		//for the future, strcmp(string, string) is useful for comparison
		//between two strings. will return 0 if equal.
		//while strtoken(char *str, const char *delim) is probably the
		//most useful for parsing a string
		param[0] = strtok(buf, " ");
		param[1] = strtok(NULL, " ");
		param[2] = strtok(NULL," ");
		param[3] = strtok(NULL, "\0");


		// The command is always the first parameter
		command = atoi(param[0]);

		/*
		 * This code is here to insure that we react properly to the
		 * number of bytes sent. If a variable assignment is made
		 * using a nul pointer, the program will segfault
		 */
		 // do we have a 2nd parameter (address)
		if (param[1] != NULL)	{	
			comaddr = atoi(param[1]);
		}
		// do we have a 3rd parameter (value)
		if (param[2] != NULL)	{
			comval = atoi(param[2]);
		}
		// do we have a 4th parameter (aux value)
		if (param[3] != NULL)
			comaux = atoi(param[3]);
		
		// Branch according to command number
		switch (command) {
		case 0: /* RPi pin test - */
			bitWrite(16, 0);
			bitWrite(16, 0);
			printf("Emergency Stop!\n");
			return(0);
			break;
		case 1: /* version */
			strcpy(reply, "Flexicart S/N: 00 \nVersion 20141102\n");
			break;
		case 2: /* Digital read */
			sprintf(reply, "%d", bitRead(comaddr));
			break;
		case 3: /* Digital write */
			/* Only a 0 or 1 allowed */
			if ((comval != 0) && (comval != 1))	{
				perror("0 or 1 only");
				strcpy(reply, "false\n");
				break;
			}
			bitWrite(comaddr, comval);
			strcpy(reply, "true\n");
			break;
		case 4:
			printf("AnalogRead not supported\n");
			strcpy(reply, "false\n");
			break;
		case 5:
			printf("AnalogWrite not supported\n");
			strcpy(reply, "false\n");
			break;
		case 6: /* PWM write */
			if(comaddr == 10 || comaddr == 11 || comaddr == 26 || comaddr == 15)
			{
				lineTracking = false;
				softStop();
			}
			if(comval >= -100 && comval <= 100)
			{
				sprintf(reply, "%d\n", PWMWrite(comaddr, comval));
			}
			else
			{
				strcpy(reply, "invalid value.\n");
			}
			break;
		case 7: /* ramp motor speed */
			/* parameters : motor, end_speed, accel
			 * 0 - front left 
			 * 1 - front right
			 * 2 - rear left
			 * 3 - rear right
			 */
			 if (comaddr < 0 || comaddr > 3) // motor number
			 {
				 printf("Valid values 0 to 3.\n");
				 strcpy(reply, "false\n");
				 break;
			 }
			 else if(comval < -100 || comval > 100) // motor speed
			 {
				 printf("Valid values -100 to 100.\n");
				 strcpy(reply, "false\n");
				 break;
			 }
			
			 /* start motor ramp 
			  * 
			  * I wanted to pass accel as a parameter, but we need to
			  * fix routine to accept more arguments
			  * Will hardcode accel for now
			  */
			 motorRamp(comaddr, comval, 2);
			
			break;

         case 8:	// enable/disable line tracking
			if (comaddr == 1) {
				lineTracking = true;
				lineTrackSpeed = comval;
				strcpy(reply, "true\n");
			}
			else
			{
				lineTracking = false;
				firstTimeTracking = true;
				lineTrackSpeed = 0;
				// set motors to zero speed
				softStop();
				strcpy(reply, "false\n");
			}
			break;
			
		case 9:	// joystick enable/disable
			if (comaddr == 1)
			{
				printf("Joystick control enabled\n");
				joystickControl = true;
			}
			else if (comaddr == 0)
			{
				printf("Joystick control disabled\n");
				softStop();
				joystickControl = false;
			}
			strcpy(reply, "true\n");			
			break;
		case 10: //tank drive
			if(comaddr == 0)
			{
				PWMWrite(DRIVE_FR, comval);
				PWMWrite(DRIVE_FL, comval);
				PWMWrite(DRIVE_RR, comval);
				PWMWrite(DRIVE_RL, comval);
				if(comval == 0)
				{
					PWMWrite(DRIVE_FR, 0);
					PWMWrite(DRIVE_FL, 0);
					PWMWrite(DRIVE_RR, 0);
					PWMWrite(DRIVE_RL, 0);
				}
			}
			else if(comaddr == 90)
			{
				PWMWrite(DRIVE_FR, comval);
				PWMWrite(DRIVE_FL, -comval);
				PWMWrite(DRIVE_RR, -comval);
				PWMWrite(DRIVE_RL, comval);
			}
			else if(comaddr == 180)
			{
				PWMWrite(DRIVE_FR, -comval);
				PWMWrite(DRIVE_FL, -comval);
				PWMWrite(DRIVE_RR, -comval);
				PWMWrite(DRIVE_RL, -comval);
			}
			else if(comaddr == 270)
			{
				PWMWrite(DRIVE_FR, -comval);
				PWMWrite(DRIVE_FL, comval);
				PWMWrite(DRIVE_RR, comval);
				PWMWrite(DRIVE_RL, -comval);
			}
			strcpy(reply, "true\n");
			break;	
		case 11: //RFID Control!
			if(comaddr == 0)
			{
				RFIDTracking = false;
				properAlignment = checkAlignment();
			}
			else
			{
				RFIDTracking = true;
				properAlignment = false;
			}
			
			break;
		case 12: //adjust alignment
			if(comaddr == 1)
			{
				properAlignment = false;
				strcpy(reply, "true\n");
			}
			else
			{
				properAlignment = true;
				softStop();
				strcpy(reply, "false\n");
			}
			break;
		case 13: //turns
			PWMWrite(DRIVE_FR, -comaddr*comval);
			PWMWrite(DRIVE_FL, comaddr*comval);
			PWMWrite(DRIVE_RR, -comaddr*comval);
			PWMWrite(DRIVE_RL, comaddr*comval);
			break;
		case 14:	//side line track
			if (comaddr == 1) {
				sideLineTracking = true;
				sideLineTrackSpeed = comval;
				strcpy(reply, "true\n");
			}
			else
			{
				sideLineTracking = false;
				firstTimeTracking = true;
				sideLineTrackSpeed = 0;
				// set motors to zero speed
				softStop();
				strcpy(reply, "false\n");
			}
			break;
		case 15:	// joystick data
			//printf("Joystick x: %d\n",comaddr);
			//printf("Joystick y: %d\n",comval);
			//printf("Joystick z: %d\n",comaux);
			joyX = comaddr;
			joyY = comval;
			joyZ = comaux;
			PWMWrite(DRIVE_FR, joyY - joyX - joyZ);
			PWMWrite(DRIVE_FL, joyY + joyZ + joyX);
			PWMWrite(DRIVE_RR, joyY - joyZ + joyX);
			PWMWrite(DRIVE_RL, joyY + joyZ - joyX);
			joyStartClock = clock();
			strcpy(reply, "true\n");			
			break;
		case 99:	// quit
			agvShutdown();
			return(0);
			break;
		default:
			printf("Input in int-int-int format - wtf is wrong with you?\n");
		}

		/* 
		 * write: reply the message to the client 
		 */
		n = write(childfd, reply, strlen(reply));
		if (n < 0) 
			error("ERROR writing to socket");

		close(childfd);
	}
	agvShutdown();
	return(0);
}