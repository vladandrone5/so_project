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

/*
Phase 3 updates:
- added pipe_command_handle function

*/

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

//Functii optionale (treasure_manager)

void add_treasure(char *hunt_id);
void create_hunt(char *hunt_id);


/*------------------------------------*/

void execute(char *op, char **args) {
    pid_t pid = fork();
    if(pid == -1) {
        perror("error fork\n");
        exit(-1);
    }
    else if(pid == 0) {
        execvp(op, args);
        perror("exec failed\n");
        exit(-1);   
    }
    else {
        int status;
        waitpid(pid, &status, 0);
    }
}

void pipe_command_run(char **args) {
    int pipefd[2];
    if(pipe(pipefd) == -1) {
        perror("pipe error\n");
        exit(-1);
    }

    pid_t pid = fork();
    if(pid < 0) {
        perror("fork error\n");
        exit(-1);
    }

    if(pid == 0) { //child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO); // redirectez stdout in pipe
        close(pipefd[1]);
        execvp(args[0], args);
        perror("error exec function\n");
        exit(-1);
    }
    else {
        close(pipefd[1]);
        char buffer[256];
        ssize_t read_size;
        while((read_size = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[read_size] = '\0';
            printf("%s", buffer);
        }
        close(pipefd[0]);
        wait(NULL);
    }
}

void manage_signals(int signal) { // modul de response la semnale
    if(signal == SIGUSR1) { // citesc din fisierul de comenzi cand primesc SIGUSR1
        FILE *f = NULL;
        if((f = fopen(commands, "r")) == 0) {
            perror("error opening command file\n");
            exit(-1);
        }

        char command[SIZE], hunt_id[SIZE], treasure_id[SIZE];

        system("gcc -Wall -o treasure_manager new_treasure_manager.c");

        // in functie de comanda executa operatia corespunzatoare
        if(fscanf(f, "%s %s %s", command, hunt_id, treasure_id) != 3) {
            perror("error reading content\n");
            exit(-1);
        }

        if(fclose(f) != 0) {
            perror("error closing file\n");
            exit(-1);
        }

        if(strcmp(command, "list_hunts") == 0) { //LIST_HUNTS
            DIR *dir = opendir("."); // directorul curent -> caut toate hunt-urile
            if(dir == NULL) {
                perror("error opening directory\n");
                exit(-1);
            }

            struct dirent *data;

            while((data = readdir(dir)) != NULL) {
                if(data->d_type == DT_DIR && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
                    char path[256];
                    snprintf(path, sizeof(path), "%s/treasures.dat", data->d_name);

                    FILE *tr_f = fopen(path, "rb");
                    if(tr_f == NULL) { // verific daca exista file-ul de treasure, daca nu sare peste
                        continue;
                    }

                    int cnt = 0;
                    treasure t;
                    while(fread(&t, sizeof(treasure), 1, tr_f) == 1) {
                        cnt++;
                    }

                    fclose(tr_f);

                    printf("Hunt: %s - Treasures: %d\n", data->d_name, cnt);
                }
            }
            closedir(dir);
        }
        else if(strcmp(command, "list_treasures") == 0) { // LIST_TREASURES
            char op[256];
            snprintf(op, sizeof(op), "./treasure_manager --list %s ceva", hunt_id);
            if(system(op) == -1) {
                perror("error operation\n");
                exit(-1);
            }
        }
        else if(strcmp(command, "view_treasure") == 0) { // VIEW_TREASURES
            char op[256];
            snprintf(op, sizeof(op), "./treasure_manager --view_treasure %s %s", hunt_id, treasure_id);
            if(system(op) == -1) {
                perror("error operation\n");
                exit(-1);
            }
        }
        else if(signal == SIGTERM) { // asteapta 5 secunde si se inchide daca primeste SIGTERM
            usleep(5000000);
            exit(0);
        }

    }
}

/*
void signal_exec_manage(int signal) { // varianta cu exec pentru signal manager, momentan doar de test
    if(signal == SIGUSR1) {
        FILE *f = fopen(commands, "r");
        if(f == NULL) {
            perror("error opening file\n");
            exit(-1);
        }

        char command[SIZE], hunt_id[SIZE], treasure_id[SIZE];

        pid_t pid = fork();
        if(pid == 0) {
            char *args[] = {"gcc", "-Wall", "-o", "treasure_maanger", "new_treasure_manager.c", NULL};
            execvp("gcc", args);
            perror("exec failed\n");
            exit(-1);
        }
        else {
            int status;
            waitpid(pid, &status, 0);
        }

        if(fscanf(f, "%s %s %s", command, hunt_id, treasure_id) != 3) {
            perror("error reading command\n");
            exit(-1);
        }

        fclose(f);

        if(strcmp(command, "list_treasures") == 0) {
            char *args[] = {"./treasure_manager", "--list", hunt_id, "ceva", NULL};
            execute("./treasure_manager", args);
        }
        else if(strcmp(command, "view_treasure") == 0) {
            char *args[] = {"./treasure_manager", "--view_treasure", hunt_id, treasure_id, NULL};
            execute("./treasure_manager", args);
        }
        else if(strcmp(command, "list_hunts") == 0) {
            DIR *dir = opendir(".");
            if(dir == NULL) {
                perror("error opening directory\n");
                exit(-1);
            }

            struct dirent *data;

            while((data = readdir(dir)) != NULL) {
                if(data->d_type == DT_DIR && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
                    
                    char path[256];
                    snprintf(path, sizeof(path), "%s/treasures.dat", data->d_name);
                    
                    FILE *tr_f = fopen(path, "rb");
                    if(tr_f == NULL) {
                        continue;
                    }
                    

                    int cnt = 0;
                    treasure t;

                    while(fread(&t, sizeof(treasure), 1, tr_f) == 1) {
                        cnt++;
                    }

                    fclose(tr_f);
                    printf("Hunt: %s - Treasures: %d\n",data->d_name, cnt);
                }
            }
            closedir(dir);
        }
    }
    else if(signal == SIGTERM) {
        usleep(5000000);
        exit(0);
    }
}
*/

void create_loop() {
    struct sigaction sg;
    sg.sa_flags = 0;
    sg.sa_handler = manage_signals; // seteaza functia de handle pe manage_signals
    sigemptyset(&sg.sa_mask); // ce semnale sunt blocate cat ruleaza manage_signals
    sg.sa_flags = 0; // default
    sigaction(SIGUSR1, &sg, NULL); // actiunea in SIGUSR1
    sigaction(SIGUSR2, &sg, NULL); // in SIGUSR2
    sigaction(SIGTERM, &sg, NULL);
    // setez manage signals ca functie ce trebuie apelata cand primesc SIGUSR1, SIGUSR2 sau SIGTERM

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

    else if(current_pid == 0) { // daca sunt in child process, creaza loop si asteapta semnale
        create_loop();
        exit(0);
    }

    monitor_pid = current_pid; // salvez in pid-ul monitorului, pid-ul child-ului
    running_code = 1; // monitorul e pornit

    printf("monitor started with PID %d\n", monitor_pid);

    return;
}

void stop() {
    if(running_code == 0) {
        perror("no monitor is running\n");
        exit(-1);
    }

    if(kill(monitor_pid, SIGTERM) == -1) { // trimit SIGTERM pentru a inchide monitorul
        perror("error killing monitor\n");
        exit(-1);
    }

    running_code = 0; // il marchez ca inchis
    printf("monitor is closing\n");
    
    return;
}

void view_treasure(char *hunt_id, char *treasure_id) {
    if(running_code == 0) {
        perror("monitor not running\n");
        exit(-1);
    }

    FILE *f = fopen(commands, "w");
    if(f == NULL) {
        perror("error opening command file\n");
        exit(-1);
    }

    fprintf(f, "view_treasure %s %s\n", hunt_id, treasure_id);
    fclose(f);

    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }
}

void list_hunts() { // voi merge pe denumirea standard "Hunt..."
    if(running_code == 0) {
        perror("no monitor running\n");
        exit(-1);
    }

    FILE *f = fopen(commands, "w");
    if(f == NULL) {
        perror("error opening command file\n");
        exit(-1);
    }

    fprintf(f, "list_hunts ceva ceva\n");
    fclose(f);


    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }
}

void list_treasures(char *hunt_id) {
    if (running_code == 0) {
        printf("Monitor not running.\n");
        return;
    }
    FILE *fp = fopen(commands, "w");
    if(fp == NULL) {
        perror("error opening command file\n");
        exit(-1);
    }
    fprintf(fp, "list_treasures %s ceva\n", hunt_id);
    
    fclose(fp);
    
    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }
}

void manage_child_sig(int signal) { // asteapta sa se termine child process-ul iar apoi printeaza ca s-a inchis(verif)
    int status;
    wait(&status);
    printf("Monitor process terminated. Status: %d\n", WEXITSTATUS(status));

    return;
}

void operation() {
    char command[256]; // string pentru comanda

    // child e monitor
    struct sigaction sg;
    sg.sa_handler = manage_child_sig;
    sigemptyset(&sg.sa_mask);
    sg.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sg, NULL);
    // secventa care seteaza un signal handler pt a rula manage child sig cand monitorul se opreste

    
    while(1) { // main loop care asteapta inputul user-ului
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
        else if(strcmp(command, "list_hunts") == 0) {
            list_hunts();
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
}

int main(void) {
    //daca primeste comenzi dupa stop da exit
    //folosesc varianta cu system pentru prelucrare semnale
    operation();

    return 0;
}