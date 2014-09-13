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

#include <set>
#include <fstream>

#define TCP_PORT_NO 7010
#define UDP_PORT_NO 9000
#define SPLITS 4
#define DGRAM_SIZE 1450

#define OUT_FILE "recv.bin"

using namespace std;

long int fileSize = 0;
long int splitSize[SPLITS];
int threadCounter[SPLITS];
char *file;
/* Purposely made global */
int tcpSocket;
struct sockaddr_in tcpClientAddr;
socklen_t clientLen;

pthread_mutex_t nackLock;

/* NACK */
struct missedDataNack {
  int idx;
  long int missedSeq;
};

/* Declaring and Initializing set */
std::set<missedDataNack> missedDataSet;
std::set<missedDataNack>::iterator it;

long int missedDataPtr = 0;
long int totalPackets[SPLITS];
long int totalRecvSize = 0;
char msg[8];


bool operator<(const missedDataNack& lhs, const missedDataNack& rhs) {
  return ((lhs.idx <= rhs.idx) || (lhs.missedSeq <= rhs.missedSeq));
}

/* write the file to disk */
void writeToDisk() {
  ofstream outfile (OUT_FILE,std::ofstream::binary);
  outfile.write(file, totalRecvSize);
}

/* Receive UDP */
void receiveUdp(int idx) {
	struct missedDataNack missedData;
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
  long int n = 1024 * 100; //experiment with it
  if (setsockopt(udpSocket, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) {
    cout << "Error: Setting Socket Options" << endl;
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
      while (expectedSeqNum < seqNum) {
        /* need to get Locking */
        pthread_mutex_lock(&nackLock);
        missedData.idx = idx;
        missedData.missedSeq = expectedSeqNum;
				missedDataSet.insert(missedData);
        expectedSeqNum += recvDataSize;
				missedDataPtr++;
        pthread_mutex_unlock(&nackLock);
      }
			expectedSeqNum += recvDataSize;
			memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize);
    } else if (expectedSeqNum == seqNum) {
			expectedSeqNum += recvDataSize;
			memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize);
    }else{ //OutofOrder
			/* got a packet out of order need to handle */
      pthread_mutex_lock(&nackLock);
			missedData.idx = idx;
      missedData.missedSeq = seqNum;
			missedDataSet.erase(missedData);
			missedDataPtr--;
      pthread_mutex_unlock(&nackLock);
			memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize); 
		}
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

void printMissedSet()
{
  std::cout << "Missed Data Set contains:";
  for (it = missedDataSet.begin(); it != missedDataSet.end(); ++it)
  {
    cout << "Index No." << (*it).idx << ' ' << (*it).missedSeq << endl;
  }
  std::cout << '\n';
}

void sendMissedSet()
{
  int rc = 0;
  int idxNetwork = 0;
	long int seqNumNetwork = 0;
	
  std::cout << "Sending Missed Data Set now:";
  for (it = missedDataSet.begin(); it != missedDataSet.end(); ++it)
  {
    idxNetwork = htons((*it).idx);
    memcpy(msg, &idxNetwork, sizeof(idxNetwork));
    seqNumNetwork = htonl((*it).missedSeq);
    memcpy((msg+2), &seqNumNetwork, sizeof(seqNumNetwork));
    
    rc = sendto(tcpSocket, &msg, sizeof(msg), 0, (struct sockaddr *)&tcpClientAddr, clientLen);
    if (rc < 0) {
      cout << "Error: Sending Data" << endl;
      exit(1);
    }

  }
  
  /* Sending ending message */
  rc = sendto(tcpSocket, "END", 3, 0, (struct sockaddr *)&tcpClientAddr, clientLen);
  std::cout << '\n';
}
int main(int argc, char *argv[]) {
  long int tot = 0;
  int rc, i;
  int waitSocket;
  long int size;
  struct sockaddr_in tcpAddr;
 // struct sockaddr_in tcpClientAddr;
  //socklen_t clientLen;
	// int tcpSocket;
  pthread_t thread[SPLITS];

  if (pthread_mutex_init(&nackLock, NULL) != 0) {
    cout << "mutex init failed" << endl;
    exit(0);
  }

	
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
	
	printMissedSet();
	sendMissedSet();
  for (i = 0; i < SPLITS; i++) {
    tot += totalPackets[i];
  }
  cout << "Packets Received: " << tot << endl; 
  cout << "Missed Packets: " << missedDataPtr << endl; 
  writeToDisk();
  return 0;
}
