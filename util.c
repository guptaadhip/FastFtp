#include "include/util.h"
#include <unistd.h>

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
void usage(){
	printf("fsft [-d destination] [-f filename]\n");   
	printf("\tRequired arguments:\n");
	printf("\t-d, --destination \t: Send file the specified destination.\n");
	printf("\t-f, --file-name \t: Read the specified file.\n");
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
    usage();
  }
  /* Exit the program */
  exit(0);
}
