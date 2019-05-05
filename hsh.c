#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LINE_MAX 80
#define ARGV_MAX 12
#define SPACE ' '

#define PARSELINE hsh_parseline
#define BUILTIN_COMMAND hsh_builtin_command
#define EXEC hsh_exec
#define MAIN main

int open_flag;

bool PARSELINE(char *buf, char *argv[], int *count, bool *need_dup){
    char *p;
    bool is_background;
    buf[strlen(buf)-1] = SPACE;
    for(;*buf && (*buf == SPACE);buf++);
    int argc = 0;
    while((p = strchr(buf, SPACE))) {
        argv[argc++] = buf;
        *p = 0;
        buf = p + 1;
        for(;*buf && (*buf == SPACE);buf++);    //忽略多余的空格
    }
    argv[argc] = NULL;
    *count = argc;
    if(!argc)
        return true;
    if((*need_dup = (argc > 2 && *argv[argc-2] == '>'))){
        open_flag = O_CREAT | O_WRONLY | O_TRUNC;
        if((*argv[argc-2]) == '>' && (argv[argc-2][2]) == 0){
            open_flag = O_CREAT | O_WRONLY;
        }
    }
    if((is_background = (*argv[argc-1] == '&')) != 0)
        argv[--argc] = NULL;
    return is_background;
}

int BUILTIN_COMMAND(char *argv[]) {
    if(!strcmp("exit", argv[0]))
        exit(0);
    if(!strcmp("logout", argv[0]))
        exit(0);
    if(!strcmp("&", argv[0]))
        return 1;
    if(!strcmp("mua", argv[0])){
        printf("mua\n");
        return 1;
    }
    return 0;
}


void EXEC(char *cmdlinebuf) {
    char *argv[ARGV_MAX];
    char buf[LINE_MAX];
    pid_t pid;

    strcpy(buf, cmdlinebuf);
    int argc;
    bool need_dup = false;
    int copy_fd;
    bool is_background = PARSELINE(buf, argv, &argc, &need_dup);
    if(argv[0] == NULL)
        return;
    if(!BUILTIN_COMMAND(argv)) {
        if(!(pid = fork())) {
            if(need_dup){
                close(1);
                int fd = open(argv[ argc-1], open_flag, 0777);
                copy_fd = dup2(1, fd);
                argv[argc-1] = NULL;
                argv[argc-2] = NULL;
            }
            int ret = execvp(argv[0], argv);
            if(need_dup){
                close(1);
                dup2(copy_fd, 1);
            }
            if(ret < 0){
                printf("%s: command not found\n", argv[0]);
                exit(0);
            }
        }

        if(!is_background) {
            int status;
            if(waitpid(pid, &status, 0) < 0)
                perror("waitpid error!\n");
            // else
            //     printf("pid:%d %s", pid, cmdlinebuf);
        } else {
            printf("pid:%d %s", pid, cmdlinebuf);
        }
    }
}

int MAIN(int argc, char const *argv[]) {
    char cmdlinebuf[LINE_MAX];
    while(true){
        printf("%% ");
        fgets(cmdlinebuf, LINE_MAX, stdin);
        if(feof(stdin))
            exit(0);
        EXEC(cmdlinebuf);
    }
    return 0;
}
