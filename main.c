#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <ctype.h>
#include <time.h>

#include "include/globalDefs.h"
#include "include/util.h"

int main(int argc, char *argv[]) {
	char filePath[ARG_MAX];
	char serverName[ARG_MAX];
	int icounter = 0; /* Counter to spawn new process */
	int splitPID;
	char splitFileName[4];
	
	/* File Buffer Paramters */
	FILE *stream;
	char *contents;
	int fileSize = 0;

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
	
	splitPID = fork();
	
	if(splitPID == 0){
		splitFile(filePath, SPLIT_COUNT);
	}else{
	
		wait(&splitPID); /* Wait for the file to Split */
		
		splitFileName[0]= 120;
		splitFileName[3]= '\0';
		
		while(icounter <= MAX_PROCESS){
			processID[icounter] = fork();
			
			if(processID[icounter] == 0){ /* Child forked */
				printf("Child Process-%d\n",icounter+1);
				
				/* Generate splitted file name */
				splitFileName[1] = ((int) (icounter / 26)) + 97;
				splitFileName[2] = icounter + 97; 
				
				//Open the stream. Note "b" to avoid DOS/UNIX new line conversion.
				stream = fopen(splitFileName, "rb");

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
				printf("Got the entire file part-%d\n",icounter+1);
				
				//killProcess(icounter, SIGKILL); Not working
				exit(0); /* Kill Child */
			}else{ /* Parent */
				printf("Parent Process-%d\n",icounter+1);
				icounter++;
			}
		}
	}	
	return 0;
}
