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

#include "robotMap.h"
#include "140008lib.h"
#include "joystick.h"

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
	perror(msg);
	exit(0);
}

int main(int argc, char **argv) {
	int sockfd, portno, n;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	char buf[BUFSIZE];
	char message[BUFSIZE];
	char* param[3];
	int cont = 0;
	
	// All below is joystick stuff
	bool joystickControl = false;
	int command; /*command type for agv*/
	int comaddr; /*addr for command to go*/
	int comval; /*value of command*/
	bool fire_pressed = false;
	// true only if an event we are using is activated
	bool relevant = true;
	int joystick_x_axis;
	int joystick_y_axis;
	int joystick_z_axis;
	int fd, rc;
	int done = 0;

	struct js_event jse;

	/*
	 * Right now client will exit if joystick not plugged in. Need 
	 * to fix that later
	 *  
	 */
	/*fd = open_joystick(0);
	if (fd < 0) {
		printf("open failed.\n");
		exit(1);
	}*/

	while(cont == 0)
	{
		// Why are we doing argument checking and 
		// socket stuff inside a while loop?

		/* check command line arguments */
		if (argc != 3) {
			fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
			exit(0);
		}
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
	
		/* connect: create a connection with the server */
		if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0) 
			error("ERROR connecting");
	
		/* get message line from the user */
		printf("Please enter msg: ");
		bzero(buf, BUFSIZE);
		fgets(buf, BUFSIZE, stdin);
		/* 
		 * Copy the buffer to message array which will be evaluated
		 * to determine if joystick control is being done
		 * message will contain whatever we sent as the message
		 */
		strcpy(message, buf);

		 /* Are we implementing joystick control */
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
		
		if (command == 9) {
			if (comaddr == 1) {
				joystickControl = true;
				printf("Setting joystick control\n");
			}
			else {
				joystickControl = false;
				printf("Clearing joystick control\n");
			}
		}
	
	/*	rc = read_joystick_event(&jse);
		usleep(1000);
		if (rc == 1) {
			if (jse.type == 2 && jse.number == 0) {
				joystick_x_axis = jse.value;
				//relevant = true;
			}
			else if (jse.type == 2 && jse.number == 1) {
				joystick_y_axis = jse.value;
				//relevant = true;
			}
			else if (jse.type == 2 && jse.number == 3) {
				joystick_z_axis = jse.value;
				//relevant = true;
			}
			else if (jse.value == 1 && jse.type == 1 && jse.number == 0) {
				fire_pressed = true;
				//relevant = true;
			}
			else if (jse.value == 0 && jse.type == 1 && jse.number == 0) {
				fire_pressed = false;
				//relevant = true;
			}
			else
				relevant = false;
				
			if (true)
				printf("X: %8hd, Y: %8hd, Z: %8hd, Fire: %d\n",
					joystick_x_axis, joystick_y_axis, joystick_z_axis, fire_pressed);
		}
		
	*/	/*
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
