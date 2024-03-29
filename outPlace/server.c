/* Server side module for TCP command connection */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netdb.h>

#define SPLITS 4

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

int main(int argc, char *argv[]) {
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  char recv_udp_port[sizeof(int) + 1];
  struct sockaddr_in serv_addr, cli_addr;
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
	n = read(newsockfd,recv_udp_port,sizeof(int));
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

	/* UDP socket */
	socklen_t clientSocketLength;
	struct hostent *clientName;
	char receiveBuffer[splitSize];
	int serverSocketFD;
	struct sockaddr_in clientAddress;
	int recvfromID;
	
	clientName = gethostbyaddr(inet_ntoa(cli_addr.sin_addr),sizeof(struct in_addr),AF_INET);
	bzero((char *) &clientAddress, sizeof(clientAddress));
	
	serverSocketFD = socket(AF_INET, SOCK_DGRAM, 0);
	
	clientAddress.sin_family = AF_INET;
	bcopy((char *) clientName->h_addr, (char *)&clientAddress.sin_addr.s_addr, clientName->h_length);
	clientAddress.sin_addr.s_addr = cli_addr.sin_addr.s_addr;
	clientAddress.sin_port = htons(atoi(recv_udp_port));
	
	bzero(receiveBuffer, splitSize);
	socklen_t clientAddressLength = sizeof(clientAddress);
	clientSocketLength = sizeof(struct sockaddr_in);
	
	printf("\n %d \n", (int) sizeof(splits[0]));
	
	if(sendto(serverSocketFD,splits[0],splitSize,0,(struct sockaddr *) &clientAddress,clientAddressLength) < 0){
		fprintf(stderr,"Sending Error!");
		exit(1);
	}
	
	recvfromID = recvfrom(serverSocketFD,receiveBuffer,splitSize,0,(struct sockaddr *)&clientAddress,&clientSocketLength);
	
	if(recvfromID < 0){
		fprintf(stderr,"Receiving Error!");
		exit(1);
	}
	
	printf("Received - %s\n",receiveBuffer);
	
	// closing sockets
  close(newsockfd);
  close(sockfd);
	close(serverSocketFD);
  return 0; 
}
