#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>

#define TCP_PORT_NO 7007
#define UDP_PORT_NO 9000
#define SPLITS 4
#define DGRAM_SIZE 1450

long int fileSize = 0;
long int splitSize = 0;
int threadCounter[SPLITS];
char *file;

/* Receive UDP */
void receiveUdp(int idx) {
  long int seqNum = 0;
  char buffer[DGRAM_SIZE + 8];
  struct sockaddr_in clientAddr, serverAddr;
  socklen_t clientAddrLen;
  int udpSocket, rc;
  long int recvSize = 0;
  int yes = 1;
  long int exactLocation = idx * splitSize;
  long int recvDataSize = 0;
  /* NACK */
  long int expectedSeqNum = 0;

  
  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    fprintf(stderr, "Error: Creating Socket\n");
    exit(1);
  }
  
  if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    fprintf(stderr, "Error: Setting Socket Options\n");
    exit(1);
  }

  /*long int n = 1024 * 9000; //experiment with it
  if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVBUFFORCE, &n, sizeof(n)) == -1) {
    fprintf(stderr, "Error: Setting Socket Options\n");
    exit(1);
  }*/

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(UDP_PORT_NO + idx);
  fprintf(stdout, "Listening on Port: %d\n", ntohs(serverAddr.sin_port));
  fflush(stdout);
  
  if (bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    fprintf(stderr, "Error: Binding to Socket %d\n", ntohs(serverAddr.sin_port));
    exit(1);
  }
	clientAddrLen = sizeof(clientAddr);
  

  while (expectedSeqNum < splitSize) {
    bzero((char *) buffer, sizeof(buffer));
    rc = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                  (struct sockaddr *) &clientAddr, &clientAddrLen);
    if (rc < 0) {
      fprintf(stderr, "Error: Receiving Data\n");
      exit(1);
    }
    memcpy(&seqNum, buffer, sizeof(long int));
    seqNum = ntohl(seqNum);
    memcpy(&recvDataSize, (buffer + 4), sizeof(long int));
    recvDataSize = ntohl(recvDataSize);
    recvSize += recvDataSize;
    //printf("Got seqNum: %ld, size: %d\n", seqNum, rc);
    if (expectedSeqNum < seqNum) {
      //printf("got: %ld expect seq: %ld\n", seqNum, expectedSeqNum);
      while (expectedSeqNum != seqNum) {
        expectedSeqNum += recvDataSize;
      }
    } else if (expectedSeqNum > seqNum) {
      //printf("Out of order seq: %ld\n", seqNum);
      /* got a packet out of order need to handle */
    }
    expectedSeqNum += recvDataSize;
    memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize); 
    //printf("Data Read: %d, Total Received Size: %ld SeqNum: %ld\n", rc, recvSize, seqNum);
  }
}

/* Start the UDP Servers */
void *udp(void *argc) {
  int idx = *((int *) argc);
  while (fileSize == 0);
  /* sanity to do the file size Split */
  splitSize = fileSize / SPLITS + 1;
  struct timeval t0,t1;
  gettimeofday(&t0, 0);
  receiveUdp(idx);
  /* lets do the needful now */
  gettimeofday(&t1, 0);
  long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
  printf("Time taken: %ld microseconds\n", elapsed);
  //printf("Server Idx: %d\n", idx);
  /* wait for the tcp command to be received */
  return 0;
}

int main(int argc, char *argv[]) {
  int rc, i;
  int waitSocket, tcpSocket;
  long int size;
  struct sockaddr_in tcpAddr;
  struct sockaddr_in tcpClientAddr;
  socklen_t clientLen;
  pthread_t thread[SPLITS];
	
	// Creating the Internet domain socket
  waitSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (waitSocket < 0) {
    fprintf(stderr, "ERROR opening socket\n");
    exit(1);
  }
  
	// Set server address values and bind the socket
	tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_addr.s_addr = INADDR_ANY;
  tcpAddr.sin_port = htons(TCP_PORT_NO);
  if (bind(waitSocket, (struct sockaddr *) &tcpAddr, sizeof(tcpAddr)) < 0) {
    fprintf(stderr, "ERROR on binding\n");
  }
  
  listen(waitSocket,1);
	clientLen = sizeof(tcpClientAddr);
  /* Lets do the udp server also */
  for(i = 0; i < SPLITS; i++) {
    threadCounter[i]=i;
    pthread_create(&thread[i], NULL, udp, (void *) &threadCounter[i]);
  }
  
	// accept connection and open TCP socket
  tcpSocket = accept(waitSocket, (struct sockaddr *) &tcpClientAddr, &clientLen);
  if (tcpSocket < 0) {
    fprintf(stderr, "ERROR on accept\n");
  }

  rc = read(tcpSocket, &size, sizeof(size));
  if (rc < 0) {
    fprintf(stderr, "ERROR reading from socket\n");
    exit(1);
  }
  file = malloc(size + 1);
  fileSize = size;
  //printf("Got File Size: %ld\n", fileSize);
  
  splitSize = fileSize / SPLITS;
  
  for(i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }
  //printf("\nGot Data:%s\n", file); 
  return 0;
}
