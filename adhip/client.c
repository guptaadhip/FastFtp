/* This is the TCP client socket */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h> 
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>

#define TCP_PORT_NO 7007
#define UDP_PORT_NO 9000
#define SPLITS 4
#define DGRAM_SIZE 1450

#define PATH "data.bin"

int threadCounter[SPLITS];
long int fileSize = 0;
char *file;
/* Server address */
struct sockaddr_in tcpServerAddr;
uint32_t splitSize = 0;

/* Send file as UDP */
void sendUdp(int idx) {
  char msg[DGRAM_SIZE + 8];
  usleep(50000);
  struct sockaddr_in udpServerAddress;
  int udpSocket;
  socklen_t serverAddrLen;
  long int sendPtr = 0;
  long int sendSize = DGRAM_SIZE;
  long int exactStart = idx * splitSize;
  int rc = 0;
  long int seqNum = 0;
  long int sendSizeN = 0;
  
  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket < 0) {
    fprintf(stderr, "Error: Creating Socket\n");
    exit(1);
  }

  /* lets copy the address */
  udpServerAddress = tcpServerAddr;
  udpServerAddress.sin_port = htons(UDP_PORT_NO + idx);
  
  serverAddrLen = sizeof(udpServerAddress);
  while (sendPtr < splitSize) {
    if ((splitSize - sendPtr) < DGRAM_SIZE) {
      sendSize = splitSize - sendPtr;
    } else {
      sendSize = DGRAM_SIZE;
    }
    /* fill the structure */
    seqNum = htonl(sendPtr);
    memcpy(msg, &seqNum, sizeof(seqNum));
    sendSizeN = htonl(sendSize);
    memcpy((msg+4), &sendSizeN, sizeof(sendSizeN));
    memcpy((msg + 8), (file + exactStart + sendPtr), sendSize);
    rc = sendto(udpSocket, &msg, sizeof(msg), 0,
              (struct sockaddr *)&udpServerAddress, serverAddrLen);
    if (rc < 0) {
      fprintf(stderr, "Error: Sending Data\n");
      exit(1);
    }
    sendPtr += sendSize;
  }
}

/* send UDP file */
void *udp(void *argc) {
  int idx = *((int *) argc);
  sendUdp(idx);
  return 0;
}

void fileBuffer() {
  int fd;
  struct stat stats;
  
  if ((fd = open(PATH, O_RDONLY)) < 0) {
    printf("Error Opening the file\n");
    exit(0);
  }
  if (fstat (fd, &stats) < 0) {
    printf("Error in getting file size\n");
    exit(0);
  }
  fileSize = stats.st_size;
  if ((file = mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*) -1) {
    printf("Error caching the file\n");
    exit(0);
  }
  splitSize = (fileSize / SPLITS) + 1;
}

/* main */
int main(int argc, char *argv[]) {
  int rc, i;
  int tcpSocket;
  struct hostent *serverInfo;
  pthread_t thread[SPLITS];

  /* buffer the file */
  fileBuffer();
  
  /* get server details */
  serverInfo = gethostbyname(argv[1]);
  if (serverInfo == NULL) {
    printf("Failed to get the server for the hostname\n");
  }

  /* Creating the Internet domain socket */
  tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (tcpSocket < 0) {
    printf("Socket creation failed\n");
  }

  /* Set server address values */
  tcpServerAddr.sin_family = AF_INET;
  bcopy((char *) serverInfo->h_addr,(char *) &tcpServerAddr.sin_addr.s_addr,
                                                      serverInfo->h_length);
  tcpServerAddr.sin_port = htons(TCP_PORT_NO);
  if (connect(tcpSocket, (struct sockaddr *) &tcpServerAddr,
                                              sizeof(tcpServerAddr)) < 0) {
    printf("Conneting to server failed\n");
  }
  
  rc = write(tcpSocket, &fileSize, sizeof(fileSize));
  if (rc < 0) {
    fprintf(stderr, "ERROR writing to socket\n");
    exit(1);
  }
  
  for(i = 0; i < SPLITS; i++) {
    threadCounter[i]=i;
    pthread_create(&thread[i], NULL, udp, (void *) &threadCounter[i]);
  }
  
  for(i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }
  
  close(tcpSocket);
  return 0; 
}
