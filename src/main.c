#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
                if (c == '\'') state->current_state = NORMAL;
                else state->token_buffer[state->token_index++] = c;
                break;
            case DQUOTE:
                if (c == '\"') state->current_state = NORMAL;
                else state->token_buffer[state->token_index++] = c;
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

        char *cmd = working_state.tokens[0].text;

        if (strcmp(cmd, "exit") == 0) return 0;

        if (strcmp(cmd, "echo") == 0) {
            for (int i = 1; i < working_state.token_count; i++) {
                printf("%s%s", working_state.tokens[i].text, (i == working_state.token_count - 1) ? "" : " ");
            }
            printf("\n");
        } 
        else if (strcmp(cmd, "cd") == 0) {
            char *path = (working_state.token_count > 1) ? working_state.tokens[1].text : getenv("HOME");
            if (path == '~') {
              path = getenv("HOME");
            }
            if (chdir(path) != 0) perror("cd");
        }
        else {
            pid_t pid = fork();
            if (pid == 0) {
                char *args[65];
                for (int i = 0; i < working_state.token_count; i++) {
                    args[i] = working_state.tokens[i].text;
                }
                args[working_state.token_count] = NULL;
                execvp(args[0], args);
                fprintf(stderr, "%s: command not found\n", args[0]);
                exit(1);
            } else {
                wait(NULL);
            }
        }
        for (int i = 0; i < working_state.token_count; i++) {
            free(working_state.tokens[i].text);
        }
    }
    return 0;
}