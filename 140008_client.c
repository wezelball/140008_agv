/*
 * 140008_client.c - A simple TCP client
 * usage: 140008_client <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

#include "robotMap.h"
#include "140008lib.h"

/* for joystick */
#include "joystick.h"
#include <pthread.h>	/* POSIX Threads */

#define BUFSIZE 1024

/* prototype for thread routine */
void joystick_update ( void *ptr );
//void heartbeatCheck (void *ptr );
void INThandler(int);

/*
 * struct to hold data to be passed to a thread
 * this shows how multiple data items can be passed to a thread
 * (we are not making use of this yet
 */
typedef struct struct_thdata
{
    int thread_no;
    char message[100];
} thdata;


/* Global variables for thread to access */

// Initialized by the joystick thread
signed int joystick_x, joystick_y, joystick_z;
// Threads kick my ass on this one
signed int joy_x, joy_y, joy_z;
bool fire_pressed = false;
/* relevant is true if we activate a joystick function
 * that is bring used in the application
 */
bool relevant = true;
struct js_event jse;
int rc;
bool joystickControl = false; // we are using joystick to steer the AGV

/* The network socket is now global so the signal handler routine
 * has access to it
 */
int sockfd;

/*
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(0);
}

int main(int argc, char **argv) {
	//int sockfd;
	int portno, n;
	struct sockaddr_in serveraddr;
  //const struct sockaddr serveraddr;
	struct hostent *server;
	char *hostname;
	char buf[BUFSIZE];
	char message[BUFSIZE];
	char* param[3];
	int cont = 0;

	/* All below is joystick stuff */
	bool joystickControl = false;
	int command; /*command type for agv*/
	int comaddr; /*addr for command to go*/
	int comval; /*value of command*/

	/* True only if an event we are using is activated */
	bool relevant = true;

	int fd, rc;
	int done = 0;

	/* Data types for threads */
	pthread_t joyThread;
	thdata joyData; // this is a structure we can pass to the thread
	//pthread_t heartbeat_thread;
	//thdata heartData;

	/* This is trapping the Ctrl-C signal for safe shutdown */
	signal(SIGINT, INThandler);

	/* Create the joystick polling thread */
	pthread_create(&joyThread, NULL, (void *) &joystick_update, (void *) &joyData);
	/* Create the client heartbeat thread */
	//pthread_create(&heartbeat_thread, NULL, (void *) &heartbeatCheck, (void *) &heartData);

	// FIXME If no joystick, don't just die
	fd = open_joystick(0);
	if (fd < 0) {
		printf("open failed.\n");
		//exit(1);
	}

/* check command line arguments */
if (argc != 3) {
  fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
  exit(0);
}

	while(cont == 0)
	{
		// Why are we doing argument checking and
		// socket stuff inside a while loop?
		// this may be causing the excessivs file open
		// errors we are having

		/* FIXME - THIS SECTION SHOULD BE OUTSIDE OF while() LOOP */
		/* ****************************************************** */

		hostname = argv[1];
		portno = atoi(argv[2]);

		/* socket: create the socket */
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		error("ERROR opening socket");
		
		/* gethostbyname: get the server's DNS entry */
		server = gethostbyname(hostname);
		if (server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		exit(0);
		}
		
		/* build the server's Internet address */
		bzero((char *) &serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		bcopy((char *)server->h_addr,
		(char *)&serveraddr.sin_addr.s_addr, server->h_length);
		serveraddr.sin_port = htons(portno);


		/* ****************************************************** */
		/* FIXME - THIS SECTION SHOULD BE OUTSIDE OF while() LOOP */
    //I think I was just able to pull it out of loop
    //except for this part
                /*Connect: create a connection with the server*/
                if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
                {
                        error("ERROR connecting");
                }
    
    
		bzero(buf, BUFSIZE);

		if (!joystickControl) {
			/* get message line from the user */
			printf("Please enter msg: ");
			fgets(buf, BUFSIZE, stdin);
		}
		else {
			usleep(125000);
			if (relevant) {
				joy_x = scale_joystick(joystick_x);
				joy_y = -scale_joystick(joystick_y);
				joy_z = scale_joystick(joystick_z);
				sprintf(buf, "15 %d %d %d\n", joy_x, joy_y, joy_z);
			}
		}
		/*
		 * Copy the buffer to message array which will be evaluated
		 * to determine if joystick control is being done
		 * message will contain whatever we sent as the message
		 * (i dont think we need this anymore)
		 */
		strcpy(message, buf);

		param[0] = strtok(message, " ");
		param[1] = strtok(NULL, " ");
		param[2] = strtok(NULL, "\0");
		command = atoi(param[0]); // we always have this

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

		/* Are we under joystick control */
		if (fire_pressed == true || command == 9) {
			joystickControl = true;
			printf("Fire pressed: %d\n", fire_pressed);
			printf("Command: %d\n", command);
		}
		else
		{
			if(joystickControl == true)
			{
				sprintf(buf, "9 0\n");
				printf("turning wheels off\n");
			}
			joystickControl = false;
		}

		/*
		 *Check to see if we're killing the client
		 */
		if(strcmp(buf, "99\n") == 0)
		{
			cont = 1;
      }

		/* send the message line to the server */
		n = write(sockfd, buf, strlen(buf));
		if (n < 0)
			error("ERROR writing to socket");

		/* print the server's reply */
		bzero(buf, BUFSIZE);
		n = read(sockfd, buf, BUFSIZE);
		if(buf == NULL)
		{
			buf == "\n";
		}
		if (n < 0)
			error("ERROR reading from socket");
		printf("Echo from server: %s\n", buf);
	}
	close(sockfd);
    return 0;
}

/*
 * Joystick update function
 * it accepts a void pointer
 */
void joystick_update ( void *ptr )
{
    thdata *jdata;
    jdata = (thdata *) ptr;  /* type cast to a pointer to thdata */


	while(true)
	{
		rc = read_joystick_event(&jse);
		usleep(1000);
		if (rc == 1) {
			//printf("Event: time %8u, value %8hd, type: %3u, axis/button: %u\n",
				//jse.time, jse.value, jse.type, jse.number);
			if (jse.type == 2 && jse.number == 0) {
				joystick_x = jse.value;
				relevant = true;
				//printf("X axis\n");
			}
			else if (jse.type == 2 && jse.number == 1) {
				joystick_y = jse.value;
				relevant = true;
				//printf("Y axis\n");
			}
			else if (jse.type == 2 && jse.number == 3) {
				joystick_z = jse.value;
				relevant = true;
				//printf("Z axis\n");
			}
			else if (jse.value == 1 && jse.type == 1 && jse.number == 0) {
				fire_pressed = true;
				relevant = true;
				//printf("Fire button\n");
			}
			else if (jse.value == 0 && jse.type == 1 && jse.number == 0) {
				fire_pressed = false;
				relevant = true;
			}
			else
				relevant = false;
		}
	}
}

/*
void heartbeatCheck (void *ptr) {
	thdata *hdata;
    hdata = (thdata *) ptr;  // type cast to a pointer to thdata
    while(true)
    {
		 usleep(500000);
	 }
}
*/

/* This function handles Ctrl-C trapping on the client side
 * Sends a "99" command to terminate the server properly, and
 * then shuts itself down
 */
void INThandler(int sig)
{
	char buf[BUFSIZE];
	int n;

	sprintf(buf, "99\n");
	n = write(sockfd, buf, strlen(buf));
	printf("client emergency stopped\n");
	exit(0);
}
