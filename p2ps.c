#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUFLEN 100
#define peerLength 10
#define contentLength 10
#define maxPeers 50

int main(int argc, char **argv){

    char clientIP[] = "192.168.0.45";


    //PDU Structure
    typedef struct PDU{
        char type;
        char data[BUFLEN];
    }pud;
    pud recvData;
    pud toClient;

    //Peer structure
    typedef struct peerStorage{
        char name[peerLength];
        char contentList[contentLength][contentLength];
        char address[16];
        char ports[10];
        int contentNumber;
    }peer;
    int numberOfPeers = 0;
    
    //Arraylist of peer struct
    peer peerList[10];
    int i; int j; int k; int n;

    //Initialize peer list
    for(i = 0; i < 10; i++){
        memset(peerList[i].name, '\0', sizeof(peerList[i].name));
        memset(peerList[i].address, '\0', sizeof(peerList[i].address));
        memset(peerList[i].ports, '\0', sizeof(peerList[i].ports));
        peerList[i].contentNumber= 0;
        for(j = 0; j < 10; j++){
            memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
        }
    }

    //Setting up the server
    struct sockaddr_in serverADDR;
    struct sockaddr_in clientADDR;
    struct hostent *hp;
    int PORT = 15000;
    int addrlen;
    int UDP;
    int type;

    //Create TCP and bind listening socket
    memset(&serverADDR, 0, sizeof(serverADDR));
    memset(&clientADDR, 0, sizeof(clientADDR));
    serverADDR.sin_family = AF_INET;
    serverADDR.sin_addr.s_addr = INADDR_ANY;
    serverADDR.sin_port = htons(PORT);
    
    //Allocate a Socket and bind socket for UDP
    UDP = socket(AF_INET, SOCK_DGRAM, 0);
    bind(UDP, (struct sockaddr *)&serverADDR, sizeof(serverADDR));
    listen(UDP, 5);
    addrlen = sizeof(clientADDR);

    printf("P2P Network Ready, Waiting for clients\n");

    while(1){
        recvfrom(UDP, (struct PDU *) &recvData, sizeof(recvData) + 1, 0, (struct sockaddr *) &clientADDR, &addrlen); 
	
	
        //Content Registration
        if (recvData.type == 'R'){
            printf("Registering Peer or Content\n");
            bool hasFileRegistered = false;
            bool nameConflict = false;
            bool contentConflict = false;
            char peerName[peerLength];
            char contentName[contentLength];
            char tempPort[10];

            //Get the Peer Name
            for(i = 0; i < peerLength; i++){
                if(recvData.data[i] == ' '){
                    break;
                }
                peerName[i] = recvData.data[i];
            }
            peerName[i] = '\0';
            
            //Get the Content Name
            j = 0;
            for(i = peerLength; i < peerLength + contentLength; i++){
                if(recvData.data[i] == ' '){
                    break;
                }
                contentName[j] = recvData.data[i];
                j++;
            }
            contentName[j] = '\0';

            //Get Port of client
            j = 0;
            for(i = peerLength + contentLength; i < peerLength + contentLength + 5; i++){
                if(recvData.data[i] == '\0'){
                    break;
                }
                tempPort[j] = recvData.data[i];
                j++;
            }
            tempPort[j] = '\0';            

            //Check for Peer name conflict
            for(n = 0; n < maxPeers; n++){
                if(strcmp(peerName,peerList[n].name) == 0){
                    if(strcmp(tempPort, peerList[n].ports) == 0){
                        hasFileRegistered = true;
                        break;
                    }
                    else{
                        nameConflict = true;
                        break;
                    }
                }
            }

            //Check for content name conflict
            for(j = 0; j < maxPeers; j++){
                for(k = 0; k < contentLength; k++){
                    if(strcmp(contentName, peerList[j].contentList[k]) == 0){
                        contentConflict = true;
                        break;
                    }
                }
            }

            //If the peer has not been registered
            if(hasFileRegistered == false){
                //Error Messages
                if(nameConflict){
                    printf("Error, Peer name is in use\n");
                    toClient.type = 'E';
                    strcpy(toClient.data, "Peer name is in use\0");
                    sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                }
                else if(contentConflict){
                    printf("Error, content name is in use\n");
                    toClient.type = 'E';
                    strcpy(toClient.data, "Content name is in use\0");
                    sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                }
                else{
                    printf("Registration Conplete\n");
                    toClient.type = 'A';
                    strcpy(peerList[numberOfPeers].name, peerName);
                    strcpy(peerList[numberOfPeers].contentList[peerList[numberOfPeers].contentNumber], contentName);
                    strcpy(peerList[numberOfPeers].address, clientIP);
                    strcpy(peerList[numberOfPeers].ports, tempPort);
                    strcpy(toClient.data, "Content Registered\0");
                    sendto(UDP, (struct PDU *) &toClient, sizeof(toClient), 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                    peerList[numberOfPeers].contentNumber++;
                    numberOfPeers++;
                }
            }
            else{
                //Content match error
                if(contentConflict){
                    printf("Error, content name is in use\n");
                    toClient.type = 'E';
                    strcpy(toClient.data, "Content name is in use\0");
                    sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                }

                //Register next content item
                else{
                    printf("Registration Conplete\n");
                    strcpy(peerList[n].contentList[peerList[n].contentNumber], contentName);
                    toClient.type = 'A';
                    strcpy(toClient.data, "Content Registered\0");
                    sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                    peerList[n].contentNumber++;
                }
            }
        }
        
        //Search for download
        else if(recvData.type == 'S'){
            printf("File Search Request\n");
            bool foundContent = false;
            int toclientIPLen;
            char peerName[peerLength];
            char contentName[contentLength];
            
            //Get the Content Name
            for(i = 0; i < contentLength; i++){
                if(recvData.data[i] == '\0'){
                    break;
                }
                contentName[i] = recvData.data[i];
            }
            contentName[i] = '\0';

            //Search for if the content exists
            for(i = 0; i < numberOfPeers; i++){
                for(j = 0; j < peerList[i].contentNumber; j++){
                	printf("%s\n",peerList[i].contentList[j]);
                    if(strcmp(contentName, peerList[i].contentList[j]) == 0){
                        foundContent = true;
                        break;
                    }
                }
                if(foundContent){
                    break;
                }
            }

            //If the content exists, the client information is sent to the download client
            if(foundContent){
                printf("File found\n");
                toClient.type = 'S';
                toclientIPLen = strlen(peerList[i].ports) + 1;
                memcpy(toClient.data, peerList[i].ports, toclientIPLen);
                sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                bool hasFileRegistered = false;
                bool nameExist = false;
                int n;
                char peerName [peerLength];
                char contentName [contentLength];
                strcpy(peerName, peerList[i].name);
                strcat(peerName, "1");
                strcpy(contentName, peerList[i].contentList[j]);

                //Check for Peer exists
                for(n = 0; n < maxPeers; n++){
                    if(strcmp(peerName,peerList[n].name) == 0){
                        if(strcmp(clientIP, peerList[n].address) == 0){
                            hasFileRegistered = true;
                            break;
                        }
                        else{
                            nameExist = true;
                            break;
                        }
                    }
                }

                if(hasFileRegistered == false){
                    strcpy(peerList[numberOfPeers].name, peerName);
                    strcpy(peerList[numberOfPeers].contentList[peerList[numberOfPeers].contentNumber], contentName);
                    strcpy(peerList[numberOfPeers].address, clientIP);
                    numberOfPeers++;
                    peerList[numberOfPeers].contentNumber++;
                }
                else{
                    strcpy(peerList[n].contentList[peerList[n].contentNumber], contentName);
                    peerList[n].contentNumber++;
                }
            }
            else{
                toClient.type = 'E';
                strcpy(toClient.data, "File not found");
                sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
            }
        }
        
        //Deregistering Content
        else if(recvData.type == 'T'){
            bool foundContent = false;
            bool foundPeer = false;
            char contentName[contentLength];
            printf("De-Registering Content\n");
            char peerName[peerLength];
            
            //Get the Peer Name
            for(i = 0; i < peerLength; i++){
                if(recvData.data[i] == ' '){
                    break;
                }
                peerName[i] = recvData.data[i];
            }
            peerName[i] = '\0';
            
            //Get the Content Name
            j = 0;
            for(i = peerLength; i < peerLength + contentLength; i++){
                if(recvData.data[i] == ' '){
                    break;
                }
                contentName[j] = recvData.data[i];
                j++;
            }
            contentName[j] = '\0';

            //Check for the peer name
            for(n = 0; n < maxPeers; n++){
                if(strcmp(peerName,peerList[n].name) == 0){
                    foundPeer = true;
                    break;
                }
            }

            //Check for content name
            if(foundPeer){
                for(j = 0; j < contentLength; j++){
                    if(strcmp(contentName, peerList[n].contentList[j])){
                        foundContent = true;
                        break;
                    }
                }
            }

            //Content is found and ready to be deleted
            if(foundContent = false){
                toClient.type = 'E';
                strcpy(toClient.data, "Error Content not found");
                sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
            }
            else{
                toClient.type = 'A';
                strcpy(toClient.data, "Content Deleted\0");
                sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                if(j == contentLength -1){
                    memset(peerList[i].contentList[j], '\0', sizeof(peerList[i].contentList[j]));
                }
                else{
                    for(k = j; k < contentLength - 1; k++){
                        strcpy(peerList[i].contentList[k], peerList[i].contentList[k + 1]);
                    }
                    memset(peerList[i].contentList[k + 1], '\0', sizeof(peerList[i].contentList[k]));
                    peerList[i].contentNumber--;
                }
            }

            //Delete the Peer if there are no registered contents
            if(peerList[i].contentNumber < 1){
                if(i == peerLength - 1){
                    memset(peerList[i].contentList, '\0', sizeof(peerList[i].contentList));
                    memset(peerList[i].address, '\0', sizeof(peerList[i].address));
                    peerList[i].contentNumber= 0;
                }
                else{
                    for(k = i; k < peerLength - 1; k++){
                        peerList[k] = peerList[k + 1];
                    }
                    memset(peerList[k + 1].contentList, '\0', sizeof(peerList[k + 1].contentList));
                    memset(peerList[k + 1].address, '\0', sizeof(peerList[k + 1].address));
                    peerList[k + 1].contentNumber= 0;
                }
                numberOfPeers--;
            }
        }

        //List of Content
        else if(recvData.type = 'O'){
            printf("Listing Content\n");
              toClient.type = 'O';

            for(i = 0; i < numberOfPeers; i++){
                for(j = 0; j < peerList[i].contentNumber; j++){
                    memcpy(toClient.data, peerList[i].contentList[j], sizeof(peerList[i].contentList[j]) + 1);
                    sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
                }
            }
            toClient.type = 'F';
            strcpy(toClient.data, "List Finished\n");
            sendto(UDP, (struct PDU *) &toClient, sizeof(toClient) + 1, 0, (struct sockaddr *) &clientADDR, sizeof(clientADDR));
        }
    }
}
