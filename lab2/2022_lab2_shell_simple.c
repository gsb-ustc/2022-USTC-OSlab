#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CMDLINE_LENGTH  1024    /* max cmdline length in a line*/
#define MAX_BUF_SIZE        4096    /* max buffer size */
#define MAX_CMD_ARG_NUM     32      /* max number of single command args */
#define WRITE_END 1     // pipe write end
#define READ_END 0      // pipe read end


/*  
    int split_string(char* string, char *sep, char** string_clips);

    function:       split string by sep and delete whitespace at clips' head & tail

    arguments:      char* string, Input, string to be splitted 
                    char* sep, Input, the symbol used to split string
                    char** string_clips, Input/Output, giving an allocated char* array 
                                                and return the splitted string array

    return value:   int, number of splitted strings 
*/

int split_string(char* string, char *sep, char** string_clips) {
    
    char string_dup[MAX_BUF_SIZE];
    string_clips[0] = strtok(string, sep);
    int clip_num=0;
    
    do {
        // sscanf(string_clips[clip_num], "%s", string_clips[clip_num]);
        char *head, *tail;
        head = string_clips[clip_num];
        tail = head + strlen(string_clips[clip_num]) - 1;
        while(*head == ' ' && head != tail)
            head ++;
        while(*tail == ' ' && tail != head)
            tail --;
        *(tail + 1) = '\0';
        string_clips[clip_num] = head;
        clip_num ++;
    }while(string_clips[clip_num]=strtok(NULL, sep));
    return clip_num;
}

/*
    执行内置命令
    arguments:
        argc: 命令的参数个数
        argv: 依次代表每个参数，注意第一个参数就是要执行的命令，
        若执行"ls a b c"命令，则argc=4, argv={"ls", "a", "b", "c"}
    return:
        int, 若执行成功返回0，否则返回值非零
*/
int exec_builtin(int argc, char**argv, int *fd) {
    if(argc == 0) {
        return 0;
    }
    /* 此处实现和添加内置指令 */
    // 可以挖空
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1])
            chdir(argv[1]);
        return 0;
    } else if (strcmp(argv[0], "pwd") == 0) {
        char wd[4096];
        dprintf(fd[WRITE_END], "%s", getcwd(wd, 4096));
        return 0;
    } else if (strcmp(argv[0], "exit") == 0){
        exit(0);
    } else {
        // 不是内置指令
        return -1;
    }
}


/*
    从argv中删除重定向符和随后的参数，并打开对应的文件，将文件描述符放在fd数组中。
    运行后，fd[0]读端的文件描述符，fd[1]是写端的文件描述符
*/

int process_redirect(int argc, char** argv, int *fd) {
    fd[READ_END] = STDIN_FILENO;
    fd[WRITE_END] = STDOUT_FILENO;
    int i = 0, j = 0;
    while(i < argc) {
        int tfd;
        if(strcmp(argv[i], ">") == 0) {
            tfd = open(argv[i+1], O_RDWR | O_CREAT, 0666);
            if(tfd < 0) {
                printf("open '%s' error: %s\n", argv[i+1], strerror(errno));
            } else {
                fd[WRITE_END] = tfd;
            }
            i += 2;
        } else if(strcmp(argv[i], ">>") == 0) {
            tfd = open(argv[i+1], O_RDWR | O_CREAT | O_APPEND, 0666);
            if(tfd < 0) {
                printf("open '%s' error: %s\n", argv[i+1], strerror(errno));
            } else {
                fd[WRITE_END] = tfd;
            }
            i += 2;
        } else if(strcmp(argv[i], "<") == 0) {
            tfd = open(argv[i+1], O_RDONLY);
            if(tfd < 0) {
                printf("open '%s' error: %s\n", argv[i+1], strerror(errno));
            } else {
                fd[READ_END] = tfd;
            }
            i += 2;
        } else {
            argv[j++] = argv[i++];
        }
    }
    argv[j] = NULL;
    return j;   // 新的argc
}


/*
    在本进程中执行，且执行完毕后结束进程。
    arguments:
        argc: 命令的参数个数
        argv: 依次代表每个参数，注意第一个参数就是要执行的命令，
        若执行"ls a b c"命令，则argc=4, argv={"ls", "a", "b", "c"}
    return:
        int, 若执行成功则不会返回（进程直接结束），否则返回非零
*/
int execute(int argc, char** argv) {
    int fd[2];
    argc = process_redirect(argc, argv, fd);
    if(exec_builtin(argc, argv, fd) == 0) {
        exit(0);
    }
    dup2(fd[READ_END], STDIN_FILENO);
    dup2(fd[WRITE_END], STDOUT_FILENO);
    execvp(argv[0], argv);
    exit(255);
}

int main() {
    /* 输入的命令行 */
    char cmdline[MAX_CMDLINE_LENGTH];

    /* 由管道操作符'|'分割的命令行各个部分，每个部分是一条命令 */
    char *tasks[128], *commands[128];
    int task_count, cmd_count;
    while (1) {
        char *cwd = getcwd(NULL, 0);
        printf("shell:%s -> ", cwd);
        fflush(stdout);

        fgets(cmdline, 256, stdin);
        strtok(cmdline, "\n");

        /* 基于;拆解命令行 */
        task_count = split_string(cmdline, ";", tasks);
        for(int index = 0;index < task_count;index++){

        
            /* 拆解命令行 */
            cmd_count = split_string(tasks[index], "|", commands);

            if(cmd_count == 0) {
                continue;
            } else if(cmd_count == 1) {     // 没有管道的单一命令
                char *argv[MAX_CMD_ARG_NUM];
                int argc = split_string(commands[0], " ", argv);
                int fd[2];
                argc = process_redirect(argc, argv, fd);
                if(exec_builtin(argc, argv, fd) == 0) {
                    continue;
                }
                pid_t pid = fork();
                if (pid == 0) {
                    dup2(fd[READ_END], STDIN_FILENO);
                    dup2(fd[WRITE_END], STDOUT_FILENO);
                    /* 子进程 */
                    execvp(argv[0], argv);
                    /* execvp失败 */
                    exit(255);
                }
                wait(NULL);
            } else if(cmd_count == 2) {     // 两个命令间的管道
                int pipefd[2];
                int ret = pipe(pipefd);
                //这里可以调整是否BLOCK
                // int flags = fcntl(pipefd[0], F_GETFL);
                // fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
                if(ret < 0) {
                    printf("pipe error!\n");
                    continue;
                }
                int pid = fork();
                if(pid == 0) {  // 子进程1
                    close(pipefd[READ_END]);
                    dup2(pipefd[WRITE_END], STDOUT_FILENO);
                    close(pipefd[WRITE_END]);
                    
                    char *argv[MAX_CMD_ARG_NUM];
                    int argc = split_string(commands[0], " ", argv);
                    execute(argc, argv);
                    exit(255);
                }
                pid = fork();
                if(pid == 0) {  // 子进程2
                    close(pipefd[WRITE_END]);
                    dup2(pipefd[READ_END], STDIN_FILENO);
                    close(pipefd[READ_END]);
                    sleep(5);
                    char *argv[MAX_CMD_ARG_NUM];
                    int argc = split_string(commands[1], " ", argv);
                    execute(argc, argv);
                    exit(255);
                }
                close(pipefd[WRITE_END]);
                close(pipefd[READ_END]);
                
                // 等待所有子进程结束
                while (wait(NULL) > 0);
            } else {    // 三个以上的命令，选做
                int read_fd;    // 上一个管道的读端口（出口）
                for(int i=0; i<cmd_count; i++) {
                    int pipefd[2];
                    if(i != cmd_count -1){  // n条命令只需要n-1个管道
                        int ret = pipe(pipefd);
                        if(ret < 0) {
                            printf("pipe error!\n");
                            break;
                        }
                    }
                    int pid = fork();
                    if(pid == 0) {
                        // 除了最后一条命令外，都将标准输出重定向到当前管道入口
                        if(i != cmd_count-1) {
                            close(pipefd[READ_END]);
                            dup2(pipefd[WRITE_END], STDOUT_FILENO);
                            close(pipefd[WRITE_END]);
                        }
                        
                        // 除了第一条命令外，都将标准输出重定向到上一个管道入口
                        if(i != 0) {
                            dup2(read_fd, STDIN_FILENO);
                            close(read_fd);
                        }

                        char *argv[MAX_CMD_ARG_NUM];
                        int argc = split_string(commands[i], " ", argv);
                        execute(argc, argv);
                        exit(255);
                    }
                    if(i != 0){
                        close(read_fd);
                    }
                    if(i != cmd_count-1){
                        close(pipefd[WRITE_END]);
                        read_fd = pipefd[READ_END];
                    }
                }
                // 等待所有子进程结束
                while (wait(NULL) > 0);
            }
        }

    }
}
