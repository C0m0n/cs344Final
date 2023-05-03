#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>

#define RCVBUFSIZE 32   /* Size of receive buffer */
#define NAME_SIZE 21 /*Includes room for null */
#define FILE_NAME_SIZE 51 /*Includes room for null */

char * t1 ="xwrxwrxwr-------"; //This is NOT an error. They must be reversed.
char * t2 = "----------------";

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
void doSomethingWithNumber(int, int);
void sendFileToClient(char * , int);
void HandleTCPClient(int clntSocket);   /* TCP client handling function */
long findSize(FILE * fp);
void sendToClient(long fileSize, FILE * fp, int);
void ls_dir2();

//Main
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
                    // askForNumber(clntSocket, &number, sizeof(int));
                    doSomethingWithNumber(2, clntSocket);
                    break;
            default: printf("Client selected junk.\n"); put(clntSocket, errorMsg, sizeof(errorMsg)); break;
        }
        response = sendMenuAndWaitForResponse(clntSocket);
    }//end while
    //send bye
    put(clntSocket, bye, sizeof(bye));
    close(clntSocket);    /* Close client socket */
    printf("Connection with client %d closed.\n", clntSocket);
}


unsigned int sendMenuAndWaitForResponse(int clntSocket)
{
    struct menu mainMenu;
    unsigned int response = 0;
    //Clear out main menu
    memset(&mainMenu, 0, sizeof(struct menu));   /* Zero out structure */
    strcpy(mainMenu.line1,"1) Transfer File\n");
    strcpy(mainMenu.line2, "2) Print Directory\n");
    strcpy(mainMenu.line3, "3) Quit\n");
    printf("Sending menu\n");
    put(clntSocket, &mainMenu, sizeof(struct menu)); //send menu
    get(clntSocket, &response, sizeof(unsigned int)); //get response
    return ntohl(response); //Convert the responce from network to host byte order
}

void askForName(int sock, char * name, unsigned int size)
{
    unsigned char msg[21];
    memset(msg, 0, sizeof(msg)); //Clear message
    strcpy(msg, "Enter name:\n"); // Set message
    put(sock, msg, sizeof(msg)); //Send message
    memset(name, 0, NAME_SIZE); //Clear name
    get(sock, name, NAME_SIZE); //Get name
    name[strcspn(name, "\n")] = '\0'; //Remove newline
}

void doSomethingWithName(char * name, int fileSocket)
{
    //This function does nothing
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

void doSomethingWithNumber(int number, int sock)
{
    printf("Received number from the client: %d\n", number);
    ls_dir2();
}


void get(int sock, void *buffer, unsigned int bufferSize)
{
    int totalBytesReceived = 0;
    int bytesReceived = 0;
    memset(buffer, 0, bufferSize);
    while (totalBytesReceived < bufferSize) {
        bytesReceived = recv(sock, buffer + totalBytesReceived, bufferSize - totalBytesReceived, 0);
        if (bytesReceived < 0)
            DieWithError("recv() failed");
        else if (bytesReceived == 0)
            DieWithError("Connection closed prematurely");
        totalBytesReceived += bytesReceived;
    }
    printf("Received: %s\n", buffer);
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
    printf("Sent: %s\n", buffer);
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
    //This function just send the file size to the cliend then calls sendToClient to send the file contents.
    unsigned long fileSize;
    // opening the file in read mode 
    
    printf("Opening file: %s", filename);
    strtok(filename, "\n"); // Remove newline character
    FILE* fp = fopen(filename, "r"); //Open file
  
    // checking if the file exist or not 
    if (fp == NULL) 
		printf("File open failed.\n");
	
	//Get size of file
	fileSize = findSize(fp);
	
	//Send file size to client but convert to network order first
    printf("Sending file size: %ld\n", fileSize);
	put(fileSocket, &fileSize, sizeof(unsigned long));
    printf("Sent file size\n");

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
		memset(fileBuffer, 0, 1025); //Clear buffer
		if(fileSize > 1024)
		{
            // printf("In if\n");
            //Only send the first 1024 bytes
			fread(fileBuffer, 1024, 1, fp);
			put(fileSocket, fileBuffer, 1024);
			fileBuffer[1024] = '\0';
			fileSize -= 1024;
		}
		else
		{
            //Send the whole thing to the client
            // printf("In else\n");
			fread(fileBuffer, fileSize, 1, fp);
			put(fileSocket, fileBuffer, 1024);
			fileBuffer[fileSize] = '\0';
			fileSize = 0;			
		}
		// fflush(fileSocket);
		printf("%s", fileBuffer);
	}

}

void ls_dir2() {
    //Use current directory
    char *dname = "./";
  DIR *dp;
  struct dirent *dirp;
  struct stat dstat, *sp;
  //open directory
  dp = opendir(dname);
  char dnamecpy[1024];
  while ((dirp = readdir(dp)) != NULL) {
    strcpy(dnamecpy, dname); //Copy directory name
    strcat(dnamecpy, "/"); //Add slash
    strcat(dnamecpy, dirp->d_name); //Add file name
    lstat(dnamecpy, &dstat); //Get file info
    printf("%s", dirp->d_name);
    if (S_ISDIR(sp->st_mode)) {
      printf("*");
    }
    printf("\n");
  }

}

/*
void HandleServerError(FILE * fileSocket){
	printf("Error sending Error Message\n");
	char errorMessage[16] = "File not found!";
	fwrite(errorMessage, 15, 1, fileSocket);
}
*/