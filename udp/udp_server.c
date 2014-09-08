/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define ACK_MSG "I got your message"
#define MSG_PRINT "Here is your message: %s\n"
#define MSG_LISTEN "Listening on Port No: %d\n"
#define LENGTH 512 // Buffer length
char f_name[] = "dgram_client.c";

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void fileSend(int socketFd, struct sockaddr_in cli_addr,socklen_t clilen) {
    clock_t begin, end;
    double time_spent;
    long int fileSize = 0;
    char sdbuf[LENGTH]; // Send buffer
    FILE *fp = fopen(f_name, "r");
    if(fp == NULL) {
        fprintf(stdout, "ERROR: File %s not found.\n", f_name);
        close(socketFd);
        exit(1);
    }
    bzero(sdbuf, LENGTH);
    int f_block_sz;
    clilen = sizeof(cli_addr);
    
    int count = 1;
    begin = clock();
    while((f_block_sz = fread(sdbuf, sizeof(char), LENGTH, fp)) > 0)
    {
        fprintf(stdout, "Sending chunk number = %d of size = %ld\n", count, fileSize);
        fileSize += f_block_sz;
        if(sendto(socketFd, sdbuf, f_block_sz, 0, (struct sockaddr *)&cli_addr, clilen) < 0) {
            fprintf(stdout, "ERROR: Failed to send file %s.\n", f_name);
            close(socketFd);
            exit(1);
        }
        f_block_sz = 0;
        bzero(sdbuf, LENGTH);
        count++;
    }
    sprintf(sdbuf, "End");
    int rc = sendto(socketFd, sdbuf, 10, 0, (struct sockaddr *)&cli_addr, clilen);
    printf("return val: %d\n", rc);
    
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    fprintf(stdout,"Time taken: %f Size: %ld Speed: %f\n", time_spent, fileSize, (float) fileSize/(time_spent * 1048576));
}

int main(int argc, char *argv[])
{
    socklen_t clilen;
     int n;
     char buf[1024];
    
     int sockfd, portno;
     struct sockaddr_in serv_addr, cli_addr;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
    
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

     clilen = sizeof(cli_addr);
     int count = 0;
     bzero(buf, 1024);
     n = recvfrom(sockfd,buf,1024,0,(struct sockaddr *)&cli_addr,&clilen);
     if (n < 0)
        error("recvfrom");
         
     fprintf(stdout, "Here is the message: %s\n",buf);
     
     fileSend(sockfd, cli_addr, clilen);
     return 0; 
}
