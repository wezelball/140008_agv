PILIBS = -L/usr/local/src/wiringPi/wiringPi -lwiringPi
PIINCLUDES = -I/usr/local/src/wiringPi/wiringPi
JOYLIBS = -lpthread
LIBS = -lm -lpthread

pi: 140008_client 140008_server_pi

x86: 140008_client 140008_server_x86

140008_client: 140008_client.o joystick
	gcc -g 140008_client.c joystick.c -o 140008_client $(JOYLIBS)

140008_server_pi: 140008_obj_pi procedures_pi gyroprocedures_pi
	gcc -DRPI -g 140008_server.o procedures.o gyroprocedures.o -o 140008_server $(LIBS) $(PILIBS) $(PIINCLUDES)
	
140008_server_x86: 140008_obj_x86 procedures.o gyroprocedures.o
	gcc -DNOPI -g 140008_server.o procedures.o gyroprocedures.o -o 140008_server $(LIBS)

140008_obj_pi: 140008_server.c
	gcc -g -DRPI -c 140008_server.c $(PILIBS) $(PIINCLUDES)

140008_obj_x86: 140008_server.c
	gcc -g -DNOPI -c 140008_server.c
	
procedures_pi: procedures.o
	gcc -DRPI -g -c procedures.c

gyroprocedures_pi: gyroprocedures.o
	gcc -DRPI -g -c gyroprocedures.c

joystick: joystick.o
	gcc -g -c joystick.c
clean: 
	rm -rf *.o 140008_client 140008_server DEADJOE *~
