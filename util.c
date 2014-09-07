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
