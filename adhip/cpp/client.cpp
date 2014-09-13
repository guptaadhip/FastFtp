/* This is the TCP client socket */
#include<stdlib.h>
#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <cstdlib>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <cmath>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cctype>

#define TCP_PORT_NO 7009
#define UDP_PORT_NO 9000
#define SPLITS 4
#define DGRAM_SIZE 1450

using namespace std;

#define PATH "data.bin"

int threadCounter[SPLITS];
long int fileSize = 0;
char *file;
/* Server address */
struct sockaddr_in tcpServerAddr;
long int splitSize[SPLITS];

/* Send file as UDP */
void sendUdp(int idx) {
  char msg[DGRAM_SIZE + 8];
  struct sockaddr_in udpServerAddress;
  int udpSocket;
  socklen_t serverAddrLen;
  long int sendPtr = 0;
  int sendSize = DGRAM_SIZE;
  long int exactStart = 0;
  int rc = 0;
  long int seqNum = 0;
  long int sendSizeN = 0;
  int i = 0;
  
  for (i = 0; i < idx; i++) {
    exactStart += splitSize[idx];
  }

  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocket < 0) {
    cout << "Error: Creating Socket" << endl;
    exit(1);
  }

  /* lets copy the address */
  udpServerAddress = tcpServerAddr;
  udpServerAddress.sin_port = htons(UDP_PORT_NO + idx);
  
  serverAddrLen = sizeof(udpServerAddress);
  while (sendPtr < splitSize[idx]) {
    if ((splitSize[idx] - sendPtr) < DGRAM_SIZE) {
      sendSize = splitSize[idx] - sendPtr;
    } else {
      sendSize = DGRAM_SIZE;
    }
    //printf("Messages Send Size: %d\n", sendSize);
		cout << "Messages Send Size: " << sendSize;
    /* fill the structure */
    seqNum = htonl(sendPtr);
    memcpy(msg, &seqNum, sizeof(seqNum));
    sendSizeN = htonl(sendSize);
    memcpy((msg+4), &sendSizeN, sizeof(sendSizeN));
    memcpy((msg + 8), (file + exactStart + sendPtr), sendSize);
    rc = sendto(udpSocket, &msg, sizeof(msg), 0,
              (struct sockaddr *)&udpServerAddress, serverAddrLen);
    if (rc < 0) {
      cout << "Error: Sending Data" << endl;
      exit(1);
    }
    sendPtr += sendSize;
  }
}

/* send UDP file */
void *udp(void *argc) {
  int idx = *((int *) argc);
  /* get the split size */
  if (idx == (SPLITS - 1)) {
    splitSize[idx] = (fileSize - ((long int) (fileSize / SPLITS) * (SPLITS - 1)));
  } else {
    splitSize[idx] = fileSize / SPLITS;
  }
  sendUdp(idx);
  return 0;
}

void fileBuffer() {
  int fd;
  struct stat stats;
  
  if ((fd = open(PATH, O_RDONLY)) < 0) {
    cout << "Error Opening the file" << endl;
    exit(0);
  }
  if (fstat (fd, &stats) < 0) {
    cout << "Error in getting file size" << endl;
    exit(0);
  }
  fileSize = stats.st_size;
  if ((file = (char *) mmap(0, fileSize, PROT_READ, MAP_PRIVATE, fd, 0)) == (void*) -1) {
    cout << "Error caching the file" << endl;
    exit(0);
  }
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
    cout << "Failed to get the server for the hostname" << endl;
  }

  /* Creating the Internet domain socket */
  tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (tcpSocket < 0) {
    cout << "Socket creation failed" << endl;
  }

  /* Set server address values */
  tcpServerAddr.sin_family = AF_INET;
  bcopy((char *) serverInfo->h_addr,(char *) &tcpServerAddr.sin_addr.s_addr,
                                                      serverInfo->h_length);
  tcpServerAddr.sin_port = htons(TCP_PORT_NO);
  if (connect(tcpSocket, (struct sockaddr *) &tcpServerAddr,
                                              sizeof(tcpServerAddr)) < 0) {
    cout << "Conneting to server failed" << endl;
  }
  
  rc = send(tcpSocket, &fileSize, sizeof(fileSize), 0);
  if (rc < 0) {
    cout << "ERROR writing to socket" << endl;
    exit(1);
  }
  
  for(i = 0; i < SPLITS; i++) {
    threadCounter[i]=i;
    pthread_create(&thread[i], NULL, udp, (void *) &threadCounter[i]);
  }
  
  for(i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }
  
  //close(tcpSocket);
  return 0; 
}
