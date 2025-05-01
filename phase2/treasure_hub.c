#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SIZE 100

pid_t monitor_pid = -1;
int running_code = 0;
char commands[] = "commands.txt";

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

        if(fscanf(f, "%s%s%s", command, hunt_id, treasure_id) != 3) {
            perror("error reading content\n");
            exit(-1);
        }

        if(fclose(f) != 0) {
            perror("error closing file\n");
            exit(-1);
        }

        if(strcmp(command, "list_hunts") == 0) {
            system("ls -d */ | cut -f1 -d'/'");
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

int main(void) {
    char command[256];

    
    struct sigaction sg;
    //sg.sa_handler = manage_signals;
    sigemptyset(&sg.sa_mask);
    sg.sa_flags = SA_RESTART;
    //sigaction(SIGCHLD, &sg, NULL);
    

    /*
    while(1) {
        printf("> ");
        if(fgets(command, sizeof(command), stdin) == NULL) break;
        command[strcspn(command, "\n")] = 0;

        if(strcmp(command, "start_monitor") == 0) {
            start();
        }
        else if(strcmp(command, "stop_monitor") == 0) {
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
        else {
            printf("invalid command\n");
        }
    }
    */
    return 0;
}