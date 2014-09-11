#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>

#define UDP_PORT 7865
#define DGRAM_SIZE 65535
#define SPLITS 4

char *splits[SPLITS];	/* number of splits */
long int splitLength = 0;
int numSplits = 0;  		/* number of splits */

void startUdp(int pcounter) {
  long int seqNum = 0;
  long int readPtr = 0;
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
  serAddr.sin_port = htons(UDP_PORT+pcounter);
  if (bind(socketFd, (struct sockaddr *) &serAddr, sizeof(serAddr)) < 0) {
    fprintf(stderr, "Error: Binding to Socket");
    exit(1);
  }
  cliAddrLen = sizeof(cliAddr);
  bzero(splits[pcounter], splitLength);
  /* hack in while to make it quit */
  while ((seqNum + rc) < splitLength) {
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
    readPtr -= strlen(seqStr);
    /* copy the data to the right place */
    if (seqNum == 0) {
      memcpy((splits[pcounter]+seqNum-1), (buffer+strlen(seqStr)), (rc-strlen(seqStr)));
    } else {
      memcpy((splits[pcounter]+seqNum-2), (buffer+strlen(seqStr) + 1), (rc-strlen(seqStr)));
    }
    printf("Data Read: %d, Total Data: %ld seq: %ld Len: %ld\n", rc, readPtr, seqNum, splitLength);
  }

  close(socketFd);
	printf("%d\n", pcounter);
  printf("Got the entire file!! Yay!!!\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[200];
  char udp_port[6];
	int icounter = 0 ;
	
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
  /* creating the udp */
  /* this needs to be forked */

	/* Inserting code for forking processes */
	int pcounter = 0; /* Counter to spawn new process */
	pid_t childId = 0;
	int status = -1;
	
	/* Number of processes to be spawned = Number of splits */
	while(pcounter < numSplits)
	{
		childId = fork();
		if(childId == 0)
		{
		  fprintf(stdout, "Child Process-%d started \n",pcounter+1);
			startUdp(pcounter);
			fprintf(stdout, "Child Process-%d exiting \n",pcounter+1);
		}
		else if(childId < 0){
			fprintf(stderr, "ERROR: Child Process Creation Failed");
		}
		else
		{
			fprintf(stdout, "Parent Process continuing \n");
			pcounter++;
			/* check where to handle UDP port number increase. Better handle it here and 
			pass the new UDP port number to child process (similar to server part)
			*/
			//udpPort++;    
		}
	}
	
	/* Parent processes waits for child processes to terminate 
	Signal Handling left !! 
	*/
	while (wait(&status) != -1);
  //startUdp();
  close(sockfd);
  return 0;
}
