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

#define SPLITS 4
#define DGRAM_SIZE 512
long int splitSize = 0;
char f_name[50];
char *splits[SPLITS];

void bufferFile() {
  long int fileSize = 0;
  int i = 0;
  size_t size = 0;
  FILE *stream;
  stream = fopen(f_name, "rb");
  fseek(stream, 0L, SEEK_END);
  fileSize = ftell(stream);
  fseek(stream, 0L, SEEK_SET);
  splitSize = fileSize / SPLITS;
  for(i = 0; i < SPLITS; i++) {
    splits[i] = malloc(splitSize+1);
    size=fread(splits[i],1,splitSize,stream);
    splits[i][size]=0;
  }
  fclose(stream);
}

void startUdp(int chunk, int portNo, struct sockaddr_in addr) {
  int sendPtr = 0;
  int udpSocket;
  int sendSize = DGRAM_SIZE;
  char seqNum[10];
  char buf[65535];
  socklen_t cliLen;
  struct sockaddr_in cliAddr;
  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    fprintf(stderr, "Error: Creating UDP Socket");
    exit(1);
  }
  bzero((char *) &cliAddr, sizeof(cliAddr));
  cliAddr.sin_family = AF_INET;
  inet_aton(inet_ntoa(addr.sin_addr), &cliAddr.sin_addr);
  cliAddr.sin_port = htons(portNo);
  cliLen = sizeof(cliAddr);
  while (sendPtr <= splitSize) {
    /* send only the amount left */
    if ((splitSize - sendPtr) < DGRAM_SIZE) {
      sendSize = splitSize - sendPtr;
    } else {
      sendSize = DGRAM_SIZE;
    }
    /* get the sequence number at the start */
    /* Format: <seqNum><space><data> */
    sprintf(seqNum, "%d", sendPtr);
    memset(buf, '\0', 65535);
    memcpy(buf, seqNum, strlen(seqNum));
    memset((buf + strlen(seqNum)), ' ', 1);
    memcpy((buf + strlen(seqNum) + 1), (splits[0] + sendPtr), (sendSize - strlen(seqNum) - 1));

    if(sendto(udpSocket, buf, (strlen(seqNum) + sendSize), 0, 
             (struct sockaddr *)&cliAddr, cliLen) < 0) {
      fprintf(stderr, "Error: Sending Data");
      exit(1);
    }
    sendPtr += DGRAM_SIZE;
  }
  close(udpSocket);
}


int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  int udpPort;
  char recv_udp_port[sizeof(int) + 1];
  struct sockaddr_in serv_addr, cli_addr;
  int n;
	
  /* timing */
  clock_t begin, end;
  double time_spent;
  begin = clock();

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
  bzero((char *) &cli_addr, sizeof(cli_addr));	 
   
  portno = atoi(argv[1]);
     
	// Set server address values and bind the socket 
	serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "ERROR on binding");
  }

  listen(sockfd,1);    
	clilen = sizeof(cli_addr);
	 
	// accept connection and open socket
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0) {
    fprintf(stderr, "ERROR on accept");
  }
		
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
  /* to be done: Handle the timing in such a way that the 
   * sleep is not required */
  sleep(1);
  startUdp(0, udpPort, cli_addr);
  /* timing */
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time taken: %f\n", time_spent);

	// closing sockets
  close(newsockfd);
  close(sockfd);
  return 0; 
}
