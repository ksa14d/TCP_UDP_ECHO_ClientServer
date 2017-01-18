CC = g++
CFLAGS= -O0 
LIBS = -lpthread\

all:  server
clean:
	rm -f *.o
	rm -f server 



server.o: server.cpp 
	  $(CC) $(CFLAGS) -g -c server.cpp 

server:  server.o 
	  $(CC) $(CFLAGS) -g -o server server.o ${LIBS}


