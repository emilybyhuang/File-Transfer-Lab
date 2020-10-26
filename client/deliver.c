#include "/Users/emilyhuang/Documents/ECE361/Lab3/packet.h"

int packToString( struct packet* p, char buffer[] ) {
    int offset = sprintf(buffer, "%d:%d:%d:%s:", p->total_frag, p->frag_no, p->size, p->filename);
    memcpy( buffer + offset, p->filedata, p->size);
    return offset+p->size;  //return the length of string packet
}

long int findsize(char file_name[]){
        FILE *fp= fopen(file_name, "r"); //r = reading; if wanna read and right, then r+
        if(fp==NULL) return -1;
        fseek(fp, 0L, SEEK_END);
        long int size = ftell(fp);
        fclose(fp);
        return size;
}

int main(int argc, char *argv[]){
    int sockfd, portNum;
    char ftpStr[] = "ftp";
    char yesBuf[MAXBUFLEN - 1] = "yes";
    char ackBuf[MAXBUFLEN - 1] = "ack";
    struct addrinfo servAddr;
    struct addrinfo *servAddrPtr;
    bool noValidFilename = true;
    char usrInput[1000]={'\0'},ftp[4] = {'\0'}, charPortNum[5] = {'\0'},fileName[100] = {'\0'},firstThreeChar[4] = {'\0'};

    //expecting ./deliver <server addr> to run the program
    if (argc != 2){
        printf("Error: the number of inputs is incorrect!\n");
        printf("Run based on format: deliver ug202.eecg.utoronto.ca\n");
        exit(EXIT_FAILURE);
    }


//1. Setup: getting the port num, server address, creating a socket... 
    //expecting <portnum> ftp <filename>
    while(noValidFilename){
        printf("Enter: <port number> ftp <filename>: \n");
        scanf("%d %s %s", &portNum, ftp, fileName);
        //get rid of trailing \n 
        if (fileName[strlen(fileName) - 1] == '\n')fileName[strlen(fileName) - 1] = '\0';
        if (access(fileName, F_OK) == 0)noValidFilename = false;
    }
    
    // printf("portnum : %d\n", portNum);
    // printf("ftp : %s\n", ftp);
    // printf("fn : %s\n", fileName);
    
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.ai_family = AF_INET;
    servAddr.ai_flags = AI_PASSIVE;
    servAddr.ai_socktype = SOCK_DGRAM;
    servAddr.ai_protocol = IPPROTO_UDP;

    //setup structures in the addr of servAddr;
    sprintf(charPortNum, "%d", portNum);    //convert since need it in char* because getaddrinfo needs it in char *
    int structureSetupSuccess = getaddrinfo(argv[1], charPortNum, &servAddr, &servAddrPtr);
    if (structureSetupSuccess < 0){
        printf("Structure setup failed!\n");
        exit(EXIT_FAILURE);
    }
    
    sockfd = socket(servAddrPtr->ai_family, servAddrPtr->ai_socktype, servAddrPtr->ai_protocol);
    if (sockfd < 1){
        printf("Bad socket. Exit");
        exit(EXIT_FAILURE);
    }

    //rtt: time there and back: basically time to sendto and recvfrom
    clock_t timeBeforeSend = clock();

 //2. Send ftp to server
    int numBytes = sendto(sockfd, (const void *)ftpStr, strlen(ftpStr), 0, (struct sockaddr *)(servAddrPtr->ai_addr), servAddrPtr->ai_addrlen);
    if (numBytes < 0){
        perror("Client failed sending ftp!\n");
        exit(EXIT_FAILURE);
    }

    socklen_t addrLen = sizeof(servAddrPtr->ai_addr);

//3. Expecting yes from server
    int receiveBytes1 = recvfrom(sockfd, yesBuf, MAXBUFLEN - 1, 0, (struct sockaddr *)(servAddrPtr->ai_addr), &addrLen);
    if (receiveBytes1 < 0){
        printf("receive fail!\n");
        exit(EXIT_FAILURE);
    }

    clock_t timeAfterRecv = clock() - timeBeforeSend;
    float rtt = (float)timeAfterRecv / CLOCKS_PER_SEC;
    printf("Rtt: %f\n", rtt);

    yesBuf[receiveBytes1] = '\0';
    if(yesBuf[strlen(yesBuf)-1] == '\n'){
        yesBuf[strlen(yesBuf)-1] = '\0';
    }

    char yes[] = "yes";
    if(strcmp(yes, yesBuf) != 0)exit(EXIT_FAILURE);
    printf("A file transfer can start.\n");

//4. File transfer, using the file with the filename enterred
    int sizeOfFile = findsize(fileName);
    int totalFrag = sizeOfFile / MAX_FILEDATA_SIZE + 1; //Round up to nearest integer size/1000
    char strPacket[MAX_PACKET_SIZE];    //string for the struct packet to send
    FILE *fp = fopen(fileName, "r"); //r = reading
    if (fp == NULL){
        printf("File not found!");
        exit(EXIT_FAILURE);
    }


    for (int i = 1; i <= totalFrag; i++){

        //a. make the packet(int structure)
        struct packet temp;
        temp.total_frag = totalFrag;
        temp.frag_no = i; 
        temp.filename = fileName;
        temp.filedata = (char *)malloc(MAX_FILEDATA_SIZE * sizeof(char));
        temp.size = fread((void *)temp.filedata, sizeof(char), MAX_FILEDATA_SIZE, fp);

        
        //b. convert the packet just made into string
        int nToSend = packToString( &temp, strPacket);

        //c. send the string packet to server
        int numByteRec = sendto(sockfd, (const void *)strPacket, nToSend, 0, (struct sockaddr *)(servAddrPtr->ai_addr), servAddrPtr->ai_addrlen);
        if (numByteRec == -1){
            printf("In sendPacket: FAILED");
            i--;//to resend this packet
            continue; //to keep sending
        }

        printf("Sending packet: %d\n", i);
        //initiate ACK timer
        clock_t ackStartTime = clock();

        //d. listen for ack from server
        while (true){
            int receiveBytes2 = recvfrom(sockfd, ackBuf, MAXBUFLEN - 1, 0, (struct sockaddr *)(servAddrPtr->ai_addr), &addrLen);
            if (receiveBytes2 < 0) continue;    //to keep recv from   

            //recording time waiting so far
            clock_t roundTrip = clock() - ackStartTime;
            float roundTripFloat = roundTrip / CLOCKS_PER_SEC;

            //check for timeout
            if (roundTripFloat >= 2 * rtt){
                //resend current packet
                printf("Timeout! Resending packet %d\n", i);
                i--;//so can that we can resend this packet, using i as index to the packet
                break;
            }

            //once we are sure there is no timeout, check for ack, if ack, don't need to wait for another ack
            strncpy(firstThreeChar, ackBuf, 3);
            if(strcmp(firstThreeChar, "ack")==0){
                printf("%s\n", ackBuf);
                fseek(fp, MAX_FILEDATA_SIZE, (i-1)*MAX_FILEDATA_SIZE);  
                break;
            }  
        }
    }
    freeaddrinfo(servAddrPtr);
    close(sockfd);
    return 0;      
}
