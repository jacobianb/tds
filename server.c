#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define BUF_LEN 1024


void set_buf(char *buf, char *str)
{
    int i;
    for (i = 0; i < BUF_LEN; i++) {
        buf[i] = str[i];
    }
    
}
int main(){
    int udpSocket, nBytes;
    char buf[BUF_LEN];
    struct sockaddr_in serverAddr, clientAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size, client_addr_size;
    int i;

    /*Create UDP socket*/
    udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

    /*Configure settings in address struct*/
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  

    /*Bind socket with address struct*/
    bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    /*Initialize size variable to be used later on*/
    addr_size = sizeof serverStorage;

    int data_rate = 0;
    while(1){
        /* Try to receive any incoming UDP datagram. Address and port of 
        requesting client will be stored on serverStorage variable */
        nBytes = recvfrom(udpSocket,buf,BUF_LEN,0,(struct sockaddr *)&serverStorage, &addr_size);
        printf("RECEIVED!\n");
        switch(buf[0]) {
        case 'a': /* requesting server data */
            printf("requesting data\n");
            set_buf(buf, "10x20:30");
            printf("BUFFER: %s\n", buf);
            break;
        case 'b': /* setting data rate */
            sscanf(&buf[1], "%d", &data_rate);
            set_buf(buf, "data rate change accepted.");
            printf("BUFFER: %s\n", buf);
            printf("desired data rate: %d\n", data_rate);
            break;
        }
        nBytes = strlen(buf);
        /*Send uppercase message back to client, using serverStorage as the address*/
        sendto(udpSocket,buf,nBytes,0,(struct sockaddr *)&serverStorage,addr_size);
    }

    return 0;
    }