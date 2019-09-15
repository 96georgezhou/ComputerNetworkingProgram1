#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
using namespace std;

const int CONNECTION_REQUEST_SIZE = 10;
const int BUFFSIZE = 1500;
const string THREAD_MESSAGE = "Creating new thread with count: ";

int port;
int repetition;

struct thread_data // Input data to threads
{
	int thread_id;
	int clientFileDescriptor;
};

/**
 * Check if command is valid
 * @param argumentValues
 * @return
 */
int checkValidInput(char *argumentValues[])
{
	try
	{
		port = stoi(argumentValues[ 1 ]);
		repetition = stoi(argumentValues[ 2 ]);
	}
	catch ( invalid_argument &exception )
	{
		cout << "INVALID! Integers only!";
		cout << exception.what();
		return -1;
	}
	catch ( out_of_range &exception )
	{
		cout << "INVALID! Integers within range only!";
		cout << exception.what();
		return -1;
	}
	if ( port < 1024 || port > 65535 )
	{
		cout << "INVALID! Valid port only!";
		return -1;
	}
	if ( repetition < 0 )
	{
		cout << "INVALID! Valid repetition only!";
		return -1;
	}
	return 0;

}

/**
 * Calculate time elapsed after connecting with client
 * @param threadData
 * @return
 */
void *benchMark(void *threadData)
{
	char databuf[BUFFSIZE];
	struct thread_data *data;
	data = (struct thread_data *) threadData;
	struct timeval start;
	struct timeval stop;
	int count = 0;
	gettimeofday(&start , nullptr);
	for ( int i = 0; i <= repetition; i++ )
	{
		for ( int numberRead = 0;
			  (numberRead += read(data->clientFileDescriptor , databuf , BUFFSIZE - numberRead)) < BUFFSIZE;
			  ++count );
		count++;
	}

	gettimeofday(&stop , nullptr);
	long totalTime = (stop.tv_sec - start.tv_sec) * 1000000 + (stop.tv_usec - start.tv_usec);
	write(data->clientFileDescriptor , &count , sizeof(count));
	cout << "Data-receiving time for thread " + to_string(data->thread_id) + " = " + to_string(totalTime) + " usec"
		 << endl;
	cout << "Finish with thread " + to_string(data->thread_id) << endl;
	close(data->clientFileDescriptor); // close the connections
}


/**
 * Connect to the port number with number of repetitions
 * @param argumentNumber
 * @param argumentValues
 * @return
 */
int main(int argumentNumber , char *argumentValues[])
{
	if ( argumentNumber != 3 )
	{
		cout << "Please enter valid port number of repetitions";
		return -1;
	}
	int checkResult = checkValidInput(argumentValues);
	if ( checkResult == -1 )
	{
		cout << "Please enter argument values";
		return -1;
	}
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	memset(&hints , 0 , sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	int addressInfoStatus = getaddrinfo(nullptr , argumentValues[ 1 ] , &hints , &serverInfo);
	if ( addressInfoStatus != 0 )
	{
		cout << "Unable to connect";
		cout << gai_strerror(addressInfoStatus);
		return -1;
	}
	int socketFileDescriptor;
	int serverBindResult;
	struct addrinfo *possibleConnection;
	for ( possibleConnection = serverInfo;
		  possibleConnection != nullptr; possibleConnection = possibleConnection->ai_next )
	{
		socketFileDescriptor = socket(possibleConnection->ai_family , possibleConnection->ai_socktype ,
									  possibleConnection->ai_protocol);
		if ( socketFileDescriptor == -1 )
		{
			cout << "Invalid socket file descriptor";
			continue;
		}
		serverBindResult = bind(socketFileDescriptor , possibleConnection->ai_addr , possibleConnection->ai_addrlen);
		if ( serverBindResult == -1 )
		{
			cout << "Unable to bind to the socket using this file descriptor";
			close(socketFileDescriptor);
			continue;
		}
		break;
	}
	if ( possibleConnection == NULL )
	{
		cout << "Unable to connect or empty result was given";
		return -1;
	}
	freeaddrinfo(serverInfo);
	int listenUsingSocketResult = listen(socketFileDescriptor , CONNECTION_REQUEST_SIZE);
	if ( listenUsingSocketResult != 0 )
	{
		cout << "Unable to listen using the socket file descriptor";
		return -1;
	}
	int count = 1;
	while ( true )
	{
		struct sockaddr_storage clientSocket;
		socklen_t clientSocketSize = sizeof(clientSocket);
		int clientFileDescriptor = accept(socketFileDescriptor , (struct sockaddr *) &clientSocket , &clientSocketSize);
		if ( clientFileDescriptor == -1 )
		{
			cout << "Unable to connect to client. Trying again";
			continue;
		}
		pthread_t new_thread;
		struct thread_data data;
		data.thread_id = count;
		data.clientFileDescriptor = clientFileDescriptor;
		cout << THREAD_MESSAGE + to_string(count) << endl;
		int threadResult = pthread_create(&new_thread , nullptr , benchMark , (void *) &data);
		if ( threadResult != 0 )
		{
			cout << "Unable to create thread. Trying again";
			continue;
		}
		count++;
	}
}
