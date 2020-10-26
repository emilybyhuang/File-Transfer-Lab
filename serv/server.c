/*
You should implement a server program, called “server.c” in C on a UNIX system. Its
execution command should have the following structure:

server <UDP listen port>

Upon execution, the server should:
1. Open a UDP socket and listen at the specified port number
2. Receive a message from the client
a. if the message is “ftp”, reply with a message “yes” to the client.
b. else, reply with a message “no” to the client.
*/


#include "/Users/emilyhuang/Documents/ECE361/Lab3/packet.h"


//returns the index of the indexOfChar-th charToSearch in findIndex
int findIndex(char * str, char charToSearch, int indexOfChar){
    int numFound = 0;
    for(int i = 0; i < strlen(str); i++){
        if(str[i] == charToSearch) numFound++;
        if(numFound == indexOfChar)return i;
    }
    return -1; //if never found
}

int digits(int n){
    int numDigits;
    while (n != 0) {
        n /= 10;     // n = n/10
        ++numDigits;
    }
    return numDigits;
}


int main(int argc, char *argv[]){
    char yesMsg[] = "yes";
    char noMsg[] = "no";
    char *portNum;
    struct addrinfo servAddr;
    struct addrinfo* servAddrPtr;
    struct sockaddr_storage cliAddr;
    char buf[MAXBUFLEN-1];
    char ftpStr[] = "ftp";
    int sockfd;

    //expecting server <port num>
    if(argc != 2){
        printf("Error: the number of inputs is incorrect!\n");
        exit(EXIT_FAILURE);
    }

//1. Setup: make the socket and bind
    portNum = argv[1];

    //reset sin_zero (for padding the structure to the length of a struct sockaddr) in socketAddr to 0 by using memset
    //set first num bytes of the block of memory pointed by ptr to the specified value(which is 0)
    memset(&servAddr, 0, sizeof(servAddr));
    //memset(&cliAddr, 0, sizeof(cliAddr));
    
    servAddr.ai_family = AF_INET;       //IPv4
    servAddr.ai_socktype = SOCK_DGRAM;
    servAddr.ai_flags = AI_PASSIVE;
    servAddr.ai_protocol = IPPROTO_UDP; 
    //allocate linked list(fills the structs needed later) using getaddrinfo
    int structureSetupSuccess = getaddrinfo(NULL, portNum, &servAddr, &servAddrPtr);

    if(structureSetupSuccess < 0 ) {
        printf("Structure setup failed!\n");
        exit(EXIT_FAILURE);
    }

    //create unbound socket in domain
    //servAddrPtr->ai_family: which is IPv4
    //servAddrPtr->ai_socktype is datagram for UDP
    //protocol is 0: default protocol
    //sockfd: used to access the socket later
    sockfd = socket(servAddrPtr -> ai_family, servAddrPtr->ai_socktype, servAddrPtr->ai_protocol);
    if(sockfd < 0){
        printf("Socket failed!\n");
        exit(EXIT_FAILURE);
    }

    //assign address to unbound socket just made, unbound socket CAN'T receive anything
    //assigns the address in the 2nd parameter(servAddrPtr -> ai_addr) to the socket in the 1st parameter 
    //which is the socket's file descriptor
    //3rd parameter specifies the size, in bytes, of the address structure pointed to by addr
    int bindRet = bind(sockfd, (struct sockaddr * )servAddrPtr -> ai_addr,servAddrPtr -> ai_addrlen);
    if(bindRet < 0){
        printf("Bind failed!\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addrLen = sizeof(cliAddr);

//2. Expecting ftp from client
    //call to receive a message from a socket
    //returns length of input message upon completion
    //if no message: it'll wait
    int recvBytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)&(cliAddr), &addrLen);
    //int recvBytes = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, (struct sockaddr *)(cliAddrPtr -> ai_addr), &addrLen);
    if(recvBytes < 0){
        perror("Error receiving!\n");
        exit(EXIT_FAILURE);
    }
    buf[recvBytes] = '\0';

    //get rid of the newline enterred
    if(buf[strlen(buf)-1] == '\n')buf[strlen(buf)-1] = '\0';


//3. If ftp from client, send yes, else send no
    //expecting ftp from client. If ftp received, can start file transfer
    int strCmpRes = strcmp(ftpStr, buf);
    if(strCmpRes == 0){
        int sendBytes = sendto(sockfd, (const void *)yesMsg, strlen(yesMsg), 0, (struct sockaddr *)&(cliAddr), addrLen);
        // printf("Sendbytes: %d\n", sendBytes);
        // printf("Sent!\n");
    }else{
        int sendBytes = sendto(sockfd, (const void *)noMsg, strlen(noMsg), 0, (struct sockaddr *)&(cliAddr), addrLen);
        // printf("Sendbytes: %d\n", sendBytes);
        // printf("Sent to!!\n");
    }

//4. File transfer, using the info provided by client
    FILE *fileptr;
    bool start = true, firstTrans = true;
    int numPackets = 0;
    char comingtext[MAX_PACKET_SIZE];
    int packetStrLen;
    while(start){
        //a. (comment c. in deliver)receive a packet from client
        if((packetStrLen = recvfrom(sockfd, comingtext, MAX_PACKET_SIZE, 0, (struct sockaddr *)&(cliAddr), &addrLen)) < 0){
            printf("Client message error\n");
            continue;   //want to receive another packet from client
        }
        //b. convert from the string packet received into struct packet 
        struct packet p;
        char filename[MAX_PACKET_SIZE];
        sscanf(comingtext, "%d:%d:%d:%s:", &(p.total_frag), &(p.frag_no), &(p.size), filename);
        p.filename = strtok(filename, ":" ); 

        //populating filedata sent by client
        int indexOfColon = findIndex(comingtext, ':', 4)+1; 

        //make sure to malloc first or else can't memcpy
        p.filedata = malloc(MAX_FILEDATA_SIZE * sizeof(char));
        memcpy( p.filedata,comingtext+indexOfColon, p.size);  //data starts after 4th ':' from the left
        
        //keep track of num of packets received
        numPackets++;

        //c. create ack msg and send to client
        char numberStr[100],ack[10] = "ack: ";
        sprintf(numberStr, "%d", numPackets);
        printf("Acking packet with num: %s\n", numberStr);
        strcat(ack, numberStr);
        strcat(ack, "!");
        printf("ack msg: %s\n", ack);
        sendto(sockfd, (const void *)ack , strlen( ack ), 0, (struct sockaddr *)&(cliAddr), addrLen);

        //d. write the info received into the filename given in the packet
        //assuming no buffer needed for this lab: write to file as longs as if it's in order
        if(p.frag_no == numPackets){
            //first packet: open file, don't need to do it for future packets till close
            if(firstTrans){
                fileptr = fopen(p.filename, "w");//write mode
                firstTrans = false;//don't open again after opened
            }
            //write accordingly
            //p -> filedata : array to write to
            //sizeof(char) : size of element
            //p->size : num of elements
            //fileptr : output stream(where to write)
            fwrite(p.filedata, sizeof(char), p.size, fileptr);

            if(p.frag_no == p.total_frag){
                printf("Got last packet!\n");
                start = false;
                //exit this while loop: don't receive anymore                
            }
        }
    }
    fclose(fileptr);
    printf("Done!\n");
    close(sockfd);
    return (EXIT_SUCCESS);
}