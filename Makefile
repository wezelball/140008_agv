PILIBS = -L/usr/local/src/wiringPi/wiringPi -lwiringPi -lpthread
PIINCLUDES = -I/usr/local/src/wiringPi/wiringPi

pi: 140008_client 140008_server_pi

x86: 140008_client 140008_server_x86

140008_client: 140008_client.o
	gcc -g 140008_client.c -o 140008_client

140008_server_pi: 140008_obj_pi controlLoop.o procedures_pi
	gcc -DRPI -g 140008_server.o controlLoop.o procedures.o -o 140008_server $(PILIBS) $(PIINCLUDES)

140008_obj_pi: 140008_server.c
	gcc -DRPI -c 140008_server.c $(PILIBS) $(PIINCLUDES)
	
controlLoop.o: controlLoop.c
	gcc -g -c controlLoop.c

procedures_pi: procedures.o
	gcc -DRPI -g -c procedures.c

140008_server_x86: 140008_server.o controlLoop.o procedures.o
	gcc -DNOPI -g 140008_server.o controlLoop.o procedures.o -o 140008_server
	
clean: 
	rm -rf *.o 140008_client 140008_server DEADJOE *~
