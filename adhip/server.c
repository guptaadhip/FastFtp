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
#define DGRAM_SIZE 1470

long int fileSize = 0;
long int splitSize = 0;
int threadCounter[SPLITS];

/* Receive UDP */
void receiveUdp(int idx) {
  long int seqNum = 0;
  char buffer[DGRAM_SIZE + 4];
  struct sockaddr_in clientAddr, serverAddr;
  socklen_t clientAddrLen;
  int udpSocket, rc;
  long int recvSize = 0;
  int yes = 1;
  
  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    fprintf(stderr, "Error: Creating Socket\n");
    exit(1);
  }
  
  if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    fprintf(stderr, "Error: Setting Socket Options\n");
    exit(1);
  }

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(UDP_PORT_NO + idx);
  
  if (bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    fprintf(stderr, "Error: Binding to Socket %d\n", ntohs(serverAddr.sin_port));
    exit(1);
  }
	clientAddrLen = sizeof(clientAddr);
  while (seqNum <= (splitSize - DGRAM_SIZE)) {
    rc = recvfrom(udpSocket, &buffer, sizeof(buffer), 0,
                  (struct sockaddr *) &clientAddr, &clientAddrLen);
		if (rc < 0){
			fprintf(stderr, "Error: Receiving Data\n");
			exit(1);
		}
    memcpy(&seqNum, buffer, sizeof(long int));
    seqNum = ntohl(seqNum);
    recvSize += rc;
    //printf("Data Read: %d, Total Received Size: %ld SeqNum: %ld\n", rc, recvSize, seqNum);
  }
}

/* Start the UDP Servers */
void *udp(void *argc) {
  int idx = *((int *) argc);
  printf("Server Idx: %d\n", idx);
  /* wait for the tcp command to be received */
  while (fileSize == 0);
  struct timeval t0,t1;
	gettimeofday(&t0, 0);
  /* lets do the needful now */
  receiveUdp(idx);
  gettimeofday(&t1, 0);
	long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
	printf("Time taken: %ld microseconds\n", elapsed);
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
  fileSize = size;
  printf("Got File Size: %ld\n", fileSize);
  
  splitSize = fileSize / SPLITS;
  
  for(i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }
  return 0;
}
