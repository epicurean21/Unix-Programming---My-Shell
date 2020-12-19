#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD_ARG 10
#define BUFSIZE 256

const char *prompt = "myshell> ";
char *cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZE];
int background;

void fatal(char *str) {
    perror(str);
    exit(1);
}

int makelist(char *s, const char *delimiters, char **list, int MAX_LIST) {
    int i = 0;
    int numtokens = 0;
    char *snew = NULL;

    if ((s == NULL) || (delimiters == NULL)) return -1;

    snew = s + strspn(s, delimiters);    /* Skip delimiters */
    if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
        return numtokens;

    numtokens = 1;

    while (1) {
        if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
            break;
        if (numtokens == (MAX_LIST - 1)) return -1;
        numtokens++;
    }
    return numtokens;
}

void Cd(int argc, char **argv) {
    int result;
    if (argc == 1) {
        result = chdir(getenv("HOME"));
        if (result == -1) {
            printf("change directory error\n");
        }
    } else if (argc == 2) {
        if (strcmp(argv[1], "~") == 0) {
            result = chdir(getenv("HOME"));
            if (result == -1) {
                printf("change directory error\n");
            }
        } else {
            result = chdir(argv[1]);
            if (result == -1) {
                printf("No such Directory\n");
            }
        }
    } else {
        printf("using cd error\n");
    }
}

void childProcess_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        printf("child process terminated\n");
    }
}

int main(int argc, char **argv) {
    int i = 0;
    int count = 0;
    pid_t pid;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = childProcess_handler;
    act.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &act, NULL);

    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    while (1) {
        fputs(prompt, stdout);
        fgets(cmdline, BUFSIZE, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';
        count = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);
        background = 0;
        if(count == 0)
            continue;
        if (strcmp("&", cmdvector[count - 1]) == 0) {
            background = 1;
            cmdvector[count - 1] = NULL;
        }

        if (strcmp("cd", cmdvector[0]) == 0) {
            Cd(count, cmdvector);
        } else if (strcmp(cmdvector[0], "exit") == 0) {
            printf("myshell exit...\n");
            exit(1);
        } else {
            switch (pid = fork()) {
                case 0: {
                    signal(SIGQUIT, SIG_DFL);
                    signal(SIGINT, SIG_DFL);
                    setpgid(0, 0);
                    if (!background)
                        tcsetpgrp(STDIN_FILENO, getpgid(0));
                    execvp(cmdvector[0], cmdvector);
                    fatal("main()");
                    break;
                }
                case -1: {
                    fatal("main()");
                }
                default: {
                    if (!background) {
                        waitpid(pid, NULL, 0);
                        tcsetpgrp(STDIN_FILENO, getpgid(0));
                    }
                }
            }
        }
    }
    return 0;
}