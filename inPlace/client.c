#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>

#define UDP_PORT 7865
#define DGRAM_SIZE 65535
#define SPLITS 4

/* UDP packet Bundle */
struct udpPacketData{
    int     sequenceNo;
		size_t  bufferLength;
    char    buffer[DGRAM_SIZE];
};

char *splits[SPLITS];	/* number of splits */
long int splitLength = 0;
int numSplits = 0;  		/* number of splits */

/* Code for File writing*/
/*void writeToFile() {
    long int fileSize = 0;
    int i = 0;
    size_t size = 0;
    FILE *op;
    op = fopen("Received.txt", "wab");
    for(i = 0; i < SPLITS; i++) {
        size=fwrite(splits[i],1,splitLength,op);
    }
    fclose(op);
}*/

void *startUdp(void *tempX) {
  long int seqNum = 0;
  long int readPtr = 0;
  int yes = 1;
  int chunk = *((int *) tempX);
  char buffer[DGRAM_SIZE];
  char *temp;
  struct sockaddr_in cliAddr, serAddr; 
  socklen_t cliAddrLen;
  int socketFd, rc;
	int receivePtr =0;
	int lastSequence =0;
	
	struct udpPacketData udpPacket;
  
	socketFd = socket(AF_INET, SOCK_DGRAM, 0);
  
	if (socketFd < 0) {
    fprintf(stderr, "Error: Creating Socket");
    exit(1);
  }
  
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    perror("setsockopt");
    exit(1);
  }
	
  bzero((char *) &serAddr, sizeof(serAddr));
  serAddr.sin_family = AF_INET;
  serAddr.sin_addr.s_addr = INADDR_ANY;
  serAddr.sin_port = htons(UDP_PORT + chunk);
  
	if (bind(socketFd, (struct sockaddr *) &serAddr, sizeof(serAddr)) < 0) {
    fprintf(stderr, "Error: Binding to Socket %d", ntohs(serAddr.sin_port));
    exit(1);
  }
  
	cliAddrLen = sizeof(cliAddr);
  
	bzero(splits[chunk], splitLength);
  
	/* hack in while to make it quit */
	
	while(receivePtr <= splitLength){
		rc = recvfrom(socketFd, &udpPacket, sizeof(udpPacket), 0, 
				(struct sockaddr *) &cliAddr, &cliAddrLen);
		if (rc < 0){
			fprintf(stderr, "Error: Receiving Data");
			exit(1);
		}
		
		if(udpPacket.sequenceNo != (lastSequence+1)){ 
			// Change this code to handle re-transmission
			fprintf(stderr, "Error: Packet unordered or missing\n");
			continue;
		}	
		lastSequence=udpPacket.sequenceNo;
		
		receivePtr+=udpPacket.bufferLength;
		//Append the incoming packet in order
		memcpy((splits[chunk]+receivePtr), udpPacket.buffer, udpPacket.bufferLength);
		
		printf("Data Read: %d, Total Data: %ld seq: %ld Len: %ld\n", rc, receivePtr, udpPacket.sequenceNo, udpPacket.bufferLength);
	}
	
  close(socketFd);
  return 0;
  //printf("Got the entire file!! Yay!!!\n");
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[200];
  char udp_port[6];
	int icounter = 0;
  int i;
  pthread_t thread[SPLITS];
	
  if (argc < 3) {
    fprintf(stderr,"usage %s hostname port\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[2]);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) { 
    fprintf(stderr, "ERROR opening socket");
    exit(1);
  }
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr,"ERROR, no such host\n");
    exit(1);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
  serv_addr.sin_port = htons(portno);
    
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR connecting");
    exit(1);
  }
  bzero(&udp_port, sizeof(udp_port));
  sprintf(udp_port, "%ld", (long int) UDP_PORT);
  n = write(sockfd, udp_port,sizeof(udp_port));
  if (n < 0) { 
    fprintf(stderr, "ERROR writing to socket");
    exit(1);
  }

  bzero(buffer, 200);
  n = read(sockfd, buffer, 200);
  if (n < 0) {
    fprintf(stderr, "ERROR reading from socket");
    exit(1);
  } 
	/* check this buffer + 1, buffer + 2. 
	If you send the message as "4 10000" from server, 
	then buffer[0] = 4, buffer[1] = space, buffer[2] = 1, buffer[3] = 0, buffer[4] = 0 and 
	so on, as buffer is a simple character array.
	*/
	
  splitLength = atol((buffer+2));
	numSplits = atoi(&buffer[0]); // Not used, we will use global SPLITS value
	//printf("%ld %d\n",splitLength,numSplits);
	//exit(1);
	for(icounter=0; icounter < SPLITS;icounter++){
		splits[icounter] = malloc(splitLength+1);
	}

  for (i = 0; i < SPLITS; i++) { 
    pthread_create(&thread[i], NULL, startUdp, &i);
  }

  for (i = 0; i < SPLITS; i++) { 
    pthread_join(thread[i], NULL);
  }

  /* writing the data received to a file */
  //writeToFile();
  close(sockfd);
  return 0;
}
