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
#include <unistd.h>
#include <set>

#define TCP_PORT_NO 8011
#define UDP_PORT_NO 10000
#define UDP_RECV_PORT_NO 11000
#define SPLITS 4
#define DGRAM_SIZE 1450
#define OUT_FILE "recv.bin"

using namespace std;

uint64_t fileSize = 0;
uint64_t splitSize[SPLITS];
int threadCounter[SPLITS];
char *file;

/* what is it running as 0: receiver, 1: sender */
int receiver = 0;
int doneReceiveCount = 0;

int sendingDone = 0;

/* TCP Command Connection Socket */
int tcpSocket;
/* tcp socket Mutex */
pthread_mutex_t tcpSocketLock;
pthread_mutex_t udpResendLock[SPLITS];

struct sockaddr_in tcpClientAddr;

/* socket option */
int yes = 1;

uint64_t missedDataPtr = 0;
uint64_t totalPackets[SPLITS];
uint64_t totalRecvSize = 0;
int resendNACK[SPLITS];

std::set<uint64_t> missedSeqSender[SPLITS];
std::set<uint64_t> missedSeqReceiver[SPLITS];

struct sendNACKData{
	uint32_t idx;
	uint64_t seqNum;
};

/* write the file to disk */
void writeToDisk() {
  ofstream outfile (OUT_FILE,std::ofstream::binary);
  outfile.write(file, totalRecvSize);
}

/* SENDER CODE */

void nackHandler() {
  /* lets just print them for now */
  int idx;
  int rc;
  uint64_t seqNum;
  //char msg[10];
	struct sendNACKData values;
	unsigned char buffer[sizeof(values)];
	
  while (1) {
		rc=0;
		rc = recv(tcpSocket, buffer, sizeof(buffer), 0);
    
    if (rc < 0) {
      cout << "ERROR reading from socket1" << endl;
      exit(1);
    }
		values = *(struct sendNACKData *) &buffer;
		if(values.idx < SPLITS){
			idx = ntohl(values.idx);
			seqNum = ntohl(values.seqNum);
		}else{
			idx = ntohl(values.seqNum);
			seqNum = ntohl(values.idx);
		}
    
		if (idx > SPLITS) {
			sendingDone = 1;
			cout << "Got last value closing" << endl;
			//close(tcpSocket);
			return;
		}
		cout << idx << "  " << seqNum << endl;
		if(idx < SPLITS){
			if(seqNum  < fileSize){
				pthread_mutex_lock(&udpResendLock[idx]);
				//if(missedSeqSender[idx].find(seqNum) != missedSeqSender[idx].end()) continue;
        missedSeqSender[idx].insert(seqNum);
				pthread_mutex_unlock(&udpResendLock[idx]);
			}
		}	
  }
}

/* Send file as UDP */
void sendUdp(int idx) {
  char msg[DGRAM_SIZE + 8];
  struct sockaddr_in udpServerAddress;
  int udpSocket;
  socklen_t serverAddrLen;
  uint64_t sendPtr = 0;
  int sendSize = DGRAM_SIZE;
  uint64_t exactStart = 0;
  int rc = 0;
  uint64_t seqNum = 0;
  uint64_t sendSizeN = 0;
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
  udpServerAddress = tcpClientAddr;
  udpServerAddress.sin_port = htons(UDP_RECV_PORT_NO + idx);
  
  serverAddrLen = sizeof(udpServerAddress);
  while (sendPtr < splitSize[idx]) {
    if ((splitSize[idx] - sendPtr) < DGRAM_SIZE) {
      sendSize = splitSize[idx] - sendPtr;
    } else {
      sendSize = DGRAM_SIZE;
    }
    /* fill the structure */
    seqNum = htonl(sendPtr);
    memcpy(msg, &seqNum, sizeof(seqNum));
    sendSizeN = htonl(sendSize);
    memcpy((msg + 4), &sendSizeN, sizeof(sendSizeN));
    memcpy((msg + 8), (file + exactStart + sendPtr), sendSize);
    rc = sendto(udpSocket, &msg, sizeof(msg), 0,
                (struct sockaddr *)&udpServerAddress, serverAddrLen);
    if (rc < 0) {
      cout << "Error: Sending Data" << endl;
      exit(1);
    }
    sendPtr += sendSize;
  }
	
	while(sendingDone != 1){
		while(!missedSeqSender[idx].empty()){
			sendPtr = *(missedSeqSender[idx].begin());
      if ((splitSize[idx] - sendPtr) < DGRAM_SIZE) {
				sendSize = splitSize[idx] - sendPtr;
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
        cout << "Error: Sending Data" << endl;
        exit(1);
			}
			pthread_mutex_lock(&udpResendLock[idx]);
			missedSeqSender[idx].erase(missedSeqSender[idx].begin());
			pthread_mutex_unlock(&udpResendLock[idx]);
		}
	}
}

/* RECEIVER CODE */

void sendNack(int idx, uint64_t seqNum) {
  int rc;
	struct sendNACKData values;
	unsigned char buffer[sizeof(values)];
	values.idx=ntohl(idx);
	values.seqNum=ntohl(seqNum);
	memcpy(&buffer, &values, sizeof(values));
  rc = send(tcpSocket, buffer, sizeof(buffer), 0);
  if (rc < 0) {
    cout << "ERROR reading from socket" << endl;
    exit(1);
  }
}

/* send NACK file */
void *continousNACK(void *argc) {
	int idx;
	while(receiver == 0){
		//sleep(1);
		for (idx = 0; idx < SPLITS; idx++) {
			if(!missedSeqReceiver[idx].empty()){
				std::set<uint64_t>::iterator seqIt;
				for (seqIt = missedSeqReceiver[idx].begin(); seqIt != missedSeqReceiver[idx].end(); ++seqIt) {
					//if( *(seqIt) > (fileSize - DGRAM_SIZE)) missedSeqReceiver[idx].erase(*(seqIt));
					pthread_mutex_lock(&tcpSocketLock);
					cout << (uint64_t) *(seqIt) << endl;
					sendNack(idx, *(seqIt));
					pthread_mutex_unlock(&tcpSocketLock);
				}
			} 
		}
	}
	return 0;
}

void receiveUdp(int idx) {
  uint64_t seqNum = 0;
  char buffer[DGRAM_SIZE + 8];
  struct sockaddr_in clientAddr, serverAddr;
  socklen_t clientAddrLen;
  int rc;
  int udpSocket;
  uint64_t recvSize = 0;
  uint64_t exactLocation = 0;
  uint64_t recvDataSize = 0;
 /* NACK */
  uint64_t expectedSeqNum = 0;
  int i = 0;
 	
  for (i = 0; i < idx; i++) {
    exactLocation += splitSize[idx];
  }

  udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSocket < 0) {
    cout << "Error: Creating Socket" << endl;
    exit(1);
  }

  clientAddr.sin_family = AF_INET;
  clientAddr.sin_addr.s_addr = INADDR_ANY;
  clientAddr.sin_port = htons(UDP_PORT_NO + idx);
  
  if (bind(udpSocket, (struct sockaddr *) &clientAddr, sizeof(clientAddr)) < 0) {
    cout << "Error: Binding to Socket " <<  ntohs(clientAddr.sin_port) << endl;
    exit(1);
  }
	clientAddrLen = sizeof(serverAddr);
  
  while (recvSize < splitSize[idx]) {
    bzero((char *) buffer, sizeof(buffer));
    rc = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                  (struct sockaddr *) &serverAddr, &clientAddrLen);
    if (rc < 0) {
      cout << "Error: Receiving Data" << endl;
      exit(1);
    }
    totalPackets[idx]++;
    memcpy(&seqNum, buffer, sizeof(uint64_t));
    seqNum = ntohl(seqNum);
    memcpy(&recvDataSize, (buffer + 4), sizeof(uint64_t));
    recvDataSize = ntohl(recvDataSize);
    recvSize += recvDataSize;
    
		if (expectedSeqNum == seqNum) {
			expectedSeqNum += recvDataSize;
			missedSeqReceiver[idx].erase(expectedSeqNum);
			memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize);
    }else if(expectedSeqNum > seqNum) {
			memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize);
			missedSeqReceiver[idx].erase(seqNum);
		}else{
			while (expectedSeqNum <= seqNum) {
				pthread_mutex_lock(&tcpSocketLock);
					missedSeqReceiver[idx].insert(expectedSeqNum);
					sendNack(idx, expectedSeqNum);
					resendNACK[idx]=0;
					expectedSeqNum += DGRAM_SIZE;
				pthread_mutex_unlock(&tcpSocketLock);
			}
			memcpy((file + exactLocation + seqNum), (buffer + 8), recvDataSize);
			missedSeqReceiver[idx].erase(seqNum);
			expectedSeqNum += recvDataSize;
		}
  }
  totalRecvSize += recvSize;
}

/* Orchestrator */

void *udp(void *argc) {
  int idx = *((int *) argc);
  /* get the split size */
  while (fileSize == 0);
  if (idx == (SPLITS - 1)) {
    splitSize[idx] = (fileSize - ((uint64_t) (fileSize / SPLITS) * (SPLITS - 1)));
  } else {
    splitSize[idx] = fileSize / SPLITS;
  }
  receiveUdp(idx);
  doneReceiveCount++;
  while (doneReceiveCount != SPLITS) {
    continue;
  }
  usleep(20000);
  sendUdp(idx);
  return 0;
}

int main(int argc, char *argv[]) {
  int rc, i;
  int waitSocket;
  uint64_t size;
  struct sockaddr_in tcpAddr;
  socklen_t clientLen;
  pthread_t thread[SPLITS];
	pthread_t contNACK;
  
  /* receiver variable should be set */
  receiver = 1;
	
	if (pthread_mutex_init(&tcpSocketLock, NULL) != 0){
		cout << "mutex init failed" << endl;
		exit(0); 
	}
  
  for(i = 0; i < SPLITS; i++) {
		if (pthread_mutex_init(&udpResendLock[i], NULL) != 0) {
			cout << "mutex init failed" << endl;
			exit(0);
		}
	}
  
	// Creating the Internet domain socket
  waitSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (waitSocket < 0) {
    cout << "ERROR opening socket" << endl;
    close(waitSocket);
    exit(1);
  }
  
  if (setsockopt(waitSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    cout << "Error: Setting Socket Options" << endl;
    exit(1);
  }
	// Set server address values and bind the socket
	tcpAddr.sin_family = AF_INET;
  tcpAddr.sin_addr.s_addr = INADDR_ANY;
  tcpAddr.sin_port = htons(TCP_PORT_NO);
  if (bind(waitSocket, (struct sockaddr *) &tcpAddr, sizeof(tcpAddr)) < 0) {
    cout << "ERROR on binding" << endl;
    close(waitSocket);
    exit(1);
  }
  
  listen(waitSocket,1);
	clientLen = sizeof(tcpClientAddr);
  /* Lets do the udp server also */
  for(i = 0; i < SPLITS; i++) {
    threadCounter[i]=i;
    pthread_create(&thread[i], NULL, udp, (void *) &threadCounter[i]);
  }
 
	pthread_create(&contNACK, NULL, continousNACK, NULL);
	
	// accept connection and open TCP socket
  tcpSocket = accept(waitSocket, (struct sockaddr *) &tcpClientAddr, &clientLen);
  if (tcpSocket < 0) {
    cout << "ERROR on accept" << endl;
    close(tcpSocket);
    close(waitSocket);
    exit(1);
  }

  struct timeval connectionTimeout;
  connectionTimeout.tv_sec = 300;       /* Timeout in seconds */
  setsockopt(tcpSocket, SOL_SOCKET, SO_SNDTIMEO,
             (struct timeval *) &connectionTimeout, sizeof(struct timeval));
  setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO,
             (struct timeval *) &connectionTimeout, sizeof(struct timeval));
  
  close(waitSocket);

  rc = recv(tcpSocket, &size, sizeof(size), 0);
  if (rc < 0) {
    cout << "ERROR reading from socket" << endl;
    close(tcpSocket);
    exit(1);
  }
  file = (char *) malloc(size + 1);
  bzero((char *) file, size);
  fileSize = size;
  
  /* when all the threads are done receiving lets become sender */
  while (doneReceiveCount != SPLITS) {
    continue;
  }
  writeToDisk();
  receiver = 0;
  sendNack(SPLITS+1, SPLITS+1);
  cout << "Receiving done, Turning sides" << endl;
  
  nackHandler();
  
  while(sendingDone != 1) {
    continue;
  }
  
  for(i = 0; i < SPLITS; i++) {
    pthread_join(thread[i], NULL);
  }
  
  close(waitSocket);
  close(tcpSocket);
  return 0;
}
