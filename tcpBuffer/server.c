/* This is the TCP server socket */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <sys/wait.h>

#define ACK_MSG "I got your message"
#define MSG_PRINT "Here is your message: %s\n"
#define MSG_LISTEN "Listening on Port No: %d\n"
#define LENGTH 512 // Buffer length
char f_name[50];
char *contents;
long int fileSize = 0;

void printUsage() {
  fprintf(stdout, "TCP based Echo Server. Usage:\n");
  fprintf(stdout, "\tserver [portNo]\n\n");
  fprintf(stdout, "portNo: Integer range 1-65535 (Prefered: 2000 - 65535)\n");
}

/* lets Buffer */
void bufferFile() {
  FILE *stream;
  stream = fopen(f_name, "rb");
  fseek(stream, 0L, SEEK_END);
  fileSize = ftell(stream);
  fseek(stream, 0L, SEEK_SET);
  contents = malloc(fileSize+1);
  size_t size=fread(contents,1,fileSize,stream);
  contents[size]=0;
  fclose(stream);
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

/* reap the zombie: handler */
void zombieHandler(int signal) {
  pid_t pid;
  int status;
  while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
    if (pid == -1) {
      if (errno == EINTR) {
        continue;
      }
      break;
    } else if (pid == 0) {
      /* no more child processes */
      break;
    }
  }
}

void fileSend(int socketFd) {
  clock_t begin, end;
  double time_spent;
  begin = clock();
  if(send(socketFd, contents, fileSize, 0) < 0) {
    printf("ERROR: Failed to send file %s.\n", f_name);
    close(socketFd);
    exit(1);
  }
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time taken: %f Size: %ld Speed: %f", time_spent, fileSize, (float) fileSize/(time_spent * 1048576));
}

/* connection handler */
void connectionHandler(int socketFd) {
  /* create the read buffer and initialize */
  char buffer[256];
  memset(buffer, '\0', sizeof(buffer));
  /* lets buffer it */
  bufferFile();
  /* fileHandler */
  fileSend(socketFd);
  /* cleaning up */
  close(socketFd);
}

/* main */
int main(int argc, char *argv[]) {
  int socketFd, newSocketFd, portNo;
  struct sockaddr_in serverAddr, clientAddr;
  socklen_t clientAddrLength;
  pid_t pid;
  if (argc > 3) {
    errno = E2BIG;
    errorHandler(NULL, true);
  }
  /* Check for valid inputs */
  if (argc < 2) {
    errno = EINVAL;
    errorHandler(NULL, true);
  }
  /* check for a valid number */
  if (!isNumber(argv[1])) {
    errno = EINVAL;
    errorHandler(NULL, true);
  }
  portNo = atoi(argv[1]);

  memset(f_name, '\0', 50);
  strcpy(f_name, argv[2]);

  /* all looks good so far lets kill the zombies */
  struct sigaction sigAction;
  memset(&sigAction, 0, sizeof(sigAction));
  sigAction.sa_handler = zombieHandler;
  sigaction(SIGCHLD, &sigAction, NULL);

  /* zeroing the address structures Good Practice*/
  bzero((char *) &serverAddr, sizeof(serverAddr));
  bzero((char *) &clientAddr, sizeof(clientAddr));
  
  /* Creating the Internet domain socket */
  socketFd = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFd < 0) {
    errorHandler("Socket creation failed", false);
  }

  /* Set server address values */
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(portNo);
  if (bind(socketFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    errorHandler("Bind on socket failed", false);
  }

  /* Listen for incoming connections */
  listen(socketFd, 10);
  fprintf(stdout, MSG_LISTEN, portNo);
  clientAddrLength = sizeof(clientAddr);
  while (true) {
    /* accept the connection and open the socket */
    newSocketFd = accept(socketFd, (struct sockaddr *) &clientAddr, 
                                                  &clientAddrLength);
    if (newSocketFd < 0) {
      continue;
    }
    pid = fork();
    if (pid < 0) {
      errorHandler("Failed to fork child process", false);
    }
    /* child process */
    if (pid == 0) {
      close(socketFd);
      connectionHandler(newSocketFd);
      exit(0);
    } else {
      /* 
       * close the connection file descriptor 
       * reference for this process
       */
      close (newSocketFd);
    }
  }
  /* this will never be hit but for sanity reasons */
  close(socketFd);
  close (newSocketFd);
  return 0; 
}
