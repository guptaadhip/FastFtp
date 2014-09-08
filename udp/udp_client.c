#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define ACK_MSG "I got your message"
#define MSG_PRINT "Here is your message: %s\n"
#define MSG_LISTEN "Listening on Port No: %d\n"
#define LENGTH 512

void printUsage() {
    fprintf(stdout, "UDP based Echo Client. Usage:\n");
    fprintf(stdout, "client [hostname] [portNo]\n\n");
    fprintf(stdout, "portNo: Integer range 1-65535 (Prefered: 2000 - 65535)\n");
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

/* Message handler */
void fileHandler(int socketFd, struct sockaddr_in serv_addr, socklen_t serv_len) {
    char revbuf[LENGTH];
    char f_name[15] = "receive.txt";
    
    FILE *fp = fopen(f_name, "a");
    if(fp == NULL) {
        fprintf(stderr, "File %s cannot be opened.\n", f_name);
        fclose(fp);
        exit(1);
    } else {
        bzero(revbuf, LENGTH);
        int f_block_sz = 0;
        int success = 0;
        
        int count = 1;
        while(success == 0) {
            while((f_block_sz = recvfrom(socketFd, revbuf, LENGTH, 0, (struct sockaddr *)&serv_addr, &serv_len)))
            {
                fprintf(stdout, "Reading chunk number = %d\n", count);
                count++;
                if(f_block_sz < 0) {
                    fprintf(stderr, "Receive file error.\n");
                    fclose(fp);
                    exit(1);
                }
                if (strcmp(revbuf,"End") == 0) {
                    fprintf(stdout,"File Transfer completed\n");
                    success = 1;
                    break;
                }
                int write_sz = fwrite(revbuf, sizeof(char), f_block_sz, fp);
                if(write_sz < f_block_sz) {
                    fprintf(stderr, "File write failed.\n");
                    fclose(fp);
                    exit(1);
                }
                bzero(revbuf, LENGTH);
            }
            fprintf(stderr, "ok!\n");
            success = 1;
            fprintf(stdout, "Yes! Received data from server\n");
            fclose(fp);
        }
    }
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[] = "hi";
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    socklen_t serv_len = sizeof(serv_addr);
    
    n = sendto(sockfd, buffer, 2, 0,(struct sockaddr *)&serv_addr, serv_len);
    if (n < 0) error("sendto");
    
    fileHandler(sockfd, serv_addr, serv_len);
    close(sockfd);
    return 0;
}
