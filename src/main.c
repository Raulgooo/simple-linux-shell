#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PATH_SEPARATOR ":"

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
      char *cmd_name = &command[5];
      if (strcmp(cmd_name, "exit") == 0 ||
          strcmp(cmd_name, "echo") == 0 ||
          strcmp(cmd_name, "type") == 0) {
        printf("%s is a shell builtin\n", cmd_name);
      } else {
          char *path = getenv("PATH");
          if (path != NULL) {
            char *pathcopy = strdup(path);
            char *dir = strtok(pathcopy, PATH_SEPARATOR);
            int found = 0;
            while (dir != NULL) {
              char fullpath[1024];
              snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd_name);
              if (access(fullpath, X_OK) == 0) {
                printf("%s is %s\n", cmd_name, fullpath);
                found = 1;
                break;
              }
              dir = strtok(NULL, PATH_SEPARATOR);
            }
            free(pathcopy);
            if (!found) {
              printf("%s: not found\n", cmd_name);
            }
          } else {
            printf("%s: not found\n", cmd_name);
          }
          continue;
        }
        continue;
      }
      if (strcmp(command, "exit") == 0) {
        return 0;
      }

      if (strncmp(command, "echo", 4) == 0) {
        printf("%s\n", &command[5]);
        continue;

      }
      else {
        printf("%s: not found\n", command);
        continue;
      }
    } 
  }
