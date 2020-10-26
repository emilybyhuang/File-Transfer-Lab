
#include "global.h"
#define DATA_SIZE 1000
#define MAX_FILEDATA_SIZE 400
#define MAX_PACKET_SIZE 4096
#define MAXBUFLEN 1000

struct packet{
        unsigned int total_frag;
        unsigned int frag_no;
        unsigned int size;
        char* filename;
        char* filedata;
} packet;

//to debug
void printPacket ( struct packet p ) {
    printf("Total Fragments : %d\n", p.total_frag);
    printf("Fragment Number : %d\n", p.frag_no);
    printf("Size : %d\n", p.size);
    printf("Filename : %s\n", p.filename);  
    printf("Data : %s\n", p.filedata);
}