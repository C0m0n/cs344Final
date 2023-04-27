#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define NAME_SIZE 21 /*Includes room for null */
#define FILE_NAME_SIZE 51 /*Includes room for null */

struct menu{
  unsigned char line1[20];
  unsigned char line2[20];
  unsigned char line3[20];
} men;

void DieWithError(char *errorMessage);  /* Error handling function */
void get(int, void *, unsigned int);
void put(int, void *, unsigned int);
unsigned int sendMenuAndWaitForResponse(int);
void askForName(int sock, char *, unsigned int);
void doSomethingWithName(char *, int);
void askForNumber(int sock, int *, unsigned int);
void doSomethingWithNumber(int);
void sendFileToClient(char * , int);
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
long findSize(FILE * fp);
void sendToClient(long fileSize, FILE * fp, int);

void HandleTCPClient(int clntSocket)
{
    int recvMsgSize;                    /* Size of received message */
    unsigned int response = 0;
    unsigned char name[NAME_SIZE]; //max length 20
    int number = 0;
    unsigned char errorMsg[] = "Invalid Choice";
    unsigned char bye[] = "Bye!";

    response = sendMenuAndWaitForResponse(clntSocket);
    while(response != 3)
    {
        switch(response)
        {
            case 1: printf("Client selected 1.\n");
                    askForName(clntSocket, name, NAME_SIZE);
                    doSomethingWithName(name, clntSocket);
                    break;
            case 2: printf("Client selected 2.\n");
                    askForNumber(clntSocket, &number, sizeof(int));
                    doSomethingWithNumber(number);
                    break;
            default: printf("Client selected junk.\n"); put(clntSocket, errorMsg, sizeof(errorMsg)); break;
        }
        response = sendMenuAndWaitForResponse(clntSocket);
    }//end while

    put(clntSocket, bye, sizeof(bye));
    close(clntSocket);    /* Close client socket */
    printf("Connection with client %d closed.\n", clntSocket);
}

unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    memset(&mainMenu, 0, sizeof(struct menu));   /* Zero out structure */
    strcpy(mainMenu.line1,"1) Transfer File\n");
    strcpy(mainMenu.line2, "2) Print Directory\n");
    strcpy(mainMenu.line3, "3) Quit\n");
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu));
    get(clntSocket, &response, sizeof(unsigned int));
    return ntohl(response);
}

void askForName(int sock, char * name, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter name:\n");
    put(sock, msg, sizeof(msg));
    memset(name, 0, NAME_SIZE);
    get(sock, name, NAME_SIZE);
}

void doSomethingWithName(char * name, int fileSocket)
{
    printf("Received name from the client: %s\n", name);
    sendFileToClient(name, fileSocket);

}

void askForNumber(int sock, int * numPtr, unsigned int size)
{
    unsigned char msg[21];
    int numIn = 0;

    memset(msg, 0, sizeof(msg));
    strcpy(msg, "Enter number:\n");
    put(sock, msg, sizeof(msg));
    get(sock, &numIn, sizeof(int));
    *numPtr = ntohl(numIn);
}

void doSomethingWithNumber(int number)
{
    printf("Received number from the client: %d\n", number);
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
}





////////////////////////////////
/*
void HandleFileTransferClient(int clntSock)
{
	FILE * fileSocket;
	unsigned char filenameLen;
	char filename[FILE_NAME_SIZE];
	
	if ((fileSocket = fdopen(clntSock, "r+")) == NULL)
		DieWithError("Error wrapping socket in FILE");
	
	fread(&filenameLen, sizeof(char), 1, fileSocket);
	printf("Length of filename: %d\n", filenameLen);
	
	if(filenameLen >= FILE_NAME_SIZE)
		DieWithError("File name too long");
	
	fread(filename, filenameLen, 1, fileSocket);
	filename[filenameLen] = '\0';
	printf("File name: %s\n", filename);
	
	sendFileToClient(filename, fileSocket);
	
	fclose(fileSocket);
}
*/
void sendFileToClient(char * filename, int fileSocket)
{
    long fileSize;
    // opening the file in read mode 
    
    printf("Opening file: %s", filename);
    strtok(filename, "\n"); // Remove newline character
    FILE* fp = fopen(filename, "r"); 
  
    // checking if the file exist or not 
    if (fp == NULL) 
		printf("File open failed.\n");
	
	//Get size of file
	fileSize = findSize(fp);
	
	//Send file size to client but convert to network order first
	fileSize = htonl(fileSize);
    printf("Sending file size: %ld\n", fileSize);
	put(fileSocket, fileSize, sizeof(long));
    printf("Sent file size\n");
	//fflush(fileSocket);
	
	//Change fileSize back to use it
	fileSize = ntohl(fileSize);
	printf("File size: %ld\n", fileSize);

	sendToClient(fileSize, fp, fileSocket);
	
    // closing the file 
    fclose(fp); 	
}

long findSize(FILE * fp) 
{
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long res = ftell(fp); 
  
	//resetting to the start
	fseek(fp, 0L, SEEK_SET);
	
    return res; 
}

void sendToClient(long fileSize, FILE * fp, int fileSocket)
{
	//Send file to client
	printf("Here is the file:\n");
	while (fileSize > 0)
	{

		char fileBuffer[1025];
		memset(fileBuffer, 0, 1025);
		if(fileSize > 1024)
		{
            printf("In if\n");
			fread(fileBuffer, 1024, 1, fp);
			put(fileSocket, fileBuffer, 1024);
			fileBuffer[1024] = '\0';
			fileSize -= 1024;
		}
		else
		{
            printf("In else\n");
			fread(fileBuffer, fileSize, 1, fp);
			put(fileSocket, fileBuffer, 1024);
			fileBuffer[fileSize] = '\0';
			fileSize = 0;			
		}
		//fflush(fileSocket);
		printf("%s", fileBuffer);
	}

}
/*
void HandleServerError(FILE * fileSocket){
	printf("Error sending Error Message\n");
	char errorMessage[16] = "File not found!";
	fwrite(errorMessage, 15, 1, fileSocket);
}
*/