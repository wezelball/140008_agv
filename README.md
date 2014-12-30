140008_agv
==========

Test code for RandD 140008 AGV
Created 9/26/2014

We have set up a simple TCP client/server model with the ability to discriminate text.

Next step is to code a SWITCH statement for conditional branching.

11/2/14
Today is the birth of the AGV, as it now has moving parts!

All drives (except rear left) are working, sensors are working. I'll have
to redesign board as I should have run PWM outputs straight from GPIO,
and not thru interface chips.  I also need to free up the serial port 
and run directly past interface chips.  That will be used for RFID.

Need to figure out how to get power to RPi from robot. I see a 5V power 
converter in the near future.

Need to create a wireless access point for the cart and attached
devices.

11/4/14
Fixed left drive which was a electrical issue.  Motors run forward when
given a positive speed.

Got rid of some compiler warnings by adding proper includes.

Fixed threads by moving back into main.

Started code on speed ramp generation.

12/12/14
Password for remote access is 140008

12/13/14
Almost to the point of line tracking - fixed a lot of stuff in the code

12/27/14
Have line following code up to 50 in proper orientation, and 30% sideways orientation, safety features for disconnect and emergency shutdown, dead man switch, joystick control, and gyroscope and accelerometer feedback.

12/30/14
Accelerometer code proceeding well.  Now need to address the interface PCB design upgrade - there are several issues to address to improve the board:
1. PWM signals need to come straight from the Pi, and not go through the buffer chips. PWM and other "raw" outputs/inputs should be a special class of I/O. Even through PWM's from Pi may be deprecated, need to maintain backwards compatibility.
2. The pins on the SPI and and I2C headers are too tall - I need to specify shorter ones.
3. All I/O and power connections to the board need to be changed to plug-in style connectors (i.e. phoenix), to simplify board replacement.
4. There are too few inputs, and too many outputs. Currently 9 inputs, 12 outputs, can probably reverse that.
5. Buffer chips should be socketed!
6. Need to case the PCB with an enclosure.
