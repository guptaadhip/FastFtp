#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/util.h"

int main(int argc, char *argv[]) {
  //struct stat st;
  char fileName[100];
  if (argc < 2) {
    printf("Error in the outputs");
    exit(1);
  }
  //stat(fileName, &st);
  //size = st.st_size;
  strcpy(fileName, argv[1]);
  splitFile(fileName, 10);
  return 0;
}
