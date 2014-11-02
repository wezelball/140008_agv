/*
 * Note if we want serial port for RFID, will need to reserve
 * GPIO14 and GPIO15
 * 
 */

//Drive motors constant- soft pwms, get plugged into Oxx
#define D_FR_LT 11 //Drive front left motor, connect to O8, gpio 7, J8 pin 26
#define D_FR_RT 10 //Drive front right motor, connect to O7, gpio 8, J8 pin 24
#define D_RR_LT 26 //Drive rear left motor, connect to O9, gpio 12, J8 pin 32
#define D_RR_RT 15 //Drive rear right motor, connect to O1, gpio 14, J8 pin 8

// oops, i used this for safety relay - dlc
// #define CNVYR_DR 16 //Converyor belt motor, connect to O2, gpio 15, J8 pin 10

#define MAG_SR_FT 7 //Magnetic sensor front, connect to I3, gpio 4, J8 pin 7
#define MAG_SR_RR 21 //Magnetic sensor rear, connect to I1, gpio 5, J8 pin 29
#define MAG_SR_LT 22 //Magnetic left, connect to I2, gpio 6, J8 pin 31
#define MAG_SR_RT 23 //magnetic right, connect to I7, gpio 13, J8 pin 33

#define BATT_CURR 999 //Reads battery current, connect to ain, PORTNO UNKNOWN ATM
#define BATT_VOLT 999 //Reads battery current, connect to ain, PORTNO UNKNOWN ATM

#define MAG_SLND 27 //Electromagnetic solenoid, connects to O10 gpio 16, J8 pin 36

#define RELAY_ENABLE 16	//Safety relay 1, connect to O2, gpio 15, J8 pin 10

