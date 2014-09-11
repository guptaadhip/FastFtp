/* Server side module for TCP command connection */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <pthread.h>

#include <sys/time.h>

#define SPLITS 4
#define DGRAM_SIZE 65500

/* UDP packet Bundle */
struct udpPacketData{
    int     sequenceNo;
		size_t  bufferLength;
    char    buffer[DGRAM_SIZE];
};

long int splitSize = 0;
char f_name[50];
char *splits[SPLITS];
int udpPort;
struct sockaddr_in addr;

void bufferFile() {
  long int fileSize = 0;
  int i = 0;
  size_t size = 0;
  FILE *stream;
  stream = fopen(f_name, "rb");
  fseek(stream, 0L, SEEK_END);
  fileSize = ftell(stream);
	//printf("fileSize-%ld\n",fileSize);
  fseek(stream, 0L, SEEK_SET);
  splitSize = fileSize / SPLITS + 1;
  for(i = 0; i < SPLITS; i++) {
    splits[i] = malloc(splitSize+1);
    size=fread(splits[i],1,splitSize,stream);
    splits[i][size]=0;
  }
  fclose(stream);
}

void *startUdp(void *temp) {
  int sendPtr = 0;
  int chunk = *((int *) temp);
  int udpSocket;
  int sendSize = DGRAM_SIZE;
  char buf[DGRAM_SIZE];
  socklen_t cliLen;
  struct sockaddr_in cliAddr;
  
	struct udpPacketData udpPacket;
	
	bcopy((char *) &addr, (char *) &cliAddr, sizeof(addr));
  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  
	if (udpSocket < 0) {
    fprintf(stderr, "Error: Creating UDP Socket");
    exit(1);
  }
	
  bzero((char *) &cliAddr, sizeof(cliAddr));
  cliAddr.sin_family = AF_INET;
  inet_aton(inet_ntoa(addr.sin_addr), &cliAddr.sin_addr);
  cliAddr.sin_port = htons(udpPort+chunk);
  cliLen = sizeof(cliAddr);
	
	udpPacket.sequenceNo = 0;
	
	while(sendPtr <= splitSize){
		/* send only the amount left */
    if ((splitSize - sendPtr) < DGRAM_SIZE) {
      sendSize = splitSize - sendPtr;
    } else {
      sendSize = DGRAM_SIZE;
    }
		
		memcpy(udpPacket.buffer, splits[chunk],sendSize);
    udpPacket.bufferLength = sizeof(splits[chunk]);
    udpPacket.sequenceNo++;
		
		if(sendto(udpSocket, &udpPacket, sizeof(udpPacket), 0, 
             (struct sockaddr *)&cliAddr, cliLen) < 0) {
      fprintf(stderr, "Error: Sending Data");
      exit(1);
    }
		
		sendPtr += sendSize;
	}
	return 0;
}


int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno, i;
  socklen_t clilen;
  pthread_t thread[SPLITS];
  char recv_udp_port[sizeof(int) + 1];
  struct sockaddr_in serv_addr;
  int n;
	
	// Checking for valid inputs
  if (argc < 3) {
    fprintf(stderr, "ERROR: no port provided");
    exit(1);
  }

  /* Split and buffer the shit */
  strcpy(f_name, argv[2]);
  bufferFile();

	// Creating the Internet domain socket 
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "ERROR opening socket");
    exit(1);
  }
	
  // zeroing the address structures 	
  bzero((char *) &serv_addr, sizeof(serv_addr));
  bzero((char *) &addr, sizeof(addr));	 
   
  portno = atoi(argv[1]);
     
	// Set server address values and bind the socket 
	serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR on binding");
  }

  listen(sockfd,1);    
	clilen = sizeof(addr);
	 
	// accept connection and open TCP socket
  newsockfd = accept(sockfd, (struct sockaddr *) &addr, &clilen);
  if (newsockfd < 0) {
    fprintf(stderr, "ERROR on accept");
  }

	struct timeval t0,t1;			
	gettimeofday(&t0, 0);

  // reading Receiver's UDP port number
  bzero(recv_udp_port,sizeof(int) + 1);
	n = read(newsockfd,recv_udp_port, sizeof(recv_udp_port));
  if (n < 0) {
    fprintf(stderr, "ERROR reading from socket");
    exit(1);
  }
  fprintf(stdout, "Here is Receiver's UDP port no. : %s\n",recv_udp_port);
     
	// sending file transfer info to server
	char info[200];
	sprintf(info, "%d %ld", SPLITS, splitSize); 
	n = write(newsockfd, info, sizeof(info));
  if (n < 0) {
    fprintf(stderr, "ERROR writing to socket");
    exit(1);
  }
  udpPort = atoi(recv_udp_port);
  /* lets send the first chunk */
  /* TBD: Handle the timing in such a way that the 
   * sleep is not required */
  usleep(500000);
	
  /* threading to handle things */
  for (i = 0; i < SPLITS; i++) {
    pthread_create(&thread[i], NULL, startUdp, &i);
	}

  for (i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }

  /* timing */
  gettimeofday(&t1, 0); 
	long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
	fprintf(stdout,"Time taken: %ld\n", elapsed);
			

	// closing sockets
  close(newsockfd);
  close(sockfd);
  return 0; 
}
