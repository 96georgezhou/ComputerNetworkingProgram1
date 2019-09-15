#include <iostream>
#include <netdb.h>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

const int TOTAL_SIZE = 1500;
const int MULTIPLE_WRITE = 1;
const int WRITEV = 2;
const int SINGLE_WRITE = 3;

int port;
int repetition;
int numberBuffer;
int bufferSize;
char *serverIPName;
int type;

/**
 * Check if input valid from server
 * @param argumentValues
 * @return
 */
int checkValidInput(char *argumentValues[])
{
    try
    {
        port = stoi(argumentValues[ 1 ]);
        repetition = stoi(argumentValues[ 2 ]);
        numberBuffer = stoi(argumentValues[ 3 ]);
        bufferSize = stoi(argumentValues[ 4 ]);
        type = stoi(argumentValues[ 6 ]);
    }
    catch (invalid_argument &exception) {
        cout << "INVALID! Integers only!";
        cout << exception.what();
        return -1;
    }
    catch (out_of_range &exception) {
        cout << "INVALID! Integers within range only!";
        cout << exception.what();
        return -1;
    }
    serverIPName = argumentValues[ 5 ];
    if ( port < 1024 || port > 65535 )
    {
        cout << "Invalid port number";
        return -1;
    }
    if ( repetition < 0 )
    {
        cout << " Invalid number of repetition";
        return -1;
    }
    if ( numberBuffer * bufferSize != TOTAL_SIZE )
    {
        cout << "Invalid number of data buffer or data buffer size. " +
                to_string(TOTAL_SIZE);
        return -1;
    }
    if ( string(serverIPName).empty())
    {
        cout << "Invalid server name";
        return -1;
    }
    if ( type < 1 || type > 3 )
    {
        cout << "Invalid transfer scenario. Only accept " + to_string(MULTIPLE_WRITE) + ", " + to_string(WRITEV)
                + " or " + to_string(SINGLE_WRITE);
        return -1;
    }
    return 0;
}

/**
 * Print function for Client
 * @param start
 * @param lap
 * @param end
 * @param numberOfRead
 */
void printResult(struct timeval start , struct timeval lap , struct timeval end , int numberOfRead)
{
    // Second to microsecond needs to scale by 1000000
    long dataSendingTime = (lap.tv_sec - start.tv_sec) * 1000000 + lap.tv_usec - start.tv_usec;
    long roundTripTime = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
    cout << "Test 1: ";
    cout << "data-sending time = " + to_string(dataSendingTime) + " usec, ";
    cout << "round-trip time = " + to_string(roundTripTime) + " usec, ";
    cout << "Number of reads =  " + to_string(numberOfRead) << endl;
    return;
}

/**
 * Main function for Client
 * @param argumentNumber
 * @param argumentValues
 * @return
 */
int main(int argumentNumber , char *argumentValues[])
{
    if ( argumentNumber != 7 )
    {
        cout << "Incorrect number of arguments provided";
        return -1;
    }
    int checkResult = checkValidInput(argumentValues);
    if ( checkResult == -1 )
    {
        cout << "Invalid argument supplied. Please enter argument values";
        return -1;
    }
    char databuf[numberBuffer][bufferSize];
    struct addrinfo hints; //define what the getaddrinfo going to do.
    struct addrinfo *serverInfo;
    memset(&hints , 0 , sizeof(hints));
    hints.ai_family = AF_UNSPEC; // either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    int addrInfoStatus = getaddrinfo(serverIPName , argumentValues[ 1 ] , &hints , &serverInfo);
    if ( addrInfoStatus != 0 )
    {
        cout << "Unable to connect";
        cout << gai_strerror(addrInfoStatus);
        return -1;
    }
    struct addrinfo *possibleConnection;
    int socketFileDescriptor;
    int socketConnectionResult;
    for ( possibleConnection = serverInfo;
          possibleConnection != NULL; possibleConnection = possibleConnection->ai_next )
    {
        socketFileDescriptor = socket(possibleConnection->ai_family , possibleConnection->ai_socktype ,
                                      possibleConnection->ai_protocol);
        if ( socketFileDescriptor == -1 )
        {
            cout << "Invalid one socket file descriptor detected";
            continue;
        }
        socketConnectionResult = connect(socketFileDescriptor , possibleConnection->ai_addr ,
                                         possibleConnection->ai_addrlen);
        if ( socketConnectionResult == -1 )
        {
            cout << "Invalid one socket connection result detected.";
            continue;
        }
        cout << "Found a connection. Breaking out";
        break;
    }
    // If still null, then it means that we went through all possible connections but none satisfied
    if ( possibleConnection == NULL )
    {
        cout << "Unable to connect or empty result was given";
        return -1;
    }
    freeaddrinfo(serverInfo);
    struct timeval start , lap , end;
    gettimeofday(&start , NULL);
    for ( int i = 0; i <= repetition; i++ )
    {
        if (type == 1) {
            for ( int j = 0; j < numberBuffer; j++ )
            {
                write(socketFileDescriptor , databuf[ j ] , bufferSize);
            }
        }
        if (type == WRITEV) {
            struct iovec vector[numberBuffer];
            for ( int j = 0; j < numberBuffer; j++ )
            {
                vector[ j ].iov_base = databuf[ j ];
                vector[ j ].iov_len = bufferSize;
            }
            writev(socketFileDescriptor , vector , numberBuffer);
        }
        if (type == SINGLE_WRITE)
        {
            write(socketFileDescriptor , databuf , numberBuffer * bufferSize);
        }
    }
    gettimeofday(&lap, nullptr);
    int numberOfRead;
    read(socketFileDescriptor , &numberOfRead , sizeof(numberOfRead));
    gettimeofday(&end , nullptr);
    printResult(start , lap , end , numberOfRead);
    return 0;
}
