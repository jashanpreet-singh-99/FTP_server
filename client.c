#define _XOPEN_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<time.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/wait.h>

#define PREFIX "C$ "
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8099
#define MIRROR_PORT 8100

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

int IS_TAR = 0;

// dgetfiles 2003-03-01 2023-04-21

struct tm *validateDate(char * arg) {
    struct tm *time = malloc(sizeof(struct tm));
    strptime(arg, "%Y-%m-%d", time);
    //printf(" %d - %d - %d\n", time->tm_year, time->tm_mon, time->tm_mday);
    if (time->tm_year > 0) {
        if (time->tm_mon >= 0 && time->tm_mon < 12) {
            if ((time->tm_mday >= 1 && time->tm_mday <= 31) && (time->tm_mon == 0 || time->tm_mon == 2 || time->tm_mon == 4 || time->tm_mon == 6 || time->tm_mon == 7 || time->tm_mon == 9 || time->tm_mon == 11)) {
                return time;
            } else if ((time->tm_mday >= 1 && time->tm_mday <= 30) && (time->tm_mon == 3 || time->tm_mon == 5 || time->tm_mon == 8 || time->tm_mon == 10)) {
                return time;
            } else if ((time->tm_mday >= 1 && time->tm_mday <= 28) && time->tm_mon == 1) {
                return time;
            } else if (time->tm_mday == 29 && time->tm_mon == 1 && (time->tm_year % 4 == 0)) {
                return time;
            } else {
                printf("Sorry, Day is not valid.\n");
                return NULL;
            }
        } else {
            printf("Sorry, Invalid Month entered.\n");
            return NULL;
        }
    } else {
        printf("Sorry, Minimum date year allowed is 1901.\n");
        return NULL;
    }
    return NULL;
}

long getTime(struct tm * time) {
    return (365 * time->tm_year) + (31 * time->tm_mon) + time->tm_mday;
}

char* split_command(char * command, char ** argv, int * S_COUNT) {
    char* tmp = (char*) malloc(sizeof(char) * (strlen(command) + 2));
    memset(tmp, '\0', sizeof(char) * (strlen(command) + 2));
    strcpy(tmp, command);
    argv[*S_COUNT] = tmp;
    //argv[*S_COUNT] = strcpy(tmp, command);
    //printf("S %s %d\n", argv[*S_COUNT], *S_COUNT);
    command[0] = '\0';
    *S_COUNT = *S_COUNT + 1;
    return command; 
}

char *convert_to_string(char ** argv_n, int *N_COUNT) {
    char * strg = (char*) malloc(sizeof(argv_n) + (20 * sizeof(char)));
    memset(strg, '\0', sizeof(argv_n) + (20 * sizeof(char)));
    strcpy(strg, argv_n[0]);
    for (int i = 1 ; i < *N_COUNT; i++) {
        strcat(strg, " ");
        strcat(strg, argv_n[i]);
    }
    return strg;
}

int validateCommand(char * stmt, int * UNZIP, int *sd) {
    *UNZIP = 0;
    IS_TAR = 0;
    // if blank string passed
    if (strlen(stmt) < 1) {
        return -1;
    }

    // Quit string
    if (strcmp(stmt, CMD_QUIT) == 0) {
        return 0;
    }
    //printf("STMT : %s\n", stmt);
    int count = 0;
    int S_COUNT = 0;
    char * command = (char*) malloc(sizeof(char) * strlen(stmt));
    memset(command, '\0', sizeof(char) * strlen(stmt));
    char **argv = (char**) malloc(sizeof(char) * (strlen(stmt) + 40));
    memset(argv, '\0', sizeof(char) * (strlen(stmt)+40));
    int ccount = 0;
    while (stmt[count] != '\0') {
        switch (stmt[count]) {
            case ' ':
                command = split_command(command, argv, &S_COUNT);
                ccount = 0;
                break;
            default:
                //printf("%s\n", command);
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
    //printf("CMD : %s\n", argv[0]);
    if (strcmp(argv[0], CMD_FIND) == 0) {
        if (S_COUNT < 2 || S_COUNT > 2) {
            printf("Error, usage error %s\n", CMD_FIND);
            return -1;
        }
        // cmd find
        return 1;
    } else if (strcmp(argv[0], CMD_SIZE) == 0) {
        if (S_COUNT < 3 || S_COUNT > 4) {
            printf("Error, usage error %s\n", CMD_SIZE);
            return -1;
        } else if (S_COUNT == 4) {
            if (strcmp(argv[3], CMD_U) != 0) {
                printf("Error, usage error, options %s\n", CMD_SIZE);
                return -1;
            }
            for (int i = 1; i < S_COUNT-1; i++) {
                if (strcmp(argv[i], CMD_U) == 0) {
                    printf("Error, usage eror, -u should be the last argument.\n");
                    return -1;
                }
            }
        }
        if (strcmp(argv[S_COUNT-1], CMD_U) == 0) {
            *UNZIP = 1;
        }
        // set flag to unzip

        // size check
        char *ptr = argv[1];
        long size1 = strtol(ptr, &ptr, 10);
        if (*ptr != '\0') {
            printf("Size1 should be an integer.\n");
            return -1;
        }
        if (size1 < 0) {
            printf("Size 1 -ve not possible.\n");
            return -1;
        }
        ptr = argv[2];
        long size2 = strtol(ptr, &ptr, 10);
        if (*ptr != '\0') {
            printf("Size2 should be an integer.\n");
            return -1;
        }
        if (size2 < 0) {
            printf("Size 2 -ve not possible.\n");
            return -1;
        }
        if (size1 > size2) {
            printf("Error, size range. size1 <= 1 100 size2.\n");
            return -1;
        }
        return 2;
    } else if (strcmp(argv[0], CMD_DATE) == 0) {
        if (S_COUNT < 3 || S_COUNT > 4) {
            printf("Error, usage error %s\n", CMD_DATE);
            return -1;
        } else if (S_COUNT == 4) {
            if (strcmp(argv[3], CMD_U) != 0) {
                printf("Invalid uage, -u should be the last parameter.\n");
                return -1;
            }
            for (int i = 1; i < S_COUNT-1; i++) {
                if (strcmp(argv[i], CMD_U) == 0) {
                    printf("Error, usage eror, -u should be the last argument.\n");
                    return -1;
                }
            }
        }
        if (strcmp(argv[S_COUNT-1], CMD_U) == 0) {
            *UNZIP = 1;
        }
        // cmd date
        struct tm *t1, *t2;
        if ((t1 = validateDate(argv[1])) == NULL) {
            return -1;
        }
        if ((t2 = validateDate(argv[2])) == NULL) {
            return -1;
        }
        long timestamp_t1 = getTime(t1);
        long timestamp_t2 = getTime(t2);
        long diff = timestamp_t2 - timestamp_t1;
        if (diff < 0) {
            printf("Sorry, date1 should be greater than or equal to date 2.\n");
            return -1;
        }
        //printf("Time diff %ld \n", diff);
        return 2;
    } else if (strcmp(argv[0], CMD_FILES) == 0) {
        if (S_COUNT < 2 || S_COUNT > 8) {
            printf("Error, usage error %s\n", CMD_FILES);
            return -1;
        } else if (S_COUNT == 2) {
            if (strcmp(argv[1], CMD_U) == 0) {
                printf("Invalid uage, -u should be the last option.\n");
                return -1;
            }
        } else if (S_COUNT == 8) {
            if (strcmp(argv[7], CMD_U) != 0) {
                printf("Invalid uage, -u should be the last option.\n");
                return -1;
            }
            for (int i = 1; i < S_COUNT-1; i++) {
                if (strcmp(argv[i], CMD_U) == 0) {
                    printf("Error, usage eror, -u should be the last argument.\n");
                    return -1;
                }
            }
        }
        if (strcmp(argv[S_COUNT-1], CMD_U) == 0) {
            *UNZIP = 1;
        }
        return 2;
    } else if (strcmp(argv[0], CMD_TAR) == 0) {
        // if (S_COUNT < 2 || S_COUNT > 8) {
        //     printf("Error, usage error %s\n", CMD_TAR);
        //     return -1;
        // } else if (S_COUNT == 8) {
        //     if (strcmp(argv[7], CMD_U) != 0) {
        //         printf("Invalid uage, -u should be the last option.\n");
        //         return -1;
        //     }
        //     for (int i = 1; i < S_COUNT-1; i++) {
        //         if (strcmp(argv[i], CMD_U) == 0) {
        //             printf("Error, usage eror, -u should be the last argument.\n");
        //             return -1;
        //         }
        //     }
        // }
        if (S_COUNT < 2) { // atleast 1 extension
            printf("Error, usage error %s\n", CMD_TAR);
            return -1;
        } else if (S_COUNT == 2) {
            if (strcmp(argv[1], CMD_U) == 0) { // atleast 1 extension before -u
                printf("Invalid uage, need atleast one extension before -u.\n");
                return -1;
            }
        }
        // verify -u is not between any extension names
        for (int i = 1; i < S_COUNT-1; i++) {
            if (strcmp(argv[i], CMD_U) == 0) {
                printf("Error, usage eror, -u should be the last argument.\n");
                return -1;
            }
        }

        char **argv_n = (char**) malloc(sizeof(char) * (strlen(stmt) + 40));
        memset(argv_n, '\0', sizeof(char) * (strlen(stmt)+40));
        int N_COUNT = 2;
        argv_n[0] = argv[0];
        argv_n[1] = argv[1];
        for (int i = 2; i < S_COUNT; i++) {
            //printf("C %s\n", argv[i]);
            int match = 0;
            for (int j = 0 ; j < N_COUNT; j++) {
                if (strcmp(argv[i], argv_n[j]) == 0) {
                    match = 1;
                    break;
                }
            }
            if (match == 0) {
                argv_n[N_COUNT] = argv[i];
                N_COUNT++;
            }
        }
        //printf("CMD NEW : %s\n", convert_to_string(argv_n, &N_COUNT));
        if (N_COUNT > 8) {
            printf("Error, usage error %s\n", CMD_TAR);
            return -1;
        } else if (N_COUNT == 8) {
            if (strcmp(argv_n[7], CMD_U) != 0) { // atleast 1 extension before -u
                printf("Invalid uage, too manu arguments.\n");
                return -1;
            }
        }
        IS_TAR = 1;
        stmt = convert_to_string(argv_n, &N_COUNT);
        //printf("CMD NEW : %s\n", stmt);
        if (strcmp(argv_n[N_COUNT-1], CMD_U) == 0) {
            *UNZIP = 1;
        }
        if (send(*sd, stmt, strlen(stmt), 0) < 0) {
            perror("Send :");
            exit(EXIT_FAILURE);
        }
        // cmd tar
        return 2;
    } 
    return -1;
}

int main(void) {
    // Init Server Object
    struct sockaddr_in sp, spm;
    memset(&sp, 0, sizeof(sp));
    sp.sin_family = AF_INET;
    sp.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &sp.sin_addr) < 1) {
        perror("Inet_pton :");
        exit(EXIT_FAILURE);
    }

    memset(&spm, 0, sizeof(spm));
    spm.sin_family = AF_INET;
    spm.sin_port = htons(MIRROR_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &spm.sin_addr) < 1) {
        perror("Inet_pton :");
        exit(EXIT_FAILURE);
    }

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    int sd2 = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        printf("Error while initiating Server Socket....\n");
        perror("SEVER Socket :");
        return EXIT_FAILURE;
    }
    if (sd2 < 0) {
        printf("Error while initiating Mirror Socket....\n");
        perror("MIRROR Socket :");
        return EXIT_FAILURE;
    }
    //printf("Socket ID - %d\n", sd);

    // Connect to server
    if (connect(sd, (struct sockaddr *) &sp, sizeof(struct sockaddr_in))) {
        perror("SERVER Connect :");
        return EXIT_FAILURE;
    }

    char * ack_buf = (char*) malloc(sizeof(char) * 6);
    if (read(sd, ack_buf, 6) < 0) {
        perror("SERVER Read :");
        return EXIT_FAILURE;
    }
    printf("SERVER : Connection %s\n", ack_buf);
    if (strcmp(ack_buf, ACK_RJT) == 0) {
        if (connect(sd2, (struct sockaddr *) &spm, sizeof(struct sockaddr_in))) {
            perror("MIRROR Connect :");
            return EXIT_FAILURE;
        }
        sd = sd2;
        printf("MIRROR : Connection ACCEPT\n");
        // connect to mirror
    } else if (strcmp(ack_buf, ACK_APT) != 0) {
        printf("Gargage Value received from server, plz restart the server.\n");
        return EXIT_FAILURE;
    }

    char *statement = (char*) malloc(BUF_SIZE * sizeof(char));
    int UNZIP = 0;
    while (1) {
        // User Input
        printf("%s", PREFIX);
        fgets(statement, BUF_SIZE, stdin);
        statement[strlen(statement) - 1] = '\0';

        char * buff = (char*) malloc(sizeof(char) * BUF_SIZE);
        int vc = -1;
        switch (vc = validateCommand(statement, &UNZIP, &sd)) {
            case 0: // quit command
            // Read Response from server
                         // Send cmd to server
                if (send(sd, statement, strlen(statement), 0) < 0) {
                    perror("Send :");
                    exit(EXIT_FAILURE);
                }
                if (read(sd, buff, BUF_SIZE) < 0) {
                    perror("Read :");
                    return EXIT_FAILURE;
                }
                printf("SERVER : %s\n", buff);
                close(sd);
                return EXIT_SUCCESS;
            case 1: // valid command
                // Send cmd to server
                if (send(sd, statement, strlen(statement), 0) < 0) {
                    perror("Send :");
                    exit(EXIT_FAILURE);
                }
                if (read(sd, buff, BUF_SIZE) < 0) {
                    perror("Read :");
                    return EXIT_FAILURE;
                }
                printf("SERVER : %s\n", buff);
                break;
            case 2: // tar command
                if (IS_TAR != 1) {
                    if (send(sd, statement, strlen(statement), 0) < 0) {
                        perror("Send :");
                        exit(EXIT_FAILURE);
                    }
                }
               
                char * size_buf = (char *) malloc(sizeof(char) * 20);
                if (read(sd, size_buf, 20) < 0) {
                    perror("Read :");
                    return EXIT_FAILURE;
                }
                long file_size = strtol(size_buf, &size_buf, 10);
                if (*size_buf != '\0') {
                    printf("Size of Tar, should be an integer.\n");
                    return -1;
                }
                printf("SERVER : Tar Size %d\n", file_size);
                char * tar_file = "temp.tar.gz";

                if (file_size < 1) {
                    char * msg_buf = (char *) malloc(sizeof(char) * 14);
                    int rn = read(sd, msg_buf, 13);
                    if (rn <= 0) {
                        printf("Error, While reading server Tar %d\n", rn);
                    } else {
                        printf("SERVER: %s\n", msg_buf);
                    }
                } else {
                    int fd_t = open(tar_file, O_CREAT | O_RDWR | O_TRUNC, 0777);
                    if (fd_t < 0) {
                        printf("Unable to create Tar from server.\n");
                    }


                    int block = 256;
                    int extra = (file_size % block != 0) ? 1 : 0;
                    char * r_buf = (char *) malloc(sizeof(char) * block);
                    int iter = (file_size / block) + extra;
                    //printf("ITER : %d %d\n", iter, extra);
                    for (int i = 0 ; i < iter; i++) {
                        int rn = read(sd, r_buf, block);
                        if (rn <= 0) {
                            printf("Error, While reading server Tar %d\n", rn);
                        }
                        write(fd_t, r_buf, rn);
                    }
                    close(fd_t);
                    if (UNZIP == 1) {
                        //printf("Unzip requested\n");
                        int pidu;
                        if ((pidu = fork()) > 0) {
                            int stat;
                            waitpid(pidu, &stat, 0);
                            if (WIFEXITED(stat)) {
                                if (WEXITSTATUS(stat) == 0) {
                                    UNZIP = 0;
                                    unlink(tar_file);
                                }
                            }
                        } else if (pidu == 0) {
                            char * cmd = "tar";
                            char * cmd_arg[] = {cmd, "-xzf", tar_file, NULL};
                            //printf("%s\n", cmd_arg[2]);
                            int n = execvp(cmd, cmd_arg);
                            if ( n < 0 ) {
                                printf("Error , Issues in execution of execvp TAR.\n");
                            }
                        } else {
                            printf("Error, while forking procees.\n");
                        }
                    }
                }
                break;
            default:
                printf("Error in command validation %d\n", vc);
                break;
        }

    }

    return 0;
}