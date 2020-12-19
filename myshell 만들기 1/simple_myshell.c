#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";
char *cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ];
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

int main(int argc, char **argv) {
    int i = 0;
    int count = 0;
    pid_t pid;
    cmdline[strlen(cmdline) - 1] = '\0';


    while (1) {
        fputs(prompt, stdout);
        fgets(cmdline, BUFSIZ, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';
        count = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

        if (strcmp("&", cmdvector[count - 1]) == 0) {
            background = 1;
            cmdvector[count - 1] = NULL;
        }

        if (strcmp("cd", cmdvector[0]) == 0) {
            Cd(count, cmdvector);
        }
        else if (strcmp(cmdvector[0], "exit") == 0) {
            printf("myshell exit...\n");
            exit(1);
        } else {
            switch (pid = fork()) {
                case 0:
                    execvp(cmdvector[0], cmdvector);
                    fatal("main()");
                case -1:
                    fatal("main()");
                default: {
                    if(!background) {
                        waitpid(pid, NULL, 0);
                    }
                }
            }
        }
    }
    return 0;
}
