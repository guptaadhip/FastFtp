#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "include/globalDefs.h"
#include "include/util.h"

int main(int argc, char *argv[]) {

	char filePath[ARG_MAX];
	char serverName[ARG_MAX];
	
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

	//splitFile(filePath, 10);

	initializeProcessHandler();
	
	while(1){}
	
	return 0;
}
