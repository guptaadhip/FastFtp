#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/globalDefs.h"
#include "include/util.h"

#define SPLIT "/usr/bin/split"

struct processHandler *processHandle;

int splitFile(char* fileName, int splitCount) {
  char splitCountStr[SPLIT_COUNT];
  int rc = 0;
  sprintf(splitCountStr, "%d", splitCount);
  rc = execl(SPLIT, SPLIT, fileName, "-n", splitCountStr, (char *) 0);
  printf("Error: %d\n", rc);
  return rc;
}

/* 
 *	Function 	- usage - Prints the Usage Details
 *	Parameter 	- None
 *	Return		- NULL
 */
void printUsage() {
  fprintf(stdout, "Fast & Reliable File Transfer Protocol.\nUsage:\n");
	fprintf(stdout, "\tfsft [ServerName] [FilePath]\n");   
	fprintf(stdout, "\tRequired arguments:\n");
	fprintf(stdout, "\t\t[ServerName] : Server IP Address or Hostname\n");
	fprintf(stdout, "\t\t[FilePath] : File Path\n");
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
  /* Exit the program */
  exit(0);
}

/* Initialize processHandler */
void initializeProcessHandler(){
		int icounter = 0;
		
		for(icounter = 0;icounter < MAX_PROCESS;icounter++){
			processID[icounter] = -1;
		}
}

/* create Process */
/*
	processID[icounter] = fork();
*/

/* get Process ID */
pid_t getProcessID(){
	return getpid();
}

/* get Parent Process ID */
pid_t getParentProcessID(){
	return getppid();
}

/* get Join Process ID */
int joinProcess(int iprocessID);

/* Kill Process ID */
void killProcess(int iProcessID, int signal){
	if(processID[iProcessID] != 0){
		if(kill(processID[iProcessID],signal) == 0){
			processID[iProcessID] = 0;
		}
	}
	printf("killProcess");
}

/* 
* 	Function	- 	SigCatcher
*		Parameters 	-	void
*		Return		-	Null
*/
void SigCatcher(){
	wait3(NULL,WNOHANG,NULL);
}

/* 
* 	Function	- 	handleKill
*		Parameters 	-	void
*		Return		-	Null
*/
void handleKill(int i, siginfo_t *info, void *dummy){
	int icounter =0;
	errorHandler("Exiting!",false);
	
	for(icounter =0; icounter <MAX_PROCESS; icounter ++){
		killProcess(processID[icounter],i); /* Kill child */
	}
	exit(0); /* Kill Parent after killing all childs*/
}