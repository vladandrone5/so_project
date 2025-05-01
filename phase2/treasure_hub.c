#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#define SIZE 100
#define NAME_SIZE 100
#define LINE_SIZE 1000

pid_t monitor_pid = -1;
int running_code = 0;
char commands[] = "commands.txt"; // file care memoreaza ultima comanda

typedef struct {
    char treasure_id[10];
    char username[NAME_SIZE + 1];
    double latitude, longitude;
    char clue[LINE_SIZE + 1];
    int value;
}treasure;

//Functii de implementat

void start(); // porneste monitorul
void create_loop(); // creeaza loop pt monitor
void manage_signals(int signal);
void list_hunts();
void list_treasures(char *hunt_id);
void stop();
void manage_child_sig(int signal);


void manage_signals(int signal) {
    if(signal == SIGUSR1) {
        FILE *f = NULL;
        if((f = fopen(commands, "r")) == 0) {
            perror("error opening command file\n");
            exit(-1);
        }

        char command[SIZE], hunt_id[SIZE], treasure_id[SIZE];

        //system("gcc -Wall -o treasure_manager new_treasure_manager.c");

        if(fscanf(f, "%s%s%s", command, hunt_id, treasure_id) != 3) {
            perror("error reading content\n");
            exit(-1);
        }

        if(fclose(f) != 0) {
            perror("error closing file\n");
            exit(-1);
        }

        if(strcmp(command, "list_hunts") == 0) {
            //system("ls -d */ | cut -f1 -d'/'");
            DIR *dir = opendir("./");
            if(dir == NULL) {
                perror("error opening directory\n");
                exit(-1);
            }
            
            struct dirent *data;

            while((data = readdir(dir)) != NULL) {
                if(data->d_type == DT_DIR) {
                    char op[256];
                    strcpy(op, "");
                    snprintf(op, sizeof(op), "./treasure_manager --count_treasures %s", data->d_name);
                    system(op);
                }
            }

            closedir(dir);
        }
        else if(strcmp(command, "list_treasures") == 0) {
            char op[256];
            snprintf(op, sizeof(op), "./treasure_manager --list %s ceva", hunt_id);
            system(op);
        }
        else if(strcmp(command, "view_treasure") == 0) {
            char op[256];
            snprintf(op, sizeof(op), "./treasure_manager --view_treasure %s %s", hunt_id, treasure_id);
            system(op);
        }
        else if(signal == SIGTERM) {
            usleep(500000);
            exit(0);
        }

    }
}

void create_loop() {
    struct sigaction sg;
    sg.sa_flags = 0;
    sg.sa_handler = manage_signals; // seteaza functia de handle pe manage_signals
    sigemptyset(&sg.sa_mask); // ce semnale sunt blocate cat ruleaza manage_signals
    sg.sa_flags = 0; // default
    sigaction(SIGUSR1, &sg, NULL); // actiunea in SIGUSR1
    sigaction(SIGUSR2, &sg, NULL); // in SIGUSR2
    sigaction(SIGTERM, &sg, NULL);

    while(1) { // am creat pauza pt a astepta comenzi
        pause();
    }

    return;
}

void start() { // porneste monitorul
    if(running_code != 0) { // verif daca monitorul este deja deschis
        perror("monitor already runs\n");
        exit(-1);
    }

    pid_t current_pid = fork(); // creaza un child process in background
    if(current_pid < 0) {
        perror("error fork command\n");
        exit(-1);
    }

    else if(current_pid == 0) { 
        create_loop();
        exit(0);
    }

    monitor_pid = current_pid;
    running_code = 1;

    printf("monitor started with PID %d\n", monitor_pid);

    return;
}

void stop() {
    if(running_code == 0) {
        perror("no monitor is running\n");
        exit(-1);
    }

    kill(monitor_pid, SIGTERM);
    running_code = 0;
    printf("monitor is closing\n");
    
    return;
}

void view_treasure(char *hunt_id, char *treasure_id) {
    if(running_code == 0) {
        printf("no monitor running\n");
        return;
    }

    char op[256];
    snprintf(op, sizeof(op), "./treasure_manager --view_treasure %s %s", hunt_id, treasure_id);


    int command = system("gcc -Wall -o treasure_manager new_treasure_manager.c");
    if(command == -1) {
       perror("error compiling file\n");
        exit(-1);
    }

    FILE *f = fopen(commands, "w");
    fprintf(f, "view_treasure %s %s\n", hunt_id, treasure_id);
    fclose(f);

    if(system(op) == -1) {
        perror("error for \"view_treasure\" function");
        exit(-1);
    }
    

    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }

    return;
}

void list_hunts() { // voi merge pe denumirea standard "Hunt..."
    if(running_code == 0) {
        perror("no monitor running\n");
        exit(-1);
    }

    FILE *f = fopen(commands, "w");
    fprintf(f, "list_hunts <> <>\n");
    fclose(f);


    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }
}

void list_treasures(char *hunt_id) {
    if(running_code == 0) {
        perror("no monitor running\n");
        exit(-1);
    }

    FILE *f = fopen(commands, "w");
    if(f == NULL) {
        perror("error open command file\n");
        exit(-1);
    }

    fprintf(f, "list_treasures %s", hunt_id);

    fclose(f);

    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }
}

void manage_child_sig(int signal) {
    int status;
    wait(&status);
    printf("Monitor process terminated. Status: %d\n", WEXITSTATUS(status));

    return;
}

int main(void) {
    char command[256];

    
    struct sigaction sg;
    sg.sa_handler = manage_child_sig;
    sigemptyset(&sg.sa_mask);
    sg.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sg, NULL);
    

    
    while(1) {
        //printf("> ");
        if(fgets(command, sizeof(command), stdin) == NULL) break;
        command[strcspn(command, "\n")] = 0;

        if(strcmp(command, "start") == 0) {
            start();
        }
        else if(strcmp(command, "stop") == 0) {
            stop();
        }
        else if(strcmp(command, "exit") == 0) {
            if(running_code != 0) {
                printf("monitor still running\n");
            }
            else {
                break;
            }
        }
        else if(strncmp(command, "view_treasure", 13) == 0) {
            char hunt_id[100], treasure_id[100];
            if(sscanf(command, "view_treasure %s %s", hunt_id, treasure_id) == 2) {
                view_treasure(hunt_id, treasure_id);
            }
            else {
                printf("usage: view_treasure <hunt_id> <treasure_id>\n");
            }
        }
        else if(strcmp(command, "list_hunts") == 0) {
            list_hunts();
        }
        else if(strncmp(command, "list_treasures", 14) == 0) {
            char *p = strchr(command, ' ');
            if(p) list_treasures(p + 1);
            else {
                printf("usage: list_treasures <hunt_id>\n");
            }
        }
        else {
            printf("invalid command\n");
        }
    }
    
    return 0;
}