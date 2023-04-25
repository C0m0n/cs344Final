/* filetransferclient.c */
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define RCVBUFSIZE 100   /* Size of receive buffer */
#define FILE_NAME_SIZE 51 /*Includes room for null */

void DieWithError(char *errorMessage);  /* Error handling function */
void talkToServer(FILE * fileSocket);
void getFile(long fileSize, FILE * fileSocket);

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in fileServAddr; /* File server address */
    unsigned short fileServPort;     /* File server port */
    char *servIP;                    /* Server IP address (dotted quad) */

    if ((argc < 2) || (argc > 3))    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */

    if (argc == 3)
        fileServPort = atoi(argv[2]); /* Use given port, if any */
    else
        fileServPort = 7;  /* 7 is the well-known port for the echo service */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&fileServAddr, 0, sizeof(fileServAddr));     /* Zero out structure */
    fileServAddr.sin_family      = AF_INET;             /* Internet address family */
    fileServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    fileServAddr.sin_port        = htons(fileServPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &fileServAddr, sizeof(fileServAddr)) < 0)
        DieWithError("connect() failed");

	FILE * fileSocket;
	
	if ((fileSocket = fdopen(sock, "r+")) == NULL)
		DieWithError("Error wrapping socket in FILE");

    talkToServer(fileSocket);

    fclose(fileSocket);
    exit(0);
}

void talkToServer(FILE * fileSocket)
{
  char filename[FILE_NAME_SIZE];   /* Name of file on server. Includes space for NULL char */
	unsigned char filenameLen;
	long fileSize = 0;
	unsigned int totalBytesRcvd = 0;
	
	printf("Enter name of file: ");
	fgets(filename, FILE_NAME_SIZE, stdin);
	filenameLen = strlen(filename);  //This includes the \n if that was pressed before length ran out.
	if(filename[filenameLen-1] == '\n')
	{
		filename[filenameLen-1] = '\0';
		filenameLen--;  //drop length by 1
	}
	else
		filename[filenameLen] = '\0';
	
	if(filenameLen == 0)
		DieWithError("No filename entered");
	/* Send length */
	fwrite(&filenameLen, sizeof(char), 1, fileSocket);
	/* Send filename */
	fwrite(filename, filenameLen, 1, fileSocket);
	fflush(fileSocket);
	
	//Get file size
	fread(&fileSize, sizeof(long), 1, fileSocket);
	//Convert to host order
	fileSize = ntohl(fileSize);
	printf("Size of file: %ld\n", fileSize);
	
	getFile(fileSize, fileSocket);
	
}

void getFile(long fileSize, FILE * fileSocket)
{
	printf("Here is the file:\n");
	//get file
	while (fileSize > 0)
	{
		char fileBuffer[1025];
		memset(fileBuffer, 0, 1025);
		if(fileSize > 1024)
		{
			fread(fileBuffer, 1024, 1, fileSocket);
			fileBuffer[1024] = '\0';
			fileSize -= 1024;
		}
		else
		{
			int n = fread(fileBuffer, fileSize, 1, fileSocket);
			//printf("n is %d\n", n);
			fileBuffer[fileSize] = '\0';
			fileSize = 0;			
		}
		
		printf("%s", fileBuffer);
	}
	printf("\n");
}
