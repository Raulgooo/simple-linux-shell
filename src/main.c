#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  char command[1024];
  while (1) {
    printf("$ ");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = '\0';
    int length = strlen(command);

    if (length == 0) {
      continue;
    }

    if (strncmp(command, "type", 4) == 0) {
      if (strcmp(&command[5], "echo") == 0 || strcmp(&command[5], "exit") == 0 || strcmp(&command[5], "type") == 0) {
        printf("%s is a shell builtin\n", &command[5]);
        continue;
      } 
      else {
        printf("%s: not found\n", &command[5]);
        continue;
      }
    }

    if (strcmp(command, "exit") == 0) {
      return 0;
    }
    
    if (strncmp(command, "echo", 4) == 0) {
      printf("%s\n", &command[5]);
      continue;
    }

    else {
      printf("%s: command not found\n", command);
      continue;
    }
  }
}
