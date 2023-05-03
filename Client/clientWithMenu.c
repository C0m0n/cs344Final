#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define RCVBUFSIZE 100   /* Size of receive buffer */
#define NAME_SIZE 21 /*Includes room for null */

typedef struct{
  unsigned int x;
  unsigned int y;
  unsigned char oper;
}TRANS_DATA_TYPE;

typedef struct{
  unsigned int x;
  unsigned int y;
}DATA_TYPE;

struct menu{
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
};

void DieWithError(char *errorMessage);  /* Error handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
void talkToServer(int);
unsigned int displayMenuAndSendSelection(int);
void sendName(int);
void sendNumber(int);
void getFile(unsigned long, int);
void getDirectory(int);

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char *echoString;                /* String to send to echo server */
    unsigned int echoStringLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv()
                                       and total bytes read */
    int answer;

    DATA_TYPE data;
    TRANS_DATA_TYPE incoming;
    memset(&incoming, 0, sizeof(TRANS_DATA_TYPE));

    if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        echoServPort = atoi(argv[2]); /* Use given port, if any */
    else
        echoServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    echoServAddr.sin_port        = htons(echoServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

   // echoStringLen = strlen(echoString);          /* Determine input length */

    talkToServer(sock);

    close(sock);
    exit(0);
}

void talkToServer(int sock)
{
    
    unsigned int selection = 0;
    unsigned char bye[5];
    unsigned long fileSize = 0;
    unsigned long numbOfFiles = 0;
    // struct menu menuBuffer;

    while(1)
    {
        // memset(&menuBuffer, 0, sizeof(struct menu));
        // get(sock, &menuBuffer, sizeof(struct menu));
        // printf("%s\n", menuBuffer.line1);
        // printf("%s\n", menuBuffer.line2);
        // printf("%s\n", menuBuffer.line3);
        // printf("Befrore display menu\n");
        selection = displayMenuAndSendSelection(sock);
        printf("Client selected: %d\n", selection);
        switch(selection)
        {
            case 1:
                sendName(sock);
                get(sock, &fileSize, sizeof(unsigned long)); //Get the file size from the server
                getFile(fileSize, sock); //This function will get the file from the server using the file size
                printf("File received\n");
                break;
            case 2:
                sendNumber(sock);
                // get(sock, &numbOfFiles, sizeof(unsigned long));
                //getDirectory(sock);
                break;
            }
        if(selection == 3) break;
    }
    selection = htonl(selection);
    put(sock, &selection, sizeof(unsigned int));
    get(sock, bye, 5);
    printf("%s\n", bye);
}

unsigned int displayMenuAndSendSelection(int sock)
{
    struct menu menuBuffer;     /* Buffer for echo string */
    unsigned int response = 0;
    unsigned int output;
    char junk;

    // printf("Inside client display menu\n");
    //Receive menu from server
    memset(&menuBuffer, 0, sizeof(struct menu));
    get(sock, &menuBuffer, sizeof(struct menu));  //in this case server is also sending null
    printf("%s\n", menuBuffer.line1);
    printf("%s\n", menuBuffer.line2);
    printf("%s\n", menuBuffer.line3);
    scanf("%d", &response); //Wait for the user to enter a selection
    getc(stdin);
    output = htonl(response);
    put(sock, &output, sizeof(unsigned int)); //Send the selection to the server
    return response;
}

void sendName(int sock)
{
    unsigned char msg[21];
    unsigned char name[NAME_SIZE];

    memset(msg, 0, sizeof(msg)); //clear the buffer
    get(sock, msg, sizeof(msg)); //get the message from the server
    printf("%s\n", msg);
    memset(name, 0, NAME_SIZE); //clear the buffer
    fgets(name, NAME_SIZE, stdin); //get the name of the file from the user
    printf("%s\n", name);
    put(sock, name, NAME_SIZE); //send the name of the file to the server
}

void sendNumber(int sock)
{
    //This function will send the server the number that the user selected.
    unsigned char msg[21];
    int number;
    char buffer[4000];
    memset(msg, 0, sizeof(msg));
    //get(sock, msg, sizeof(msg));
    //printf("%s\n", msg);
    // scanf("%d", &number);
    // number = htonl(number);
    // put(sock, &number, sizeof(int));
    get(sock, buffer, 1024);
    printf("%s\n", buffer);
}



void get(int sock, void *buffer, unsigned int bufferSize)
{
    int totalBytesReceived = 0;
    int bytesReceived = 0;

    while (totalBytesReceived < bufferSize) {
        bytesReceived = recv(sock, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
        if (bytesReceived < 0)
            DieWithError("recv() failed");
        else if (bytesReceived == 0)
            DieWithError("Connection closed prematurely");
        totalBytesReceived += bytesReceived;
    }
    // printf("Received: %s\n", buffer);
}

void put(int sock, void *buffer, unsigned int bufferSize)
{
    int totalBytesSent = 0;
    int bytesSent = 0;

    while (totalBytesSent < bufferSize) {
        bytesSent = send(sock, buffer + totalBytesSent, bufferSize - totalBytesSent, 0);
        if (bytesSent < 0)
            DieWithError("send() failed");
        totalBytesSent += bytesSent;
    }
    // printf("Sent: %s\n", buffer);
}

void getFile(unsigned long fileSize, int fileSocket)
{
	printf("Here is the file:\n");
    printf("File size is %ld \n ", fileSize);
	//get file
	while (fileSize > 0)
	{
		char fileBuffer[1024];
		memset(fileBuffer, 0, 1024);
		if(fileSize > 1024)
		{
			get(fileSocket, fileBuffer, 1024);
			fileBuffer[1024] = '\0';
			fileSize -= 1024;
		}
		else
		{
			get(fileSocket,fileBuffer, sizeof(fileBuffer));
			//printf("n is %d\n", n);
			fileBuffer[fileSize] = '\0';
			fileSize = 0;			
		}
		
		printf("%s", fileBuffer);
        // printf("File size is %ld\n", fileSize);
	}
    // printf("DONENENENENENEN\n");
	// printf("\n");
}

void getDirectory(int sock){

    char buffer[4000];
    
    

    
    
    
    
}