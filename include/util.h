#ifndef UTIL_H
#define UTIL_H

#include "globalDefs.h"
#include <stdbool.h>
#include <sys/wait.h> 
#include <signal.h>

int splitFile(char* fileName, int splitCount);

void printUsage();

void errorHandler(char *msg, bool usage);

/* Process Record -- Handler */
pid_t processID[MAX_PROCESS];

pid_t getProcessID();	/* get Process ID */
pid_t getParentProcessID();	/* get Parent Process ID */
int joinProcess(int iProcessID);	/* get Join Process ID */
int killProcess(int iProcessID, int signal);	/* get Kill Process ID */
void handleKill(int i, siginfo_t *info, void *dummy);

#endif
