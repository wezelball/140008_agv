/*
 * Note if we want serial port for RFID, will need to reserve
 * GPIO14 and GPIO15
 * 
 */

//Drive motors constant- soft pwms, get plugged into Oxx
#define DRIVE_FL 11 //Drive front left motor, connect to O8, gpio 7, J8 pin 26
#define DRIVE_FR 10 //Drive front right motor, connect to O7, gpio 8, J8 pin 24
#define DRIVE_RL 26 //Drive rear left motor, connect to O9, gpio 12, J8 pin 32
#define DRIVE_RR 15 //Drive rear right motor, connect to O1, gpio 14, J8 pin 8

#define DRIVE_FL_OFFSET 0 //Drive front left motor offset
#define DRIVE_FR_OFFSET 0 //Drive front right motor offset
#define DRIVE_RL_OFFSET 20 //Drive rear left motor offset
#define DRIVE_RR_OFFSET 0 //Drive rear right motor offset

// oops, i used this for safety relay - dlc
// #define CNVYR_DR 16 //Converyor belt motor, connect to O2, gpio 15, J8 pin 10

#define MAG_FR 7 	//Magnetic sensor front right, connect to I3, gpio 4, J8 pin 7
#define MAG_RL 21 	//Magnetic sensor rear left, connect to I1, gpio 5, J8 pin 29
#define MAG_FL 22 	//Magnetic sensor front left, connect to I2, gpio 6, J8 pin 31
#define MAG_RR 23 	//Magnetic sensor rear right, connect to I7, gpio 13, J8 pin 33
#define MAG_SL 24 	//Magnetic sensor side left, connect to I8, gpio 19, J8 pin 35
#define MAG_SR 3	//Magnetic sensor side right, connect to I6, gpio 22, J8 pin 15
 
#define BATT_CURR 999 //Reads battery current, connect to ain, PORTNO UNKNOWN ATM
#define BATT_VOLT 999 //Reads battery current, connect to ain, PORTNO UNKNOWN ATM

#define MAG_SLND 27 //Electromagnetic solenoid, connects to O10 gpio 16, J8 pin 36

#define RELAY_ENABLE 16	//Safety relay 1, connect to O2, gpio 15, J8 pin 10


