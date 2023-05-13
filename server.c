#define _XOPEN_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8099

#define BUF_SIZE 512

// Client CMDS
#define CMD_QUIT  "quit"
#define CMD_FIND  "findfile"
#define CMD_SIZE  "sgetfiles"
#define CMD_DATE  "dgetfiles"
#define CMD_FILES "getfiles"
#define CMD_TAR   "gettargz"

#define CMD_U "-u"

#define ACK_APT "ACCEPT"
#define ACK_RJT "REJECT"

int REQUEST_COUNT = 0;

char * getfileName(char* type) {
    char * file_name = (char*) malloc(sizeof(char) * 64);
    sprintf(file_name, "server_%s_%d.dump", type, REQUEST_COUNT);
    return file_name;
}

char * gettarName(char* type) {
    char * file_name = (char*) malloc(sizeof(char) * 64);
    sprintf(file_name, "tmp_%s_%d.tar.gz", type, REQUEST_COUNT);
    return file_name;
}

void execute_find_to_file(char* cmd, char** cmd_arg, char* file_name) {
    int fd_dp = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0777);
    int ret = dup2(fd_dp, STDOUT_FILENO);
    int n = execvp(cmd, cmd_arg);
    if ( n < 0 ) {
        printf("Error , Issues in execution of execvp. FIND\n");
    }
}

void tar_files_from_file(char* file, char* tar_file) {
    char * cmd = "tar";
    char * cmd_arg[] = {cmd, "--transform", "s/.*\\///g", "-czf", tar_file, "-P", "--null", "-T", file, NULL};
    //printf("%s\n", cmd_arg[2]);
    int n = execvp(cmd, cmd_arg);
    if ( n < 0 ) {
        printf("Error , Issues in execution of execvp TAR.\n");
    }
}

int check_files_size_for_tar(char * file) {
    int fd_c = open(file, O_RDONLY);
    if (fd_c < 3) {
        return 0;
    }
    char *buf = (char*) malloc(sizeof(char) * BUF_SIZE);
    int rn = read(fd_c, buf, BUF_SIZE);
    if (rn < 1) {
        close(fd_c);
        return 0;
    }
    close(fd_c);
    return rn;
}

char* split_command(char * command, char ** argv, int * S_COUNT) {
    char* tmp = (char*) malloc(sizeof(char) * (strlen(command) + 2));
    memset(tmp, '\0', sizeof(char) * (strlen(command) + 2));
    strcpy(tmp, command);
    argv[*S_COUNT] = tmp;
    command[0] = '\0';
    *S_COUNT = *S_COUNT + 1;
    return command; 
}

int process_cmd(char * stmt, int *csd) {
    // Quit string
    if (strlen(stmt) == 4 && strcmp(stmt, CMD_QUIT) == 0) {
        char * serverResponse = "Connection Closed.";
        write(*csd, serverResponse, strlen(serverResponse));
        close(*csd);
        return 0;
    }
    int count = 0;
    int S_COUNT = 0;
    char * command = (char*) malloc(sizeof(char) * strlen(stmt) + 2);
    command[0] = '\0';
    char **argv = (char**) malloc(sizeof(char) * (strlen(stmt) + 40));
    memset(argv, '\0', sizeof(char) * (strlen(stmt)+40));
    int ccount = 0;
    while (stmt[count] != '\0') {
        //printf("W : %c : %s\n", stmt[count], command);
        switch (stmt[count]) {
            case ' ':
                command = split_command(command, argv, &S_COUNT);
                ccount = 0;
                break;
            default:
                command[ccount] = stmt[count];
                ccount++;
                command[ccount] = '\0';
                break;
        }
        count++;
    }
    if (strlen(command) > 0 || S_COUNT > 0) {
        command = split_command(command, argv, &S_COUNT);
    }
    if (strcmp(argv[0], CMD_FIND) == 0) {
        char * file_name = getfileName("find");
        int pid;
        if ((pid = fork()) > 0) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    int fd_dp;
                    char * serverResponse = (char *) malloc(sizeof(char) * BUF_SIZE);
                    if ((fd_dp = open(file_name, O_RDONLY)) < 3) {
                        printf("Error, unable to read server dump.\n");
                        serverResponse = "Unable to read server_dump, umask or permission error.";
                    }
                    char * buf_read = (char*) malloc(sizeof(char) * BUF_SIZE);
                    int rn = read(fd_dp, buf_read, BUF_SIZE);
                    if (rn > 0 ) {
                        //printf("Files found \n%s\n", buf_read);
                        char* tok = strtok(buf_read, "\n");
                        //printf("Filest %s\n", tok);

                        struct stat *file_stat = (struct stat*) malloc(sizeof(struct stat));
                        if (stat(tok, file_stat) == 0) {
                            //printf("File Name: %s\n", argv[1]);
                            //printf("File Size: %d\n", file_stat->st_size);
                            //printf("File Date: %s\n", ctime(&file_stat->st_ctime));
                            strcpy(serverResponse, argv[1]);
                            strcat(serverResponse, " : ");
                            char s_size[10];
                            sprintf(s_size, "%d", file_stat->st_size);
                            strcat(serverResponse, s_size);
                            strcat(serverResponse, " bytes : ");
                            strcat(serverResponse, ctime(&file_stat->st_ctime));
                            serverResponse[strlen(serverResponse) - 1] = '\0';
                        } else {
                            printf("Error, while accessing file stat.\n");
                            serverResponse = "Error, while accessing file stat.";
                        }
                    } else {
                        // no file found
                        //printf("No File found\n");
                        serverResponse = "File not found";
                    }
                    write(*csd, serverResponse, strlen(serverResponse));
                    close(fd_dp);
                    unlink(file_name);
                    return 1;
                } else {
                    printf("Error while execvp command.\n");
                    return -1;
                }
            }
        } else if (pid == 0) {
            char * a_cmd = "find";
            int fd_dp = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0777);
            int ret = dup2(fd_dp, STDOUT_FILENO);
            char * argv_cmd[] = {a_cmd, getenv("HOME"), "-name", argv[1], "-type", "f", NULL};
            int n = execvp(a_cmd, argv_cmd);
            if ( n < 0 ) {
                printf("Error , Issues in execution of execvp");
                return -1;
            }
        } else {
            printf("Error in Forking Process.\n");
        }
    } else if (strcmp(argv[0], CMD_SIZE) == 0) {
        // adjust size 1 based on the requirment
        char * size1 = (char *)  malloc(sizeof(char) * strlen(argv[1]) + 2);
        long size_s1 = strtol(argv[1], &argv[1], 10);
        size_s1 = size_s1 - 1;
        sprintf(size1, "+%ldc", size_s1);

        // adjust size 2 based on the requirements
        char * size2 = (char *)  malloc(sizeof(char) * strlen(argv[1]) + 2);
        long size_s2 = strtol(argv[2], &argv[2], 10);
        size_s2 = size_s2 + 1;
        sprintf(size2, "-%ldc", size_s2);

        //printf("Size 1: %s, Size 2: %s\n", size1, size2);

        char * file_name = getfileName("size");
        char * tar_name = gettarName("size");
        int pids;
        if ((pids = fork()) > 0) {
            int status;
            waitpid(pids, &status, 0);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    int pidt;
                    if ((pidt = fork()) > 0) {
                        int stat;
                        waitpid(pidt, &stat, 0);
                        if (WIFEXITED(stat)) {
                            if (WEXITSTATUS(stat) == 0) {
                                // Open tar file
                                int fd_t = open(tar_name, O_RDONLY);
                                if (fd_t < 3) {
                                    printf("Error, tar not found.\n");
                                    return -1;
                                }

                                // Get file size
                                long file_size = lseek(fd_t, 0, SEEK_END);
                                printf("Tar Size : %ld\n", file_size);
                                lseek(fd_t, 0, SEEK_SET);

                                // Send size data to client
                                char * size_buf = (char *) malloc(sizeof(char) * 20);
                                sprintf(size_buf, "%ld", file_size);
                                write(*csd, size_buf, 20);

                                // read and send to client
                                int block = 256;
                                int extra = (file_size % block != 0) ? 1 : 0;
                                char * r_buf = (char *) malloc(sizeof(char) * block);
                                int iter = (file_size / block) + extra;
                                //printf("ITER : %d %d\n", iter, extra);
                                for (int i = 0 ; i < iter; i++) {
                                    int rn = read(fd_t, r_buf, block);
                                    if (rn <= 0) {
                                        printf("Error, While reading Tar %d\n", rn);
                                    }
                                    write(*csd, r_buf, rn);
                                }
                                close(fd_t);
                                unlink(file_name);
                                unlink(tar_name);
                            }
                        }
                    } else if (pidt == 0) {
                        tar_files_from_file(file_name, tar_name);
                    }
                }
            }

        } else if (pids == 0) {
            char * cmd = "find";
            char * cmd_arg[] = {cmd, getenv("HOME"), "-size", size1, "-size", size2, "-type", "f", "-print0", NULL};
            execute_find_to_file(cmd, cmd_arg, file_name);
        } else {
            printf("Error, while forking procees.\n");
        }
    } else if (strcmp(argv[0], CMD_DATE) == 0) { 
        // struct tm *time_1 = malloc(sizeof(struct tm));
        // memset(time_1, 0, sizeof(struct tm));
        // strptime(argv[1], "%Y-%m-%d", time_1);
        // time_t t1 = mktime(time_1) - (60 * 60 * 24);
        //char * date1 = (char*) malloc(sizeof(char) * 20);
        char * date1 = argv[1];
        // strftime(date1, 20, "%Y-%m-%d", localtime(&t1));
        // printf("date 1 : %s\n", date1);
        
        struct tm *time_2 = malloc(sizeof(struct tm));
        memset(time_2, 0, sizeof(struct tm));
        strptime(argv[2], "%Y-%m-%d", time_2);
        time_t t2 = mktime(time_2) + (60 * 60 * 24);
        char * date2 = (char*) malloc(sizeof(char) * 20);
        strftime(date2, 20, "%Y-%m-%d", localtime(&t2));
        printf("date 2 : %s\n", date2);

        char * file_name = getfileName("date");
        char * tar_name = gettarName("date");
        int pids;
        if ((pids = fork()) > 0) {
            int status;
            waitpid(pids, &status, 0);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    int pidt;
                    if ((pidt = fork()) > 0) {
                        int stat;
                        waitpid(pidt, &stat, 0);
                        if (WIFEXITED(stat)) {
                            if (WEXITSTATUS(stat) == 0) {
                                // Open tar file
                                int fd_t = open(tar_name, O_RDONLY);
                                if (fd_t < 3) {
                                    printf("Error, tar not found.\n");
                                    return -1;
                                }

                                // Get file size
                                long file_size = lseek(fd_t, 0, SEEK_END);
                                printf("Tar Size : %ld\n", file_size);
                                lseek(fd_t, 0, SEEK_SET);

                                // Send size data to client
                                char * size_buf = (char *) malloc(sizeof(char) * 20);
                                sprintf(size_buf, "%ld", file_size);
                                write(*csd, size_buf, 20);

                                // read and send to client
                                int block = 256;
                                int extra = (file_size % block != 0) ? 1 : 0;
                                char * r_buf = (char *) malloc(sizeof(char) * block);
                                int iter = (file_size / block) + extra;
                                //printf("ITER : %d %d\n", iter, extra);
                                for (int i = 0 ; i < iter; i++) {
                                    int rn = read(fd_t, r_buf, block);
                                    if (rn <= 0) {
                                        printf("Error, While reading Tar %d\n", rn);
                                    }
                                    write(*csd, r_buf, rn);
                                }
                                close(fd_t);
                                unlink(file_name);
                                unlink(tar_name);
                            }
                        }
                    } else if (pidt == 0) {
                        tar_files_from_file(file_name, tar_name);
                    }
                }
            }

        } else if (pids == 0) {
            char * cmd = "find";
            char * cmd_arg[] = {cmd, getenv("HOME"), "-newerct", date1, "!" ,"-newerct", date2, "-type", "f", "-print0", NULL};
            execute_find_to_file(cmd, cmd_arg, file_name);
        } else {
            printf("Error, while forking procees.\n");
        }

    } else if (strcmp(argv[0], CMD_FILES) == 0) { 

        char * file_name = getfileName("files");
        char * tar_name = gettarName("files");
        int pids;
        if ((pids = fork()) > 0) {
            int status;
            waitpid(pids, &status, 0);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    // Verify that files were found
                    if (check_files_size_for_tar(file_name) == 0) {
                        printf("No files found.\n");
                        char * msg_buf = "No file found";
                        write(*csd, "", 20);
                        write(*csd, msg_buf, 13);
                        return 1;
                    }
                    int pidt;
                    if ((pidt = fork()) > 0) {
                        int stat;
                        waitpid(pidt, &stat, 0);
                        if (WIFEXITED(stat)) {
                            if (WEXITSTATUS(stat) == 0) {
                                // Open tar file
                                int fd_t = open(tar_name, O_RDONLY);
                                if (fd_t < 3) {
                                    printf("Error, tar not found.\n");
                                    return -1;
                                }

                                // Get file size
                                long file_size = lseek(fd_t, 0, SEEK_END);
                                printf("Tar Size : %ld\n", file_size);
                                lseek(fd_t, 0, SEEK_SET);

                                // Send size data to client
                                char * size_buf = (char *) malloc(sizeof(char) * 20);
                                sprintf(size_buf, "%ld", file_size);
                                write(*csd, size_buf, 20);

                                

                                // read and send to client
                                int block = 256;
                                int extra = (file_size % block != 0) ? 1 : 0;
                                char * r_buf = (char *) malloc(sizeof(char) * block);
                                int iter = (file_size / block) + extra;
                                //printf("ITER : %d %d\n", iter, extra);
                                for (int i = 0 ; i < iter; i++) {
                                    int rn = read(fd_t, r_buf, block);
                                    if (rn <= 0) {
                                        printf("Error, While reading Tar %d\n", rn);
                                    }
                                    write(*csd, r_buf, rn);
                                }
                                close(fd_t);
                                unlink(file_name);
                                unlink(tar_name);
                            }
                        }
                    } else if (pidt == 0) {
                        tar_files_from_file(file_name, tar_name);
                    }
                }
            }

        } else if (pids == 0) {
            char * cmd = "find";
            char ** cmd_arg = (char**) malloc(sizeof(char) * BUF_SIZE);
            cmd_arg[0] = cmd;
            cmd_arg[1] = getenv("HOME");
            cmd_arg[2] = "-type";
            cmd_arg[3] = "f";
            cmd_arg[4] = "(";
            int count = 5;
            int guard = S_COUNT;
            if (S_COUNT > 7) {
                guard = 7;
            }
            for (int i = 1 ; i < guard; i++) {
                cmd_arg[count] = "-name";
                count++;
                cmd_arg[count] = argv[i];
                count++;
                if (i + 1 < guard) {
                    cmd_arg[count] = "-o";
                    count++;
                }
            }
            cmd_arg[count] = ")";
            count++;
            cmd_arg[count] = "-print0";
            count++;
            cmd_arg[count] = NULL;
            // for (int i = 0; i < count; i++) {
            //     printf("X -> %s\n", cmd_arg[i]);
            // }
            //char * cmd_arg[] = {cmd, getenv("HOME"),"(", "-name", argv[1], "-o", "-name", argv[2], ")", "-type", "f", "-print0", NULL};
            execute_find_to_file(cmd, cmd_arg, file_name);
        } else {
            printf("Error, while forking procees.\n");
        }
    } else if (strcmp(argv[0], CMD_TAR) == 0) {
        char * file_name = getfileName("tar");
        char * tar_name = gettarName("tar");
        int pids;
        if ((pids = fork()) > 0) {
            int status;
            waitpid(pids, &status, 0);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    // Verify that files were found
                    if (check_files_size_for_tar(file_name) == 0) {
                        printf("No files found.\n");
                        char * msg_buf = "No file found";
                        write(*csd, "", 20);
                        write(*csd, msg_buf, 13);
                        return 1;
                    }
                    
                    int pidt;
                    if ((pidt = fork()) > 0) {
                        int stat;
                        waitpid(pidt, &stat, 0);
                        if (WIFEXITED(stat)) {
                            if (WEXITSTATUS(stat) == 0) {
                                // Open tar file
                                int fd_t = open(tar_name, O_RDONLY);
                                if (fd_t < 3) {
                                    printf("Error, tar not found.\n");
                                    return -1;
                                }

                                // Get file size
                                long file_size = lseek(fd_t, 0, SEEK_END);
                                printf("Tar Size : %ld\n", file_size);
                                lseek(fd_t, 0, SEEK_SET);

                                // Send size data to client
                                char * size_buf = (char *) malloc(sizeof(char) * 20);
                                sprintf(size_buf, "%ld", file_size);
                                write(*csd, size_buf, 20);

                                // read and send to client
                                int block = 256;
                                int extra = (file_size % block != 0) ? 1 : 0;
                                char * r_buf = (char *) malloc(sizeof(char) * block);
                                int iter = (file_size / block) + extra;
                                //printf("ITER : %d %d\n", iter, extra);
                                for (int i = 0 ; i < iter; i++) {
                                    int rn = read(fd_t, r_buf, block);
                                    if (rn <= 0) {
                                        printf("Error, While reading Tar %d\n", rn);
                                    }
                                    write(*csd, r_buf, rn);
                                }
                                close(fd_t);
                                unlink(file_name);
                                unlink(tar_name);
                            }
                        }
                    } else if (pidt == 0) {
                        tar_files_from_file(file_name, tar_name);
                    }
                }
            }

        } else if (pids == 0) {
            char * cmd = "find";
            char ** cmd_arg = (char**) malloc(sizeof(char) * BUF_SIZE);
            cmd_arg[0] = cmd;
            cmd_arg[1] = getenv("HOME");
            cmd_arg[2] = "-type";
            cmd_arg[3] = "f";
            cmd_arg[4] = "(";
            int count = 5;
            int guard = S_COUNT;
            if (S_COUNT > 7) {
                guard = 7;
            }
            for (int i = 1 ; i < guard; i++) {
                cmd_arg[count] = "-name";
                count++;
                //printf("u %s\n", cmd_arg[count]);
                char * bb = (char*) malloc(sizeof(char) * 5);
                sprintf(bb, "*.%s", argv[i]);
                cmd_arg[count] = bb;
                count++;
                if (i + 1 < guard) {
                    cmd_arg[count] = "-o";
                    count++;
                }
            }
            cmd_arg[count] = ")";
            count++;
            cmd_arg[count] = "-print0";
            count++;
            cmd_arg[count] = NULL;
            // for (int i = 0; i < count; i++) {
            //     printf("X -> %s\n", cmd_arg[i]);
            // }
            //char * cmd_arg[] = {cmd, getenv("HOME"),"(", "-name", argv[1], "-o", "-name", argv[2], ")", "-type", "f", "-print0", NULL};
            execute_find_to_file(cmd, cmd_arg, file_name);
        } else {
            printf("Error, while forking procees.\n");
        }
    }
    return 1;
}

void processclient(int *csd) {
    while (1) {
        char *buf_res = (char *) malloc(sizeof(char) * BUF_SIZE);
        memset(buf_res, '\0', BUF_SIZE);
        int valread = read(*csd, buf_res, BUF_SIZE);
        printf("CLIENT %02d: %s\n", REQUEST_COUNT, buf_res);
        int pc = 0;
        if ((pc = process_cmd(buf_res, csd)) == 0) { // server sent quit
            return;
        } else if (pc == 1) {
            continue;
        }
        char * serverResponse = "Command Processed .....";
        write(*csd, serverResponse, strlen(serverResponse));
    }
    close(*csd);
}

int main(void) {
    // Init Server Object
    struct sockaddr_in sp;
    memset(&sp, 0, sizeof(sp));
    sp.sin_family = AF_INET;
    sp.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &sp.sin_addr) < 1) {
        perror("Inet_pton :");
        exit(EXIT_FAILURE);
    }

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        printf("Error while initiating Socket....\n");
        perror("Socket :");
        return EXIT_FAILURE;
    }
    printf("Socket ID - %d\n", sd);

    // Bind the socket
    
    if (bind(sd, (struct sockaddr *) &sp, sizeof(struct sockaddr_in)) == -1) {
        perror("Bind :");
        return EXIT_FAILURE;
    }
    printf("Server Started at : %s:%d\n", inet_ntoa(sp.sin_addr), ntohs(sp.sin_port));

    if (listen(sd, 5) == -1) {
        printf("Error while listening ....\n");
    }

    REQUEST_COUNT = 0;
    int Q = 1;
    while (Q) {
        int csd;
        if ((csd = accept(sd, (struct sockaddr *)NULL, NULL)) < 0) {
            perror("Accept : ");
            return EXIT_FAILURE;
        }
        REQUEST_COUNT ++;
        char * ack_response = (char *) malloc(sizeof(char) * 7);
        if (REQUEST_COUNT <= 4 || ((REQUEST_COUNT > 8) && (REQUEST_COUNT % 2 == 1))) {
            printf("CLIENT %02d: Connected\n", REQUEST_COUNT);
            strcpy(ack_response, ACK_APT);
            write(csd, ack_response, 6);
        } else {
            printf("CLIENT %02d: Refered to Mirror\n", REQUEST_COUNT);
            strcpy(ack_response, ACK_RJT);
            write(csd, ack_response, 6);
            continue;
        }
        if (!fork()) {
            // Handle child request here
            processclient(&csd);
            Q = 0;
        }
    }
    printf("CLIENT %02d: Disconnected\n", REQUEST_COUNT);
    return 0;
}