#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

#define ARG_MAX 1024 /* max length for arguments */

int splitFile(char* fileName, int splitCount);

void printUsage();

void errorHandler(char *msg, bool usage);


#endif
