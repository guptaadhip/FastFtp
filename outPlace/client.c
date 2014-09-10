#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

long int splitSize = 0;
long int splits = 0;

int main(int argc, char *argv[]) {
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[200];
  char *udp_port;
	
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

  asprintf(&udp_port, "%d", 9007);
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
  
	splitSize = atol(buffer+2);
	splits = atol(buffer);
	
	fprintf(stdout, "%s\n", buffer);
	
	/* UDP socket */
	socklen_t serverSocketLength;
	struct hostent *clientName;
	char receiveBuffer[splitSize];
	int clientSocketFD;
	struct sockaddr_in clientAddress, serverAddress;
	int recvfromID;
	
	clientSocketFD = socket(AF_INET, SOCK_DGRAM, 0);
	
	bzero((char *) &clientAddress, sizeof(clientAddress));
	
	clientAddress.sin_family = AF_INET;
	clientAddress.sin_addr.s_addr = INADDR_ANY;
	clientAddress.sin_port = htons(atoi(udp_port));
	
	if (bind(clientSocketFD, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0){
		fprintf(stderr, "Sender Socket Bind Error!");
		exit(1);
	}
	
	serverSocketLength = sizeof(serverAddress);
	bzero(receiveBuffer, splitSize);
	
	recvfromID = recvfrom(clientSocketFD,receiveBuffer,splitSize,0,(struct sockaddr *)&serverAddress,&serverSocketLength);
	
	if(recvfromID < 0){
		fprintf(stderr, "Receiving Error!");
	}
	printf("%s\n",receiveBuffer);
	
	if(sendto(clientSocketFD, receiveBuffer, sizeof(receiveBuffer), 0, (struct sockaddr *)&serverAddress, serverSocketLength) < 0) {
		fprintf(stderr, "Send Error!");
	}
	
	close(clientSocketFD);

  close(sockfd);
  return 0;
}
