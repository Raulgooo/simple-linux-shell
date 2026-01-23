#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  char command[1024];
  setbuf(stdout, NULL);

  // TODO: Uncomment the code below to pass the first stage
  printf("$ ");
  fgets(command, sizeof(command), stdin);
  command[strcspn(command, "\n")] = '\0';
  printf("%s: command not found\n", command);


  return 0;
}
