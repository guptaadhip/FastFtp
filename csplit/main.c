#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <string.h>

#define SPLITS 4

char *splits[SPLITS];
char f_name[50];

/* lets Buffer */
void bufferFile() {
  long int fileSize = 0;
  long int splitSize = 0;
  int i = 0;
  size_t size = 0;
  FILE *stream;
  stream = fopen(f_name, "rb");
  fseek(stream, 0L, SEEK_END);
  fileSize = ftell(stream);
  fseek(stream, 0L, SEEK_SET);
  splitSize = fileSize / SPLITS;
  for(i = 0; i < 4; i++) {
    splits[i] = malloc(splitSize+1);
    size=fread(splits[i],1,splitSize,stream);
    splits[i][size]=0;
    printf("Pointer Location: %ld\n", ftell(stream));
  }
  fclose(stream);
}

int main (int argc, char *argv[]) {
  clock_t begin, end;
  double time_spent;
  memset(f_name, '\0', 50);
  strcpy(f_name, argv[1]);
  begin = clock();
  bufferFile();
  end = clock();
  time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("Time taken: %f\n", time_spent);
  return 0;
}
