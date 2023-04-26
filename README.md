To compile the code run make -f makeserver/makeclient depending on which folder you are in


***REQUIREMENTS**

[ ] Make new main file that will spawn a process. That process will be HandleFileTransferClient.
The main will send the socket to the HandleFileTransferClient
[ ] HFTC will then send the client options on what it would like to do
[ ] The client will choose one of the options
[ ] The options are 1. get a file 2. print directory 3. quit
[ ] Then HFTC will handle the request and send the appropriate information
[ ] 