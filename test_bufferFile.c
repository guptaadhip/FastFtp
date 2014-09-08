#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void main(int argc, char *argv[]) {
  FILE *stream;
  char *contents;
  int fileSize = 0;

  //Open the stream. Note "b" to avoid DOS/UNIX new line conversion.
  stream = fopen(argv[1], "rb");
  
  //Seek to the end of the file to determine the file size
  fseek(stream, 0L, SEEK_END);
  fileSize = ftell(stream);
  fseek(stream, 0L, SEEK_SET);
  
  //Allocate enough memory (add 1 for the \0, since fread won't add it)
  contents = malloc(fileSize+1);
  
  //Read the file 
  size_t size=fread(contents,1,fileSize,stream);
  contents[size]=0; // Add terminating zero.

  printf("Read %s\n", contents);

  //Close the file
  fclose(stream);
  printf("Got the entire file\n");
}
