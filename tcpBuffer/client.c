/* This is the TCP client socket */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h> 
#include <ctype.h>
#include <sys/wait.h>

#define ACK_MSG "I got your message"
#define MSG_PRINT "Here is your message: %s\n"
#define MSG_LISTEN "Listening on Port No: %d\n"
#define LENGTH 2000

void printUsage() {
  fprintf(stdout, "TCP based Echo Client. Usage:\n");
  fprintf(stdout, "\tserver [hostname] [portNo]\n\n");
  fprintf(stdout, "portNo: Integer range 1-65535 (Prefered: 2000 - 65535)\n");
}

/* Error Function */
void errorHandler(char *msg, bool usage) {
  /* print the error */
  if (msg == NULL) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
  } else {
    fprintf(stderr, "Error: %s\n", msg);
  }
  
  if (usage) {
    fprintf(stdout, "\n");
    printUsage();
  }
  exit(0);
}

/* make sure the second argument is an number */
bool isNumber(char *str) {
  while (*str != '\0') {
    if (!isdigit(*str)) {
      return false;
    }
    str++;
  }
  return true;
}

/* Message handler */
void fileHandler(int socketFd) {
  char revbuf[LENGTH];
  char f_name[15] = "receive.txt";
  FILE *fp = fopen(f_name, "a");
  if(fp == NULL) {
    printf("File %s cannot be opened.\n", f_name);
    fclose(fp);
    exit(1);
  } else {
    bzero(revbuf, LENGTH);
    int f_block_sz = 0;
    int success = 0;
    while(success == 0) {
      while((f_block_sz = recv(socketFd, revbuf, LENGTH, 0))) {
        if(f_block_sz < 0) {
          printf("Receive file error.\n");
          fclose(fp);
          exit(1);
        }
        int write_sz = fwrite(revbuf, sizeof(char), f_block_sz, fp);
        if(write_sz < f_block_sz) {
          printf("File write failed.\n");
          fclose(fp);
          exit(1);
        }
        bzero(revbuf, LENGTH);
      }
      printf("ok!\n");
      success = 1;
      fclose(fp);
    }
  }
}

/* main */
int main(int argc, char *argv[]) {
  int socketFd, portNo;
  struct sockaddr_in serverAddr;
  struct hostent *server;

  if (argc > 3) {
    errno = E2BIG;
    errorHandler(NULL, true);
  }

  /* Check for valid inputs */
  if (argc < 3) {
    errno = EINVAL;
    errorHandler(NULL, true);
  }

  /* check for a valid number */
  if (!isNumber(argv[2])) {
    errno = EINVAL;
    errorHandler(NULL, true);
  }
  portNo = atoi(argv[2]);

  /* get server details */
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    errorHandler("Failed to get the server for the hostname", false);
  }

  /* zeroing the address structures Good Practice*/
  bzero((char *) &serverAddr, sizeof(serverAddr));
  
  /* Creating the Internet domain socket */
  socketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFd < 0) {
    errorHandler("Socket creation failed", false);
  }

  /* Set server address values */
  serverAddr.sin_family = AF_INET;
  memcpy((char *) server->h_addr, (char *) &serverAddr.sin_addr.s_addr, 
                                                      server->h_length);
  serverAddr.sin_port = htons(portNo);
  if (connect(socketFd, (struct sockaddr *) &serverAddr, 
                                              sizeof(serverAddr)) < 0) {
    errorHandler("Conneting to server failed", false);
  }
  fileHandler(socketFd);
  close(socketFd);
  return 0; 
}
