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
#include <ctype.h>

#define MAX_CMD_ARG 10
#define BUFSIZE 256
#define MAX_CMD_GRP 10

const char *prompt = "myshell> ";
char *cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZE];
int background;
char *cmdpipe[MAX_CMD_ARG];
char *cmdgrps[MAX_CMD_GRP];

void fatal(char *str)
{
    perror(str);
    exit(1);
}

int makelist(char *s, const char *delimiters, char **list, int MAX_LIST)
{
    int i = 0;
    int numtokens = 0;
    char *snew = NULL;

    if ((s == NULL) || (delimiters == NULL))
        return -1;

    snew = s + strspn(s, delimiters); /* Skip delimiters */
    if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
        return numtokens;

    numtokens = 1;

    while (1)
    {
        if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
            break;
        if (numtokens == (MAX_LIST - 1))
            return -1;
        numtokens++;
    }
    return numtokens;
}

void Cd(int argc, char **argv)
{
    int result;
    if (argc == 1)
    {
        result = chdir(getenv("HOME"));
        if (result == -1)
        {
            printf("change directory error\n");
        }
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "~") == 0)
        {
            result = chdir(getenv("HOME"));
            if (result == -1)
            {
                printf("change directory error\n");
            }
        }
        else
        {
            result = chdir(argv[1]);
            if (result == -1)
            {
                printf("No such Directory\n");
            }
        }
    }
    else
    {
        printf("using cd error\n");
    }
}

void childProcess_handler()
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        printf("\n<<child process terminated>>\n");
    }
}

void Redirection(char *cmdline)
{
    char *fileName;
    int fd;
    int l = strlen(cmdline);

    for (int i = 0; i < l; i++)
    {
        if (cmdline[i] == '>')
        {
            fileName = strtok(&cmdline[i + 1], " \t");
            fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            dup2(fd, 1);
            cmdline[i] = '\0';
        }
        if (cmdline[i] == '<')
        {
            fileName = strtok(&cmdline[i + 1], " \t");
            fd = open(fileName, O_RDONLY | O_CREAT, 0777);
            dup2(fd, 0);
            cmdline[i] = '\0';
        }
    }
}

void RedirectionCommand(char *cmdgrp)
{
    int cnt = 0;
    Redirection(cmdgrp);

    cnt = makelist(cmdgrp, " \t", cmdvector, MAX_CMD_ARG);
    execvp(cmdvector[0], cmdvector);

    fatal("error");
}

void Pipe(char *command)
{
    int fd[2];
    int cnt;

    cnt = makelist(command, "|", cmdpipe, MAX_CMD_ARG); // | 를 기준으로 makelist

    for (int i = 0; i < cnt - 1; i++)
    {
        pipe(fd);
        switch (fork())
        {
        case 0: // 자식 process
            close(fd[0]); // 입력 file descriptor close
            dup2(fd[1], 1); // dup2 함수 활용 ! 새롭게 1로 지정, 열려있다면 닫고 복제.
            RedirectionCommand(cmdpipe[i]);
        case -1:
            fatal("fork error");
        default:
            close(fd[1]); // 출력 file descriptor close
            dup2(fd[0], 0); // 0을 표준 출력으로 지정
        }
    }
    RedirectionCommand(cmdpipe[cnt - 1]);
}

int main(int argc, char **argv)
{
    int cnt = 0;
    int cdCnt = 0;
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

    while (1)
    {
        fputs(prompt, stdout);
        fgets(cmdline, BUFSIZE, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';
        cnt = makelist(cmdline, ";", cmdgrps, MAX_CMD_ARG);

        for (int i = 0; i < cnt; i++)
        {
            background = 0;
            for (int j = 0; j < strlen(cmdgrps[i]); j++)
            {
                if (cmdgrps[i][j] == '&')
                {
                    background = 1;
                    cmdgrps[i][j] = '\0';
                    break;
                }
            }

            if (cmdgrps[i][0] == 'c' && cmdgrps[i][1] == 'd')
            {
                cdCnt = makelist(cmdgrps[i], " \t", cmdvector, MAX_CMD_ARG);
                Cd(cdCnt, cmdvector);
            }
            else if (strcmp("exit", cmdgrps[i]) == 0)
            {
                printf("myshell exit...\n");
                exit(1);
            }
            else
            {
                switch (pid = fork())
                {
                case 0:
                {
                    signal(SIGTSTP, SIG_DFL);
                    signal(SIGQUIT, SIG_DFL);
                    signal(SIGINT, SIG_DFL);
                    setpgid(0, 0);
                    if (!background)
                        tcsetpgrp(STDIN_FILENO, getpgid(0));
                    Pipe(cmdgrps[i]);
                    break;
                }
                case -1:
                {
                    fatal("main()");
                }
                default:
                {
                    if (!background)
                    {
                        waitpid(pid, NULL, 0);
                        tcsetpgrp(STDIN_FILENO, getpgid(0));
                    }
                    else
                    {
                        break;
                    }
                    fflush(stdout);
                }
                }
            }
        }
    }
    return 0;
}