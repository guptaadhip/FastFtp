#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "include/globalDefs.h"
#include "include/util.h"

int main(int argc, char *argv[]) {

	char filePath[ARG_MAX];
	char serverName[ARG_MAX];
	
	if (argc < 3){
		errno = EINVAL;
		errorHandler(NULL,true);
	} else if (argc > 3) {
		errno = E2BIG;
		errorHandler(NULL,true);
	}
	
	strcpy(serverName,argv[1]); 
	strcpy(filePath,argv[2]);

	splitFile(filePath, SPLIT_COUNT);

  /* process creation */
	return 0;
}
