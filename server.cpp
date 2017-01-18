#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <math.h>
#include <getopt.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#define MAXBUFLEN 512
using namespace std;



bool IsTcp = false;

int SERVERPORT = 51000;
char SERVER_PROTOCOL;
struct option long_options[] =
{
	{ "server_protocol", required_argument, NULL, 'm' },
	{ "serverport", required_argument, NULL, 'p' },
};

fd_set master;
fd_set read_fds;
int listener;
// highest file descriptor seen so far
int highestsocket;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;


class ConnStats
{
public:
	int Sock;
	double prev_time_in_mill;
	double average;
	double MinInterval;
	double MaxInterval;
	double TotalBytesRecvd;
	double TotalBytesSent;
	double TotalMessages;
	double TotalRunTime;
	double StartTime;
	double EndTime;
	char	Buf[MAXBUFLEN + 1];
	bool IsComplete;
	int k;

	ConnStats()
	{
		prev_time_in_mill = 0;
		average = 0;
		MinInterval = 100000;
		MaxInterval = 0;
		TotalBytesRecvd = 0;
		TotalBytesSent = 0;
		TotalMessages = 0;
		TotalRunTime = 0;
		StartTime = 0;
		EndTime = 0;
		k = 0;
		IsComplete = false;
		bzero(Buf, MAXBUFLEN);
	}
	ConnStats(int SockID)
	{
		prev_time_in_mill = 0;
		average = 0;
		MinInterval = 100000;
		MaxInterval = 0;
		TotalBytesRecvd = 0;
		TotalBytesSent = 0;
		TotalMessages = 0;
		TotalRunTime = 0;
		StartTime = 0;
		EndTime = 0;
		k = 0;
		IsComplete = false;
		Sock = SockID;
		bzero(Buf, MAXBUFLEN);
	}


};

void *thread_new_TCPconnection(void *arg)
{
	ConnStats *Stats;

	Stats = (ConnStats *)arg;
	double Curr_time_in_mill = 0;
	bzero(Stats->Buf, MAXBUFLEN);
	int n;

	struct timeval now;
	while ((n = read(Stats->Sock, Stats->Buf, MAXBUFLEN)) > 0)
	{

		gettimeofday(&now, NULL);
		Curr_time_in_mill = (now.tv_sec) * 1000 + (now.tv_usec) / 1000;

		if (n == 3 && 0 == strcmp("end", Stats->Buf))
		{
			Stats->EndTime = Curr_time_in_mill;
			Stats->TotalRunTime = Stats->EndTime - Stats->StartTime;
			Stats->IsComplete = true;
			break;
		}
		else
		{
			Stats->TotalMessages++;
			Stats->TotalBytesRecvd += n;
			if ((n = write(Stats->Sock, Stats->Buf, n)) <= 0)
			{
				cout << "Error writing to socket" << endl;
			}
			else
			{
				Stats->TotalBytesSent += n;
				bzero(Stats->Buf, MAXBUFLEN);
				if (Stats->prev_time_in_mill != 0 && Stats->k <= 1000)
				{
					double interval = Curr_time_in_mill - Stats->prev_time_in_mill;
					if (interval < Stats->MinInterval)Stats->MinInterval = interval;
					if (interval > Stats->MaxInterval)Stats->MaxInterval = interval;
					double sum = (Stats->average * Stats->k + interval);
					Stats->k++;
					Stats->average = sum / Stats->k;
				}
				else
				{
					Stats->StartTime = Curr_time_in_mill;
				}
				Stats->prev_time_in_mill = Curr_time_in_mill;
			}
		}
	}
	if (Stats->IsComplete)
	{
		/********* BEGIN COMER CODE **********/
		cout << "closing the slave socket" << endl;
		/********* END COMER CODE **********/
		cout << "Messages Received : " << Stats->TotalMessages << endl;
		cout << "Bytes received : " << Stats->TotalBytesRecvd << endl;
		cout << "Run time : " << Stats->TotalRunTime << endl;
		cout << "Bytes sent : " << Stats->TotalBytesSent << endl;
		cout << "Min Lamda : " << Stats->MinInterval << endl;
		cout << "Max Lamda : " << Stats->MaxInterval << endl;
		cout << "Lamda : " << Stats->average << endl;
		close(Stats->Sock);
		FD_CLR(Stats->Sock, &master);
	}
}

vector<ConnStats> CompletedUDPTransactions;


void *thread_process_completedstats(void *arg)
{
	vector<ConnStats> statsToDisplay;
	while (1)
	{
		pthread_mutex_lock(&mtx);	    //critical section
		for (int i = 0; i < CompletedUDPTransactions.size(); i++)
		{
			statsToDisplay.push_back(CompletedUDPTransactions[i]);
		}
		CompletedUDPTransactions.clear();       //crritical section
		pthread_mutex_unlock(&mtx);

		vector<ConnStats>::iterator it = statsToDisplay.begin();
		while (it != statsToDisplay.end())
		{
			cout << "---Statistic Results-----" << endl;
			cout << "Messages Received : " << it->TotalMessages << endl;
			cout << "Bytes received : " << it->TotalBytesRecvd << endl;
			cout << "Run time : " << it->TotalRunTime << endl;
			cout << "Bytes sent : " << it->TotalBytesSent << endl;
			cout << "Min Lamda : " << it->MinInterval << endl;
			cout << "Max Lamda : " << it->MaxInterval << endl;
			cout << "Lamda : " << it->average << endl;
			it = statsToDisplay.erase(it);
		}

	}
}

void server_run(void)
{
	struct timeval timeout;
	struct sockaddr_in remoteaddr;
	socklen_t addrlen;
	double Curr_time_in_mill = 0;
	int n;
	bool IsComplete = false;
	struct timeval now;
	while (1)
	{
		FD_SET(listener, &read_fds);
		read_fds = master;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		/********* BEGIN COMER CODE **********/
		if (select(highestsocket + 1, &read_fds, NULL, NULL, &timeout) == -1)
		{
			if (errno == EINTR)
			{
				cout << "got the EINTR error in select" << endl;
			}
			else
			{
				cout << "select problem, server got errno " << errno << endl;
				printf("Select problem .. exiting server");
				fflush(stdout);
				exit(1);
			}
		}

		if (FD_ISSET(listener, &read_fds) && IsTcp == true)
		{

			int *ssock = new int;		// TCP slave socket	
			cout << "created a slave socket" << endl;
			addrlen = sizeof(remoteaddr);
			*ssock = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
			/********* END COMER CODE **********/
			if (*ssock < 0)
				perror("accept failed ");
			FD_SET(*ssock, &master); // add to master set
			if (*ssock > highestsocket)
			{
				highestsocket = *ssock;
			}
			pthread_t tid;
			ConnStats *Stats = new ConnStats(*ssock);
			int res = pthread_create(&tid, NULL, thread_new_TCPconnection, Stats);
			if (res == 0)
				cout << "successfully created thread!" << endl;
			else
				cout << "Processing the Connection !" << endl;

		}
		/********* BEGIN COMER CODE **********/
		if (FD_ISSET(listener, &read_fds) && IsTcp == false)
		{
			/********* END COMER CODE **********/
			cout << "created a udp listener" << endl;
			addrlen = sizeof(remoteaddr);
			map<int, ConnStats*> PortStatisticMap;
			ConnStats *Stats;
			char buf[MAXBUFLEN + 1];
			bzero(buf, MAXBUFLEN);

			while (1)  // map contains some port do these things
			{
				if ((n = recvfrom(listener, buf, sizeof(buf), 0, (struct sockaddr *)&remoteaddr, &addrlen)) < 0)
					perror("Error receiving data");
				else
				{
					gettimeofday(&now, NULL);
					Curr_time_in_mill = (now.tv_sec) * 1000 + (now.tv_usec) / 1000;
					if (PortStatisticMap.find(remoteaddr.sin_port) == PortStatisticMap.end())
					{
						PortStatisticMap[remoteaddr.sin_port] = new ConnStats();
					}
					Stats = PortStatisticMap[remoteaddr.sin_port];
				}
				if (n == 3 && 0 == strcmp("end", buf))
				{
					Stats->IsComplete = true;
					Stats->EndTime = Curr_time_in_mill;
					Stats->TotalRunTime = Stats->EndTime - Stats->StartTime;
					pthread_mutex_lock(&mtx);
					CompletedUDPTransactions.push_back(*Stats);
					pthread_mutex_unlock(&mtx);
					PortStatisticMap.erase(remoteaddr.sin_port); //remove stats from map here  
					if (PortStatisticMap.size() == 0)break;
				}
				else
				{
					Stats->TotalMessages++;
					Stats->TotalBytesRecvd += n;
					if ((n = sendto(listener, buf, n, 0, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr))) < 0)
						perror("Error sending data");
					else
					{
						Stats->TotalBytesSent += n;
						bzero(buf, MAXBUFLEN);
						if (Stats->prev_time_in_mill != 0 && Stats->k <= 1000)
						{
							double interval = Curr_time_in_mill - Stats->prev_time_in_mill;
							if (interval < Stats->MinInterval)Stats->MinInterval = interval;
							if (interval > Stats->MaxInterval)Stats->MaxInterval = interval;
							double sum = (Stats->average * Stats->k + interval);                     //can store previous sum in stats to use it in next
							Stats->k++;
							Stats->average = sum / Stats->k;
						}
						else
						{
							Stats->StartTime = Curr_time_in_mill;
						}
						Stats->prev_time_in_mill = Curr_time_in_mill;
					}
				}
			}
		}
	}
}


void server_init(void)
{


	int type = -1;
	/********* BEGIN COMER CODE **********/
	if (SERVER_PROTOCOL == 't')
	{
		/********* END COMER CODE ************/
		IsTcp = true;
		type = SOCK_STREAM;
	}
	else
	{
		IsTcp = false;
		type = SOCK_DGRAM;
		pthread_t tid;
		int res = pthread_create(&tid, NULL, thread_process_completedstats, NULL);
		if (res == 0)
			cout << "UDP Displayer thread running!" << endl;
		else
			cout << "UDP Displayer thread failed !" << endl;
	}
	struct sockaddr_in myaddr;       // my address
	int yes = 1;                       // for setsockopt() SO_REUSEADDR, below

	int i, j;



	FD_ZERO(&master);                // clear the master and temp sets
	FD_ZERO(&read_fds);
	// get the listener

	if ((listener = socket(PF_INET, type, 0)) == -1)
	{
		printf("cannot create a socket");
		fflush(stdout);
		exit(1);
	}

	// lose the pesky "address already in use" error message
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		printf("setsockopt");
		fflush(stdout);
		exit(1);
	}

	// bind to the port
	bzero((char *)&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(SERVERPORT);
	memset(&(myaddr.sin_zero), '\0', 8);
	if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1)
	{
		printf("could not bind to MYPORT");
		fflush(stdout);
		exit(1);
	}

	// listen to and Q upto 40 in the wait list
	if (IsTcp)
	{
		if (listen(listener, 40) == -1)
		{
			printf("too many backlogged connections on listen");
			fflush(stdout);
			exit(1);
		}
	}

	// add the listener to the master set
	FD_SET(listener, &master);

	// keep track of the biggest file descriptor
	if (listener > highestsocket)
	{
		highestsocket = listener;
	}

	FD_SET(fileno(stdin), &master);

	if (fileno(stdin) > highestsocket)
	{
		highestsocket = fileno(stdin);
	}

}



int main(int argc, char** argv)
{
	int c, option_index = 0;
	char* configfile;

	while ((c = getopt_long(argc, argv, "m:p:", long_options, &option_index)) != EOF)
	{
		switch (c)
		{

		case 'm':
			SERVER_PROTOCOL = *optarg;
			break;
		case 'p':
			SERVERPORT = atoi(optarg);
			break;
		}
	}


	server_init();

	if (pthread_mutex_init(&mtx, NULL) < 0) {
		perror("pthread_mutex_init");
		exit(1);
	}



	server_run();

	return 0;
}

