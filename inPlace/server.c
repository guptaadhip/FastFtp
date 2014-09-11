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

#include <sys/time.h>

#define SPLITS 4
#define DGRAM_SIZE 65500
long int splitSize = 0;
char f_name[50];
char *splits[SPLITS];
int udpPort;

void bufferFile() {
  long int fileSize = 0;
  int i = 0;
  size_t size = 0;
  FILE *stream;
  stream = fopen(f_name, "rb");
  fseek(stream, 0L, SEEK_END);
  fileSize = ftell(stream);
	printf("fileSize-%ld\n",fileSize);
  fseek(stream, 0L, SEEK_SET);
  splitSize = fileSize / SPLITS + 1;
  for(i = 0; i < SPLITS; i++) {
    splits[i] = malloc(splitSize+1);
    size=fread(splits[i],1,splitSize,stream);
    splits[i][size]=0;
  }
  fclose(stream);
}

void startUdp(int chunk, struct sockaddr_in addr) {
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
  cliAddr.sin_port = htons(udpPort+chunk);
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
    memcpy((buf + strlen(seqNum) + 1), (splits[chunk] + sendPtr - strlen(seqNum)+ 1), (sendSize - strlen(seqNum)));
    if(sendto(udpSocket, buf, (strlen(seqNum) + sendSize - 1), 0, 
             (struct sockaddr *)&cliAddr, cliLen) < 0) {
      fprintf(stderr, "Error: Sending Data");
      exit(1);
    }
    sendPtr += DGRAM_SIZE;
  }
	//exit(0);
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
	 
	// accept connection and open TCP socket
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
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
  sleep(1);
		
	/* Inserting code to spawn multiple processes */
	int pcounter = 0; /* Counter to spawn new process */
	pid_t childId = 0;
	int status = -1;
	
	/* Number of processes to be spawned = Number of splits */
	while(pcounter < SPLITS)
	{
		childId = fork();
		if(childId == 0)
		{
			/* timing */
			/*clock_t begin, end;
			//time_t begin, end;
			double time_spent;
			//time(&begin);
			begin = clock();*/
 
			 
		  //fprintf(stdout, "Child Process-%d started \n",pcounter+1);
			/* assuming 1 process takes care of sending one split(chunk) 
			* So chunk number argument is same as process counter arg 
			*/
			startUdp(pcounter,cli_addr);
			/*time(&end);
			time_spent = difftime(end, begin);
			fprintf(stdout,"Time taken-%d: %f\n", pcounter+1,time_spent);
			fprintf(stdout, "Child Process-%d exiting \n",pcounter+1);*/
			//sleep(4);
			/*end = clock();
			time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
			fprintf(stdout,"Time taken-%d: %f\n", pcounter+1,time_spent);*/
			exit(0);
		}
		else if(childId < 0){
			fprintf(stderr, "ERROR: Child Process Creation Failed");
		}
		else
		{
			fprintf(stdout, "Parent Process continuing \n");
			pcounter++;
		}
	}
	
	/* Parent processes waits for child processes to terminate 
	Signal Handling left !! 
	*/
	while (wait(&status) != -1)
    ;
  //startUdp(0, udpPort, cli_addr);
  /* timing */
  gettimeofday(&t1, 0); 
	long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
	fprintf(stdout,"Time taken-%d: %ld\n", pcounter+1, elapsed);
			

	// closing sockets
  close(newsockfd);
  close(sockfd);
  return 0; 
}
