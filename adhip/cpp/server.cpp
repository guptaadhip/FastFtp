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
#include <fstream>

#define TCP_PORT_NO 7007
#define UDP_PORT_NO 9000
#define SPLITS 4
#define DGRAM_SIZE 1450
#define OUT_FILE "recv.bin"


using namespace std;

long int fileSize = 0;
long int splitSize[SPLITS];
int threadCounter[SPLITS];
char *file;

/* NACK */
struct missedDataNack {
  int idx;
  long int missedSeq;
} missedData[65535];

int missedDataPtr = 0;
long int totalPackets[SPLITS];
long int totalRecvSize = 0;

/* write the file to disk */
void writeToDisk() {
  ofstream outfile (OUT_FILE,std::ofstream::binary);
  outfile.write(file, totalRecvSize);
}

/* Receive UDP */
void receiveUdp(int idx) {
  long int seqNum = 0;
  char buffer[DGRAM_SIZE + 8];
  struct sockaddr_in clientAddr, serverAddr;
  socklen_t clientAddrLen;
  int udpSocket, rc;
  long int recvSize = 0;
  int yes = 1;
  /* TBD: exactLocation */
  long int exactLocation = 0;
  long int recvDataSize = 0;
  /* NACK */
  long int expectedSeqNum = 0;
  int i = 0;
  /* lets do some time out */
  /*struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;*/
  
  for (i = 0; i < idx; i++) {
    exactLocation += splitSize[idx];
  }

  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    cout << "Error: Creating Socket" << endl;
    exit(1);
  }
  
  if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    cout << "Error: Setting Socket Options" << endl;
    exit(1);
  }

  /*if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    cout << "Error: Setting Socket Options\n");
    exit(1);
  }
  long int n = 1024 * 9000; //experiment with it
  if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVBUFFORCE, &n, sizeof(n)) == -1) {
    cout << "Error: Setting Socket Options\n");
    exit(1);
  }*/

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(UDP_PORT_NO + idx);
  //fprintf(stdout, "Listening on Port: %d\n", ntohs(serverAddr.sin_port));
  
  if (bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    cout << "Error: Binding to Socket " <<  ntohs(serverAddr.sin_port) << endl;
    exit(1);
  }
	clientAddrLen = sizeof(clientAddr);
  

  while (expectedSeqNum < splitSize[idx]) {
    bzero((char *) buffer, sizeof(buffer));
    rc = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                  (struct sockaddr *) &clientAddr, &clientAddrLen);
    if (rc < 0) {
      cout << "Error: Receiving Data" << endl;
      exit(1);
    }
    totalPackets[idx]++;
    memcpy(&seqNum, buffer, sizeof(long int));
    seqNum = ntohl(seqNum);
    memcpy(&recvDataSize, (buffer + 4), sizeof(long int));
    recvDataSize = ntohl(recvDataSize);
    recvSize += recvDataSize;
    //printf("Got seqNum: %ld, size: %d\n", seqNum, rc);
    if (expectedSeqNum < seqNum) {
      while (expectedSeqNum != seqNum) {
        /* need to get locking */
        missedData[missedDataPtr++].idx = idx;
        missedData[missedDataPtr++].missedSeq = expectedSeqNum;
        expectedSeqNum += recvDataSize;
      }
    } else if (expectedSeqNum > seqNum) {
      //printf("Out of order seq: %ld\n", seqNum);
      /* got a packet out of order need to handle */
    }
    expectedSeqNum += recvDataSize;
    memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize); 
    //printf("Data Read: %d, Total Received Size: %ld SeqNum: %ld\n", rc, recvSize, seqNum);
  }
  totalRecvSize += recvSize;
}

/* Start the UDP Servers */
void *udp(void *argc) {
  int idx = *((int *) argc);
  while (fileSize == 0);
  /* sanity to do the file size Split */
  if (idx == (SPLITS - 1)) {
    splitSize[idx] = (fileSize - ((long int) (fileSize / SPLITS) * (SPLITS - 1)));
  } else {
    splitSize[idx] = fileSize / SPLITS;
  }
  printf("%d: Split Size: %ld\n", idx, splitSize[idx]);
  struct timeval t0,t1;
  gettimeofday(&t0, 0);
  receiveUdp(idx);
  /* lets do the needful now */
  gettimeofday(&t1, 0);
  long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;
  cout << idx << "Time taken: " << elapsed << " microseconds" << endl;
  //printf("Server Idx: %d\n", idx);
  /* wait for the tcp command to be received */
  return 0;
}

int main(int argc, char *argv[]) {
  long int tot = 0;
  int rc, i;
  int waitSocket, tcpSocket;
  long int size;
  struct sockaddr_in tcpAddr;
  struct sockaddr_in tcpClientAddr;
  socklen_t clientLen;
  pthread_t thread[SPLITS];
	
	// Creating the Internet domain socket
  waitSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (waitSocket < 0) {
    cout << "ERROR opening socket" << endl;
    exit(1);
  }
  
	// Set server address values and bind the socket
	tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_addr.s_addr = INADDR_ANY;
  tcpAddr.sin_port = htons(TCP_PORT_NO);
  if (bind(waitSocket, (struct sockaddr *) &tcpAddr, sizeof(tcpAddr)) < 0) {
    cout << "ERROR on binding" << endl;
  }
  
  listen(waitSocket,1);
	clientLen = sizeof(tcpClientAddr);
  /* Lets do the udp server also */
  for(i = 0; i < SPLITS; i++) {
    threadCounter[i]=i;
    pthread_create(&thread[i], NULL, udp, (void *) &threadCounter[i]);
  }
  
	// accept connection and open TCP socket
  tcpSocket = accept(waitSocket, (struct sockaddr *) &tcpClientAddr, &clientLen);
  if (tcpSocket < 0) {
    cout << "ERROR on accept" << endl;
  }

  rc = recv(tcpSocket, &size, sizeof(size), 0);
  if (rc < 0) {
    cout << "ERROR reading from socket" << endl;
    exit(1);
  }
  file = (char *) malloc(size + 1);
  bzero((char *) file, size);
  fileSize = size;
  //printf("Got File Size: %ld\n", fileSize);
  
  for(i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }
  for (i = 0; i < SPLITS; i++) {
    tot += totalPackets[i];
  }
  cout << "Packets Received: " << tot << endl; 
  cout << "Missed Packets: " << missedDataPtr << endl; 
  writeToDisk();
  return 0;
}
