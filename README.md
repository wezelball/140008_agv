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

Fixed threads by movibng back into main

Started code on speed ramp generation.

12/12/14
Password for remote access is 140008
