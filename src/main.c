#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
        char *command_name = strdup(command);
        char *space = strchr(command_name, ' ');
        if (space != NULL) {
          *space = '\0';
        }
        char *path = getenv("PATH");
        if (path != NULL) {
          char *pathcopy = strdup(path);
          char *dir = strtok(pathcopy, PATH_SEPARATOR);
          int found = 0;
          while (dir != NULL) {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command_name);
            if (access(fullpath, X_OK) == 0) {
              found = 1;
              pid_t pid = fork();
              if(pid == 0) {
                char *args[64];
                int arg_count = 0;
                char *command_copy = strdup(command);
                char *token = strtok(command_copy, " ");
                while (token != NULL && arg_count < 63) {
                  args[arg_count++] = token;
                  token = strtok(NULL, " ");
                }
                args[arg_count] = NULL;
                execv(fullpath, args);
                exit(1);
              } else {
                int status;
                waitpid(pid, &status, 0);
                break;
              }
            }
            dir = strtok(NULL, PATH_SEPARATOR);
          }
          free(pathcopy);
          free(command_name);
          if (!found) {
            printf("%s: not found\n", command);
          }
        } else {
        printf("%s: not found\n", command);
      }
    } 
    
  }
}