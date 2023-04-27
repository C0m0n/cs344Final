
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
