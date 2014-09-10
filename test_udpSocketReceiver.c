#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ctype.h>
#include <time.h>
#include <math.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "include/globalDefs.h"
#include "include/util.h"

int main(int argc, char *argv[]) {
	clock_t begin, end;
	double time_spent;
	
	/* Arguments */
	char filePath[ARG_MAX];
	char serverName[ARG_MAX];
	
	int icounter = 0; /* Counter to spawn new process */
	//int splitPID;			/* split file process ID */
	
	//char splitFileName[4]; /* Current split File Name*/
	
	/* File Buffer Paramters */
	//FILE *stream;
	//char *contents;
	//int fileSize = 0;
	
	/* UDP connection variables */
	socklen_t receiverSocketLength;
	char receiveBuffer[BUFFER_LENGTH];
	int senderSocketFD, senderPortNo;
	struct sockaddr_in senderAddress, receiverAddress;
	int recvfromID;
	int fileSizeCounter =0;
	
	/* Added to parent process on Ctrl+C  */
	struct sigaction act;
	act.sa_sigaction= &handleKill;
	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_SIGINFO;
	sigaction(SIGINT, &act, NULL);
	
	if (argc < 3){
		errno = EINVAL;
		errorHandler(NULL,true);
	} else if (argc > 3) {
		errno = E2BIG;
		errorHandler(NULL,true);
	}
	
	strcpy(serverName,argv[1]); 
	strcpy(filePath,argv[2]);

	initializeProcessHandler();
	//printf("Start Time: %f \n", (double)(clock()) / CLOCKS_PER_SEC);
	//splitPID = fork();
	
	//if(splitPID == 0){
		//splitFile(filePath, SPLIT_COUNT);
	//}else{
		//begin = clock();
		//sleep(10);
		//printf("Start Time: %f \n", (double)(begin) / CLOCKS_PER_SEC);
		//wait(&splitPID); /* Wait for the file to Split */
		//end = clock();
		//printf("End Time: %f \n", (double)(end) / CLOCKS_PER_SEC);
		//time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		//printf("Time taken: %f \n", time_spent);
		
		//splitFileName[0]= 120;
		//splitFileName[3]= '\0';
		
		while(icounter < MAX_PROCESS){
			processID[icounter] = fork();
			
			if(processID[icounter] == 0){ /* Child forked */
				printf("Child Process-%d\n",icounter+1);
				
				/* Generate splitted file name */
				//splitFileName[1] = ((int) (icounter / 26)) + 97;
				//splitFileName[2] = icounter + 97; 
				
				//printf("splitFileName %d %d - %s\n",splitFileName[1],splitFileName[2],splitFileName);
				//Open the stream. Note "b" to avoid DOS/UNIX new line conversion.
			/*	stream = fopen(splitFileName, "rb");

				//Seek to the end of the file to determine the file size
				fseek(stream, 0L, SEEK_END);
				fileSize = ftell(stream);
				fseek(stream, 0L, SEEK_SET);

				//Allocate enough memory (add 1 for the \0, since fread won't add it)
				contents = malloc(fileSize+1);

				//Read the file 
				size_t size=fread(contents,1,fileSize,stream);
				contents[size]=0; // Add terminating zero.

				//printf("Read %s\n", contents);

				//Close the file
				fclose(stream);
				printf("Got the entire file part-%d\n",icounter+1); */
				
				begin = clock();
				
				/* Create a UDP Socket */
				senderSocketFD = socket(AF_INET, SOCK_DGRAM, 0);
				
				if (senderSocketFD < 0){
					errorHandler("Sender Socket Error!",false);
				}
				
				bzero((char *) &senderAddress, sizeof(senderAddress));
				
				senderPortNo = icounter + 3333;
				senderAddress.sin_family = AF_INET;
				senderAddress.sin_addr.s_addr = INADDR_ANY;
				senderAddress.sin_port = htons(senderPortNo);
				
				if (bind(senderSocketFD, (struct sockaddr *) &senderAddress, sizeof(senderAddress)) < 0){
					errorHandler("Sender Socket Bind Error!",false);
				}
	
				receiverSocketLength = sizeof(receiverAddress);
				bzero(receiveBuffer, BUFFER_LENGTH);
				
				recvfromID = recvfrom(senderSocketFD,receiveBuffer,BUFFER_LENGTH,0,(struct sockaddr *)&receiverAddress,&receiverSocketLength);
				
				if(recvfromID < 0){
					errorHandler("Receiving Error!",false);
				}
				printf("%s\n",receiveBuffer);
				
				//fprintf(stdout, "Here is the message: %s\n",receiveBuffer);
				
				/* Send File back  to Sender */
				if(sendto(senderSocketFD, receiveBuffer, sizeof(receiveBuffer), 0, (struct sockaddr *)&receiverAddress, receiverSocketLength) < 0) {
					errorHandler("Send Error!",false);
				}
				
				close(senderSocketFD);
				end = clock();
				time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
				printf("Time taken: %f  for Split - %d\n", time_spent,icounter+1);
		
				//fileSend(senderSocketFD, receiverAddress, receiverSocketLength, );
				
				//killProcess(icounter, SIGKILL); Not working
				exit(1); /* Kill Child */
			}else{ /* Parent */
				printf("Parent Process-%d\n",icounter+1);
				icounter++;
			}
		}
		while(1);
	//}	
	return 0;
}
