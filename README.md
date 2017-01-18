# TCP_UDP_ECHO_ClientServer


tar file contains 4 files
a)server.cpp
b)Makefile
c)Readme.txt
d)Answers to Section-5

Execution Steps :
Step 1. Extract the tar file to a folder in system.
Step 2. Open a Terminal anf change the directory path to the folder containing the source.
Step 3. use "make" command to build the code .
Step 4. run the echo server using "./server -m t -p 51000" to run the server in TCP mode on port number 51000
        run the echo server using "./server -m u -p 51000" to run the server in UDP mode on port number 51000

Step 5. run the EchoClient as in the Project requirement spec document.


Features of the Server :

1)server can handle multiple TCP connections i used multithreading to implement the feature.
2)server can handle multiple UDP connections i used multithreading to implement the feature.
3)use of a class called connection statistics to store the information relavent to the statistics.
4)Avoiding unnecessary execution of UDP listener in the absense of any messages.
5)Use seperate thread to process completed UDP statistics and display to the user.
