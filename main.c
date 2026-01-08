#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_INPUT 512
#define MAX_ARGS 64

char last_command[MAX_INPUT];  // برای دستور !!

// توابع کمکی
void split_command(char *input, char args) {
    char *token = strtok(input, " \t\n");
    int idx = 0;
    while (token != NULL && idx < MAX_ARGS - 1) {
        args[idx++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[idx] = NULL;
}

int handle_builtin(char args) {
    if (args[0] == NULL) return 1;

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL)
            fprintf(stderr, "Usage: cd <dir>\n");
        else if (chdir(args[1]) != 0)
            perror("cd");
        return 1;
    } else if (strcmp(args[0], "pwd") == 0) {
        char cwd[256];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("pwd");
        return 1;
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i] != NULL; i++)
            printf("%s ", args[i]);
        printf("\n");
        return 1;
    } else if (strcmp(args[0], "!!") == 0) {
        if (strlen(last_command) == 0) {
            printf("No commands in history.\n");
        } else {
            printf("Repeating last command: %s\n", last_command);
            char backup[MAX_INPUT];
            strcpy(backup, last_command);
            char *args2[MAX_ARGS];
            split_command(backup, args2);
            handle_builtin(args2);
        }
        return 1;
    }
    return 0;
}

void execute_pipe(char *cmd1, char *cmd2, int bg_flag) {
    int pipefd[2];
    pipe(pipefd);
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // first child
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char *args1[MAX_ARGS];
        split_command(cmd1, args1);
        execvp(args1[0], args1);
        perror("pipe exec 1");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // second child
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char *args2[MAX_ARGS];
        split_command(cmd2, args2);
        execvp(args2[0], args2);
        perror("pipe exec 2");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    if (!bg_flag) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    }
}

void execute_command(char *input) {
    // بررسی pipe
    int bg_flag = 0;
    char *pipe_pos = strchr(input, '|');
    char *bg_pos = strchr(input, '&');

    if (bg_pos) {
        bg_flag = 1;
        *bg_pos = '\0';
    }

    if (pipe_pos != NULL) {
        *pipe_pos = '\0';
        char *cmd1 = input;
        char *cmd2 = pipe_pos + 1;
        execute_pipe(cmd1, cmd2, bg_flag);
        return;
    }

    // بدون pipe
    char *args[MAX_ARGS];
    split_command(input, args);

    if (handle_builtin(args))
        return;

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } else if (pid < 0) {
        perror("fork failed");
        return;
    } else {
        if (!bg_flag)
            waitpid(pid, NULL, 0);
    }
}

int main() {
    char input[MAX_INPUT];

    while (1) {
        printf("student@unix$ ");
        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;

        // حذف فاصله خالی
        if (input[0] == '\n') continue;

        // ذخیره برای !! history
        strcpy(last_command, input);

        execute_command(input);

        // جمع‌آوری zombieها
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    return 0;
}




