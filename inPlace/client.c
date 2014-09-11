#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define UDP_PORT 7865
#define DGRAM_SIZE 512

char *splits[4];
long int splitLength = 0;

void startUdp() {
  int seqNum = 0;
  int readPtr = 0;
  char buffer[65535];
  char *temp;
  char seqStr[10];
  struct sockaddr_in cliAddr, serAddr; 
  socklen_t cliAddrLen;
  int socketFd, rc;
  socketFd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socketFd < 0) {
    fprintf(stderr, "Error: Creating Socket");
    exit(1);
  }
  bzero((char *) &serAddr, sizeof(serAddr));
  serAddr.sin_family = AF_INET;
  serAddr.sin_addr.s_addr = INADDR_ANY;
  serAddr.sin_port = htons(UDP_PORT);
  if (bind(socketFd, (struct sockaddr *) &serAddr, sizeof(serAddr)) < 0) {
    fprintf(stderr, "Error: Binding to Socket");
    exit(1);
  }
  cliAddrLen = sizeof(cliAddr);
  bzero(splits[0], splitLength);
  while (readPtr < splitLength) {
    bzero(buffer, sizeof(buffer));
    rc = recvfrom(socketFd, buffer, DGRAM_SIZE, 0, 
          (struct sockaddr *) &cliAddr, &cliAddrLen);
    if (rc < 0) {
      fprintf(stderr, "Error: Receiving Data");
      exit(1);
    }
    memset(seqStr, '\0', sizeof(seqStr));
    temp = strchr(buffer, ' '); 
    memcpy(seqStr, buffer, (temp - buffer));
    seqNum = atoi(seqStr);
    readPtr += rc;
    /* copy the data to the right place */
    if (seqNum == 0) {
      memcpy((splits[0]+seqNum-1), (buffer+strlen(seqStr)), (rc-strlen(seqStr)));
    } else {
      memcpy((splits[0]+seqNum-2), (buffer+strlen(seqStr) + 1), (rc-strlen(seqStr)));
    }
    printf("Data Read: %d, Total Data: %d seq: %d\n", rc, readPtr, seqNum);
  }

  close(socketFd);
  printf("Split:<start>%s<end>\n", splits[0]);
  exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[200];
  char udp_port[6];
	
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
  splitLength = atol((buffer+2));
  splits[0] = malloc(splitLength+1);
  /* creating the udp */
  /* this needs to be forked */
  startUdp();
  close(sockfd);
  return 0;
}
