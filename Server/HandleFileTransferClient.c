#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <stdlib.h>
#include <string.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define FILE_NAME_SIZE 51 /*Includes room for null */

void DieWithError(char *);  /* Error handling function */
void sendFileToClient(char *, FILE *);
long findSize(FILE *);
void sendToClient(long fileSize, FILE * fp, FILE * fileSocket);
void HandleServerError(FILE * );

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

void sendFileToClient(char * filename, FILE * fileSocket)
{
	long fileSize;
    // opening the file in read mode 
    FILE* fp = fopen(filename, "r"); 
  
    // checking if the file exist or not 
    if (fp == NULL) 
		HandleServerError(fileSocket);
	
	//Get size of file
	fileSize = findSize(fp);
	
	//Send file size to client but convert to network order first
	fileSize = htonl(fileSize);
	fwrite(&fileSize, sizeof(long), 1, fileSocket);
	fflush(fileSocket);
	
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

void sendToClient(long fileSize, FILE * fp, FILE * fileSocket)
{
	//Send file to client
	printf("Here is the file:\n");
	while (fileSize > 0)
	{

		char fileBuffer[1025];
		memset(fileBuffer, 0, 1025);
		if(fileSize > 1024)
		{
			fread(fileBuffer, 1024, 1, fp);
			fwrite(fileBuffer, 1024, 1, fileSocket);
			fileBuffer[1024] = '\0';
			fileSize -= 1024;
		}
		else
		{
			fread(fileBuffer, fileSize, 1, fp);
			fwrite(fileBuffer, fileSize, 1, fileSocket);
			fileBuffer[fileSize] = '\0';
			fileSize = 0;			
		}
		fflush(fileSocket);
		printf("%s", fileBuffer);
	}

}
void HandleServerError(FILE * fileSocket){
	printf("Error sending Error Message\n");
	char errorMessage[16] = "File not found!";
	fwrite(errorMessage, 15, 1, fileSocket);
}