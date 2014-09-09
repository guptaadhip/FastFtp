/* Server side module for TCP command connection */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <errno.h>

#define ACK_MSG "I got your message"
#define MSG_PRINT "Here is your message: %s\n"
#define MSG_LISTEN "Listening on Port No: %d\n"

/* Anuj - Error function*/
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/* Adhip - Error Function -> Get it from the header file ?? */
void errorHandler(char *msg, bool usage) {
  /* print the error */
  if (msg == NULL) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
  } else {
    fprintf(stderr, "Error: %s\n", msg);
  }
  
  if (usage) {
    fprintf(stdout, "\n");
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

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char recv_udp_port[sizeof(int) + 1];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
	 
	 // Checking for valid inputs
     if (argc < 2) {
         errorHandler("ERROR, no port provided\n", true);
         exit(1);
     }
	 
	 // Creating the Internet domain socket 
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        errorHandler("ERROR opening socket",false);
	
     // zeroing the address structures 	
     bzero((char *) &serv_addr, sizeof(serv_addr));
     bzero((char *) &cli_addr, sizeof(cli_addr));	 
   
     // Check if the port number is proper 
	 if (!isNumber(argv[1])) {
		errno = EINVAL;
		errorHandler(NULL, true);
	 }
     portno = atoi(argv[1]);
     
	 // Set server address values and bind the socket 
	 serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
	 
     /* listening for connections.
	 backlog set to 1 to accept */ 	 
     listen(sockfd,1);    
     fprintf(stdout, MSG_LISTEN, portno);
	 clilen = sizeof(cli_addr);
	 
	 // accept connection and open socket
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
		
     // reading Receiver's UDP port number
     bzero(recv_udp_port,sizeof(int) + 1);
	 n = read(newsockfd,recv_udp_port,sizeof(int));
     if (n < 0) error("ERROR reading from socket");
     fprintf(stdout, "Here is Receiver's UDP port no. : %s\n",recv_udp_port);
     
	 // sending file transfer info to server
	 char info[200];
	 sprintf(info, "%d %ld", 4, 9999); 
	 n = write(newsockfd,info,sizeof(info));
     if (n < 0) error("ERROR writing to socket");
	 
	 // closing sockets
     close(newsockfd);
     close(sockfd);
     return 0; 
}
