#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/signal.h>
#include <sys/wait.h>

#define BUFLEN 100
#define peerLength 10
#define contentLength 10
#define maxContent 10

 typedef struct PDU{
        char type;
        char data[BUFLEN];
}pud;

//For hosting file server
int hosting(int downloadClient){
    pud toDownloadClient;
    pud fromDownloadClient;
    FILE *fp;
    read(downloadClient, &fromDownloadClient, sizeof(fromDownloadClient) + 1);
    printf("File Requested\n");
    if(fromDownloadClient.type == 'D'){
        printf("Content Requested: %s\n", fromDownloadClient.data);
        if((fp = fopen(fromDownloadClient.data, "r")) != NULL){
            fscanf(fp, "%s", toDownloadClient.data);
            toDownloadClient.type = 'C';
            write(downloadClient, &toDownloadClient, sizeof(toDownloadClient) + 1);
        }
        else{
            toDownloadClient.type = 'E';
            strcpy(toDownloadClient.data, "Content does not exist");
        }
        fclose(fp);
    }
    return(0);
}

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}

int main(int argc, char **argv){  
    int downloadClient, client_len;     
    int i, j, k;
    int numberOfRegistedContent = 0;
    int TCP;
    int UDP;
    int alen;
    int IndexPort = 15000;
    int port;
    int numOfContent = 0;
    char input;
    char *charPort;
    struct sockaddr_in clientADDR;
    struct hostent *phe;
    struct sockaddr_in hostingAddr;
    struct sockaddr_in clientAddr;
    char peerAddr[] = "192.168.0.45";
    char *peerName;
    char contentList[contentLength][contentLength];
    char index[] = "192.168.0.44";
    pud toIndexServer;
    pud receivedData;
    port = atoi(argv[2]);
    peerName = argv[1];
    charPort= argv[2];
    bool registrationPassed;
    
    for(i = 0; i < 10;i++){
    	memset(contentList[i], '\0', sizeof(contentList[i]));
    }

    //UDP Socket
    memset(&clientADDR, 0, sizeof(clientADDR));
    clientADDR.sin_family = AF_INET;
    clientADDR.sin_port = htons(IndexPort);

    /* Map host name to IP address, allowing for dotted decimal */
    if ( phe = gethostbyname(index) ){
            memcpy(&clientADDR.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (clientADDR.sin_addr.s_addr = inet_addr(index)) == INADDR_NONE )
    fprintf(stderr, "Can't get host entry \n");

    alen = sizeof(clientADDR);
    //UCP Socket
    UDP = socket(AF_INET, SOCK_DGRAM, 0);
    connect(UDP, (struct sockaddr *)&clientADDR, sizeof(clientADDR));
    
    //Used for below
    char tempContent[contentLength];
    FILE *f;

    while(1){
        printf("What do you want to do?\n");
        printf("R : Register Content\n");
        printf("S : Search Index Server for File\n");
        printf("T : De-Register Content\n");
        printf("O : Full List of Content Available\n");
        printf("Q : Quit program and de-register all content\n");
        input = getchar();

        //Registeration Request
        if(input == 'R'){
            printf("Please enter the content name you would like to register:\n");
            scanf("%s", tempContent);

            FILE *f;
            //Check for content existence on machine
            if((f = fopen(tempContent, "r")) == NULL){
                printf("Content does not exist\n");
                continue;
            }
            fclose(f);

            //Check for content match
            for(i = 0; i < 10; i++){
                if(strcmp(tempContent, contentList[i]) == 0){
                    printf("Error, content already registered\n");
                    continue;
                }
            }
            

            //Set the PDU that is sent to the server
            strcpy(toIndexServer.data, peerName);
            for(i = strlen(peerName); i < peerLength; i++){
                strcat(toIndexServer.data, " ");
            }
            strcat(toIndexServer.data, tempContent);
            for(i = peerLength + strlen(tempContent); i < peerLength + contentLength; i++){
                strcat(toIndexServer.data, " ");
            }
            strcat(toIndexServer.data, charPort);
            toIndexServer.type = 'R';
	
            //Send and receive from index server
            sendto(UDP, (struct PDU*) &toIndexServer, sizeof(toIndexServer) + 1, 0, (struct sockaddr*) &clientADDR, alen);
            recvfrom(UDP, (struct PDU*) &receivedData, sizeof(receivedData) + 1, 0, (struct sockaddr*) &clientADDR, &alen);
	
	    printf("%s\n", receivedData.data);
            //Print out if Error Recieved
            if(receivedData.type == 'E'){
                printf("%s\n", receivedData.data);
                continue;
            }
            //If recieved an A, then move to create content server
            else if(receivedData.type == 'A'){
                printf("Registration Complete\n");
                strcpy(contentList[numberOfRegistedContent], tempContent);
                numberOfRegistedContent++;
                registrationPassed = true;
            }

            //Create content hosting server if registration passed
            if(registrationPassed){
                registrationPassed = false;

                //TCP socket for hosting
                TCP = socket(AF_INET, SOCK_STREAM, 0);
                bzero((char *) &hostingAddr, sizeof(struct sockaddr_in));
                hostingAddr.sin_family = AF_INET;
                hostingAddr.sin_port = htons(port);
                hostingAddr.sin_addr.s_addr = htonl(INADDR_ANY);
                bind(TCP, (struct sockaddr*) &hostingAddr, sizeof(hostingAddr));
                listen(TCP, 5);

                //Create Child process
                    client_len = sizeof(clientAddr);
                    downloadClient = accept(TCP, (struct sockaddr*) &clientAddr, &client_len);
                    switch(fork()){
                        case 0:
                            (void) close(TCP);
                            exit(hosting(downloadClient));
                        default:
                            (void) close(downloadClient);
                    }
            }
        }

        //Download Request
        else if(input == 'S'){
            printf("What is the name of the content you want to download?\n");
            scanf("%s", tempContent);
            bool downloadSuccessful = false;
            bool registrationSuccesful = false;

            //Send search request to index server
            toIndexServer.type = 'S';
            strcpy(toIndexServer.data, tempContent);
            sendto(UDP, (struct PDU*) &toIndexServer, sizeof(toIndexServer) + 1, 0, (struct sockaddr*) &clientADDR, alen);
            recvfrom(UDP, (struct PDU*) &receivedData, sizeof(receivedData) + 1, 0, (struct sockaddr*) &clientADDR, &alen);
            if(receivedData.type == 'S'){
                char contentPort[6];

                //Save the client port
                for(i = 0; i < 5; i++){
                    if(receivedData.data[i] == '\0'){
                        break;
                    }
                    contentPort[i] = receivedData.data[i];
                }
                contentPort[i] = '\0';
                
                //Download the file from client server
                int TCPDownload;
                pud toDownloadServer;
                int reader;
                FILE *f;

                //Create TCP Socket Stream
                struct hostent *contentServer;
                struct sockaddr_in contentServerAddr;
                TCPDownload = socket(AF_INET, SOCK_STREAM, 0);
                bzero((char *)&contentServerAddr, sizeof(struct sockaddr_in));
                contentServerAddr.sin_family = AF_INET;
                contentServerAddr.sin_port = htons(atoi(contentPort));
                contentServer = gethostbyname(peerAddr);
                bcopy(contentServer -> h_addr, (char *)&contentServerAddr.sin_addr, contentServer -> h_length);

                //Connect to server
                connect(TCPDownload, (struct sockaddr*) &contentServerAddr, sizeof(contentServerAddr));

                //Set values for download
                toDownloadServer.type = 'D';
                strcpy(toDownloadServer.data, tempContent);

                //Send File request to file server
                write(TCPDownload, (struct PDU *) &toDownloadServer, sizeof(toDownloadServer));

                //Write to file or print out error
                f = fopen(tempContent, "w+");
                while(reader = read(TCPDownload,(struct PDU *) &receivedData, sizeof(receivedData) + 1)){
                    if(receivedData.type == 'E'){
                        printf("%s\n", receivedData.data);
                        break;
                    }
                    else if(receivedData.type == 'C'){
                        fwrite(receivedData.data, sizeof(receivedData.data), 1, f);
                        downloadSuccessful = true;
                    }
                }
                fclose(f);
                close(TCPDownload);
                
                //Register as a content server
                if(downloadSuccessful){
                    printf("Download Successful\n");
                    
                    //Set the PDU that is sent to the server
                    strcpy(toIndexServer.data, peerName);
                    for(i = strlen(peerName); i < peerLength; i++){
                        strcat(toIndexServer.data, "\0");
                    }
                    strcat(toIndexServer.data, tempContent);
                    for(i = peerLength; i < peerLength + contentLength; i++){
                        strcat(toIndexServer.data, "\0");
                    }
                    strcat(toIndexServer.data, charPort);
                    toIndexServer.type = 'R';

                    //Send and receive from index server
                    sendto(UDP, (struct PDU*) &toIndexServer, sizeof(toIndexServer) + 1, 0, (struct sockaddr*) &clientADDR, alen);
                    recvfrom(UDP, (struct PDU*) &receivedData, sizeof(receivedData) + 1, 0, (struct sockaddr*) &clientADDR, &alen);

                    //Print out if Error Recieved
                    if(receivedData.type == 'E'){
                        printf("%s\n", receivedData.data);
                        continue;
                    }
                    //If recieved an A, then move to create content server
                    else if(receivedData.type == 'A'){
                        printf("Registration Complete\n");
                        strcpy(contentList[numberOfRegistedContent], tempContent);
                        numberOfRegistedContent++;
                        registrationPassed = true;
                    }

		        //Create Child process
		            client_len = sizeof(clientAddr);
		            downloadClient = accept(TCP, (struct sockaddr*) &clientAddr, &client_len);
		            switch(fork()){
		                case 0:
		                    (void) close(TCP);
		                    exit(hosting(downloadClient));
		                default:
		                    (void) close(downloadClient);
		            }
                }
            }
            else if(receivedData.type == 'E'){
                printf("%s\n", receivedData.data);
            }
        }

        //De-Register Content
        else if(input == 'T'){
            bool deregistered = false;
            bool contentFound = false;

            printf("Please enter the content name you would like to remove:\n");
            scanf("%s\n", tempContent);

            //Search if content exists
            for(i = 0; i < maxContent; i++){
                if(strcmp(tempContent, contentList[i]) == 0){
                    contentFound = true;
                    break;
                }
            }

            //If the content is found, then send the requst to the server to deregister
            if(contentFound){
                toIndexServer.type = 'T';
                
                //Copy the peer name and content name to be sent to the server
                strcpy(toIndexServer.data, peerName);
                for(i = strlen(peerName); i < peerLength; i++){
                    strcat(toIndexServer.data, "\0");
                }
                strcat(toIndexServer.data, tempContent);

                sendto(UDP, (struct PDU*) &toIndexServer, sizeof(toIndexServer) + 1, 0, (struct sockaddr*) &clientADDR, alen);
                recvfrom(UDP, (struct PDU*) &receivedData, sizeof(receivedData) + 1, 0, (struct sockaddr*) &clientADDR, &alen);
                if(receivedData.type == 'A'){
                    if(i == maxContent - 1){
                        memset(contentList[i], '\0', sizeof(contentList[i]));
                    }
                    else{
                        for(i; i < numberOfRegistedContent; i++){
                            strcpy(contentList[i], contentList[i + 1]);
                        }
                        memset(contentList[numberOfRegistedContent], '\0', sizeof(contentList[numberOfRegistedContent]));
                    }
                    numberOfRegistedContent--;
                    printf("%s\n", receivedData.data);
                    deregistered = true;
                }
                else if(receivedData.type == 'E'){
                    printf("%s\n", receivedData.data);
                    continue;
                }
            }
            else{
                printf("Error content not found");
            }
        }

        //List of content available
        else if(input == 'O'){
            toIndexServer.type = 'O';
            sendto(UDP, (struct PDU*) &toIndexServer, sizeof(toIndexServer) + 1, 0, (struct sockaddr*) &clientADDR, alen);
            for(i = 0; i < 100; i++){
                recvfrom(UDP, (struct PDU*) &receivedData, sizeof(receivedData) + 1, 0, (struct sockaddr*) &clientADDR, &alen);
                if(receivedData.type == 'O'){
                    printf("%s\n", receivedData.data);
                }
                else if(receivedData.type == 'F'){
                    printf("%s\n", receivedData.data);
                    break;
                }
            }
        }

        else if(input == 'Q'){
            bool readyQuit = false;
            bool dereg = false;
            toIndexServer.type = 'T';
            int tempNumber = numberOfRegistedContent;
            for(i = 0; i < tempNumber; i++){

                //Copy the peer name and content name to be sent to the server
                strcpy(toIndexServer.data, peerName);
                for(i = strlen(peerName); i < peerLength; i++){
                    strcat(toIndexServer.data, "\0");
                }
                strcat(toIndexServer.data, tempContent);

                sendto(UDP, (struct PDU*) &toIndexServer, sizeof(toIndexServer) + 1, 0, (struct sockaddr*) &clientADDR, alen);
                recvfrom(UDP, (struct PDU*) &receivedData, sizeof(receivedData) + 1, 0, (struct sockaddr*) &clientADDR, &alen);
                if(receivedData.type == 'A'){
                    if(i == maxContent - 1){
                        memset(contentList[i], '\0', sizeof(contentList[i]));
                    }
                    else{
                        for(i; i < numberOfRegistedContent; i++){
                            strcpy(contentList[i], contentList[i + 1]);
                        }
                        memset(contentList[numberOfRegistedContent], '\0', sizeof(contentList[numberOfRegistedContent]));
                    }
                    printf("%s\n", receivedData.data);
                    dereg = true;
                }
                else if(receivedData.type == 'E'){
                    printf("%s\n", receivedData.data);
                    continue;
                }

                if(dereg){
                    printf("Content Deleted");
                    numberOfRegistedContent--;
                    dereg = false;
                }
            }
            if(numberOfRegistedContent == 0){
                readyQuit = true;
            }
            if(readyQuit){
                exit(0);
            }
        }
    }
}
