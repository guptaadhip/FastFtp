#include "include/util.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPLIT "/usr/bin/split"

int splitFile(char* fileName, int splitCount) {
  char command[256];
  char splitCountStr[10];
  int rc = 0;
  sprintf(splitCountStr, "%d", splitCount);
  rc = execl(SPLIT, SPLIT, fileName, "-n", splitCount, (char *) 0);
  return rc;
}

/* 
 *	Function 	- usage - Prints the Usage Details
 *	Parameter 	- None
 *	Return		- NULL
 */
void printUsage() {
  fprintf(stdout, "Fast & Reliable File Transfer Protocol. Usage:\n");
	fprintf(stdout, "fsft [ServerName] [FilePath]\n\n");   
	fprintf(stdout, "Required arguments:\n");
	fprintf(stdout, "\t[ServerName] : Server IP Address or Hostname\n");
	fprintf(stdout, "\t[FilePath] : File Path\n");
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
