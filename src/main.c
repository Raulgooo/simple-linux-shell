#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PATH_SEPARATOR ":"

typedef enum { NORMAL, SQUOTE, DQUOTE, BACKSLASH } ParseState;

typedef struct {
    char *text;
    int type; 
} Token;

struct State {
    ParseState current_state;
    ParseState prev_state;
    char token_buffer[1024];
    int token_index;
    int token_count;
    Token tokens[64];
};


void add_token(struct State *state, int is_operator) {
    if (state->token_index > 0) {
        state->token_buffer[state->token_index] = '\0';
        state->tokens[state->token_count].text = strdup(state->token_buffer);
        state->tokens[state->token_count].type = is_operator;
        state->token_count++;
        state->token_index = 0;
    }
}

void init_state(struct State *s) {
    s->current_state = NORMAL;
    s->prev_state = NORMAL;
    s->token_index = 0;
    s->token_count = 0;
    memset(s->token_buffer, 0, sizeof(s->token_buffer));
}

void tokenize(char *command, struct State *state) {
    int i = 0;
    init_state(state);

    while (command[i] != '\0') {
        char c = command[i];
        switch (state->current_state) {
            case NORMAL:
                if (c == ' ') {
                    add_token(state, 0);
                }
                else if (c == '>' || c == '|') {
                  add_token(state, 0);
                  if (c == '>' && command[i + 1] == '>') {
                      state->token_buffer[0] = '>';
                      state->token_buffer[1] = '>';
                      state->token_index = 2;
                      i++;
                  }
                  else if (c == '|' && command[i + 1] == '|') {
                      state->token_buffer[0] = '|';
                      state->token_buffer[1] = '|';
                      state->token_index = 2;
                      i++;
                  }

                  else {
                      state->token_buffer[0] = c;
                      state->token_index = 1;
                  }
                  add_token(state, 1);
                } 
                else if ((c == '1' || c == '2' || c == '&') && command[i+1] == '>' && state->token_index == 0){
                    add_token(state, 0);
                    state->token_buffer[0] = c;
                    state->token_buffer[1] = '>';
                    state->token_index = 2;
                    if (command[i + 2] == '>') {
                        state->token_buffer[2] = '>';
                        state->token_index = 3;
                        i++;
                    }
                    i++;
                    add_token(state, 1);  
                } else if (c == '\'') {
                    state->current_state = SQUOTE;
                } else if (c == '\"') {
                    state->current_state = DQUOTE;
                } else if (c == '\\') {
                    state->prev_state = NORMAL;
                    state->current_state = BACKSLASH;
                } else if (c == '|' || c == '>') {
                    add_token(state, 0);
                    state->token_buffer[0] = c;
                    state->token_index = 1;
                    add_token(state, 1);
                } else {
                    state->token_buffer[state->token_index++] = c;
                }
                break;

            case SQUOTE:
                if (c == '\'') {
                    state->current_state = NORMAL;
                } else {
                    state->token_buffer[state->token_index++] = c;
                }
                break;

            case DQUOTE:
                if (c == '\"') {
                    state->current_state = NORMAL;
                } 
                else if (c == '\\') {
                    char next = command[i + 1];
                    if (next == '\"' || next == '\\' || next == '$') {
                        i++; 
                        state->token_buffer[state->token_index++] = command[i]; 
                    } else {
                        state->token_buffer[state->token_index++] = c;
                    }
                } 
                else {
                    state->token_buffer[state->token_index++] = c;
                } 
                break;

            case BACKSLASH:
                state->token_buffer[state->token_index++] = c;
                state->current_state = state->prev_state;
                break;
        }
        i++; 
    }
    add_token(state, 0); 
}

int manage_redirects(struct State *state, int *target_fd) {
    for (int i = 0; i < state->token_count; i++) {
        Token current_token = state->tokens[i];
        if (i + 1 >= state->token_count) return -1;
        char *filename = state->tokens[i + 1].text;

        int flags = O_WRONLY | O_CREAT;
        int fd_to_replace = 1;
        int both = 0;

        if (strcmp(current_token.text, ">") == 0 || strcmp(current_token.text, "1>") == 0) flags |= O_TRUNC;
        else if (strcmp(current_token.text, ">>") == 0 || strcmp(current_token.text, "1>>") == 0) flags |= O_APPEND;
        else if (strcmp(current_token.text, "2>") == 0) { flags |= O_TRUNC; fd_to_replace = 2; }
        else if (strcmp(current_token.text, "2>>") == 0) { flags |= O_APPEND; fd_to_replace = 2; }
        else if (strcmp(current_token.text, "&>") == 0 || strcmp(current_token.text, ">&") == 0) { flags |= O_TRUNC; both = 1; }
        else if (strcmp(current_token.text, "&>>") == 0) { flags |= O_APPEND; both = 1; }
        else continue;

        int file_fd = open(filename, flags, 0644);
        if (file_fd < 0) { perror("open"); return -1; }

        int saved_fd = dup(1); // Backup para restaurar
        if (target_fd) *target_fd = fd_to_replace;

        if (both) {
            dup2(file_fd, 1);
            dup2(file_fd, 2);
        } else {
            dup2(file_fd, fd_to_replace);
        }
        close(file_fd);
        state->token_count = i;
        return saved_fd;
    }
    return -1;
}

int main() {
    setbuf(stdout, NULL);
    char command[1024];
    struct State working_state;

    while (1) {
        printf("$ ");
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = '\0';
        tokenize(command, &working_state);
        if (working_state.token_count == 0) continue;

        int original_count = working_state.token_count;
        char *cmd = working_state.tokens[0].text;
        int saved_fd = -1;
        int target_fd_id = 1;

        if (strcmp(cmd, "type") == 0) {
    if (working_state.token_count > 1) {
        char *arg = working_state.tokens[1].text;
        if (strcmp(arg, "echo") == 0 || strcmp(arg, "cd") == 0 || 
            strcmp(arg, "exit") == 0 || strcmp(arg, "type") == 0 || 
            strcmp(arg, "pwd") == 0) {
            printf("%s is a shell builtin\n", arg);
        } 
        else {
            char *path_env = getenv("PATH");
            int found = 0;
            if (path_env != NULL) {
                char *path_copy = strdup(path_env); 
                char *dir = strtok(path_copy, PATH_SEPARATOR);
                while (dir != NULL) {
                    char fullpath[1024];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, arg);
                    if (access(fullpath, X_OK) == 0) {
                        printf("%s is %s\n", arg, fullpath);
                        found = 1;
                        break;
                    }
                    dir = strtok(NULL, PATH_SEPARATOR);
                }
                free(path_copy);
            }
            if (!found) {
                printf("%s: not found\n", arg);
            }
                    }
    }
        }
        else if (strcmp(cmd, "exit") == 0) return 0;

        else if (strcmp(cmd, "echo") == 0) {
            saved_fd = manage_redirects(&working_state, &target_fd_id);
            for (int i = 1; i < working_state.token_count; i++) {
                printf("%s%s", working_state.tokens[i].text, (i == working_state.token_count - 1) ? "" : " ");
            }
            printf("\n");
        }

        else if (strcmp(cmd, "pwd") == 0) {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            } else {
                perror("pwd");
            }
        }
        else if (strcmp(cmd, "cd") == 0) {
            char *path = (working_state.token_count > 1) ? working_state.tokens[1].text : getenv("HOME");
            if (strcmp(path, "~") == 0) {
              path = getenv("HOME");
            }
            if (chdir(path) != 0) {
              printf("cd: %s: No such file or directory\n", path);
            }
        }
else {
    int pipe_idx = -1;
    for (int i = 0; i < working_state.token_count; i++) {
        if (strcmp(working_state.tokens[i].text, "|") == 0) {
            pipe_idx = i;
            break;
        }
    }

    if (pipe_idx != -1) {
        int pipe_fds[2];
        if (pipe(pipe_fds) == -1) { perror("pipe"); exit(1); }
        if (fork() == 0) {
            manage_redirects(&working_state, NULL);
            dup2(pipe_fds[1], STDOUT_FILENO);
            close(pipe_fds[0]);
            close(pipe_fds[1]);

            char *left_args[64];
            for (int i = 0; i < pipe_idx; i++) left_args[i] = working_state.tokens[i].text;
            left_args[pipe_idx] = NULL;

            execvp(left_args[0], left_args);
            perror("execvp left");
            exit(1);
        }
        if (fork() == 0) {
            dup2(pipe_fds[0], STDIN_FILENO);
            close(pipe_fds[0]);
            close(pipe_fds[1]);

            char *right_args[64];
            int count = 0;
            for (int i = pipe_idx + 1; i < working_state.token_count; i++) {
                right_args[count++] = working_state.tokens[i].text;
            }
            right_args[count] = NULL;

            execvp(right_args[0], right_args);
            perror("execvp right");
            exit(1);
        }

        close(pipe_fds[0]);
        close(pipe_fds[1]);
        wait(NULL);
        wait(NULL);
    } 
    else {
        pid_t pid = fork();
        if (pid == 0) {
            manage_redirects(&working_state, NULL);
            char *args[65];
            for (int i = 0; i < working_state.token_count; i++) args[i] = working_state.tokens[i].text;
            args[working_state.token_count] = NULL;
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        } else {
            wait(NULL);
        }
    }
}


    for (int i = 0; i < original_count; i++) {
      if (working_state.tokens[i].text != NULL) {
        free(working_state.tokens[i].text);
        working_state.tokens[i].text = NULL; 
      }
    }
    }
    return 0;   
}