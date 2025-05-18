#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

#define SIZE 100
#define NAME_SIZE 100
#define LINE_SIZE 1000

/*
Quick description: Monitor doesn't print the output, it is redirected through a pipe
and then printed in an output file. Then printed to terminal from the main process

Phase 3 updates:
- added pipe_command_handle function
- added exec manage signals and with pipes
- updated the start function to redirect the stdout to pipe
- added the calculator function
- handled the printing function (reads from pipe) and then reads from output file
- added calc function for a given user
- added a wait period to list hunts for fully update the pipe output
*/

pid_t monitor_pid = -1;
int monitor_pipe[2];
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
        close(pipefd[0]); // close read
        if(dup2(pipefd[1], STDOUT_FILENO) == -1) { // redirectez stdout in pipe
            perror("dup2 error\n");
            exit(-1);
        }
        close(pipefd[1]); // close write
        printf("executing command: %s\n", args[0]);
        fflush(stdout);
        execvp(args[0], args);
        perror("error exec function\n");
        exit(-1);
    }
    else { // parent
        close(pipefd[1]);

        char buffer[256];
        ssize_t read_size;
        FILE *output = fopen("output.txt", "w");
        if(output == NULL) {
            perror("error opening output file\n");
            exit(-1);
        }
        while((read_size = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[read_size] = '\0';
            fprintf(output, "%s", buffer);
        }

        if(close(pipefd[0]) == -1) {
            perror("error closing pipe fd\n");
            exit(-1);
        }
        fclose(output);
        wait(NULL);
    }
}

void manage_signals_pipe(int signal) {
    if(signal == SIGUSR1) {
        FILE *f = fopen(commands, "r");
        if(f == NULL) {
            perror("error opening command file\n");
            exit(-1);
        }

        if(system("gcc -Wall -o treasure_mananger treasure_manager.c") == -1) {
            perror("error compiling treasure manager\n");
            exit(-1);
        }

        if(system("gcc -Wall -o c calculator.c") == -1) {
            perror("error compiling calculator\n");
            exit(-1);
        }

        char command[SIZE], hunt_id[SIZE], treasure_id[SIZE];
        if(fscanf(f, "%s %s %s", command, hunt_id, treasure_id) != 3) {
            perror("error reading content\n");
            fclose(f);
            exit(-1);
        }
        fclose(f);

        if(strcmp(command, "list_hunts") == 0) {
            DIR *dir = opendir(".");
            if(!dir) {
                perror("error opening directory\n");
                exit(-1);
            }

            struct dirent *data;

            while((data = readdir(dir)) != NULL) {
                if(data->d_type == DT_DIR && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
                    char path[256];
                    snprintf(path, sizeof(path), "%s/treasures.dat", data->d_name);
                    FILE *tr_f = fopen(path, "rb");
                    if(!tr_f) {
                        continue;
                    }

                    int count = 0;
                    treasure t;
                    while(fread(&t, sizeof(treasure), 1, tr_f) == 1) count++;
                    fclose(tr_f);

                    printf("Hunt: %s - Treasure: %d\n", data->d_name, count);
                    fflush(stdout);
                }
            }
            closedir(dir);
        }
        else if(strcmp(command, "list_treasures") == 0) {
            char *args[] = {"./treasure_manager", "--list", hunt_id, "ceva", NULL};
            pipe_command_run(args);
        }
        else if(strcmp(command, "view_treasure") == 0) {
            char *args[] = {"./treasure_manager", "--view_treasure", hunt_id, treasure_id, NULL};
            pipe_command_run(args);
        }
        else if(strcmp(command, "calculate_score") == 0) {
            char *args[] = {"./treasure_manager", "--calculate_score", hunt_id, "ceva", NULL};
            pipe_command_run(args);
        }
        else if(strcmp(command, "calc_score") == 0) { // task principal
            char *args[] = {"./treasure_manager", "--calc_score", hunt_id, treasure_id, NULL};
            pipe_command_run(args);
        }
        fflush(stdout); // eliberez output-ul
    
    }
    else if(signal == SIGTERM) {
            usleep(5000000);
            exit(0);
        }

    
}

void create_loop() {
    struct sigaction sg;
    sg.sa_flags = 0;
    sg.sa_handler = manage_signals_pipe; // seteaza functia de handle pe manage_signals
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
        return;
    }

    if(pipe(monitor_pipe) == -1) {
        perror("pipe creation failed\n");
        exit(-1);
    }

    pid_t current_pid = fork(); // creaza un child process in background
    if(current_pid < 0) {
        perror("error fork command\n");
        exit(-1);
    }

    else if(current_pid == 0) { // daca sunt in child process, creaza loop si asteapta semnale
        close(monitor_pipe[0]); // inchid read

        if(dup2(monitor_pipe[1], STDOUT_FILENO) == -1) {
            perror("error duplication\n");
            exit(-1);
        }
        close(monitor_pipe[1]);
        
        create_loop();
        exit(0);
    }

    close(monitor_pipe[1]); // inchid write
    monitor_pid = current_pid; // salvez in pid-ul monitorului, pid-ul child-ului
    running_code = 1; // monitorul e pornit

    printf("monitor started with PID %d\n", monitor_pid);

    return;
}

void stop() {
    if(running_code == 0) {
        perror("no monitor is running\n");
        return;
    }

    if(kill(monitor_pid, SIGTERM) == -1) { // trimit SIGTERM pentru a inchide monitorul
        perror("error killing monitor\n");
        exit(-1);
    }

    running_code = 0; // il marchez ca inchis
    printf("monitor is closing\n");
    
    return;
}

void read_monitor_output() {
    FILE *f = fopen("output.txt", "w");
    if(f == NULL) {
        perror("error opening output file\n");
        exit(-1);
    }

    char buffer[256];
    ssize_t bytes_read;
    while((bytes_read = read(monitor_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        fprintf(f, "%s", buffer);
        
        if(bytes_read < sizeof(buffer)) {
            break;
        }
        
    }

    fclose(f);
}

void view_treasure(char *hunt_id, char *treasure_id) {
    if(running_code == 0) {
        perror("monitor not running or monitor is closing\n");
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

    read_monitor_output();    
}

void list_hunts() { // voi merge pe denumirea standard "Hunt..."
    if(running_code == 0) {
        perror("no monitor running or monitor is closing\n");
        return;
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

    usleep(500000); // pt a avea timp sa fie scris in pipe
    read_monitor_output();   
}

void list_treasures(char *hunt_id) {
    if (running_code == 0) {
        printf("no monitor running or monitor is closing\n");
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

    read_monitor_output();
}

void calculate_score(char *hunt_id) {
    if(running_code == 0) {
        perror("no monitor running or monitor is closing\n");
        return;
    }

    FILE *f = fopen(commands, "w");
    if(f == NULL) {
        perror("error opening command file\n");
        exit(-1);
    }

    fprintf(f, "calculate_score %s ceva\n", hunt_id);

    fclose(f);

    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }

    read_monitor_output();
}

void calc_scores_by_user(char *hunt_id, char *user) {
    if(running_code == 0) {
        perror("no monitor running or monitor is closing\n");
        return;
    }

    FILE *f = fopen(commands, "w");
    if(f == NULL) {
        perror("error opening output file\n");
        exit(-1);
    }

    fprintf(f, "calc_score %s %s\n", hunt_id, user);

    fclose(f);

    if(kill(monitor_pid, SIGUSR1) == -1) {
        perror("error killing monitor\n");
        exit(-1);
    }

    read_monitor_output();
}

void print_from_file(char *filename) {
    FILE *f = fopen(filename, "r");

    if(f == NULL) {
        perror("error opening output file\n");
        exit(-1);
    }

    char line[1024];
    while(fgets(line, sizeof(line), f)) {
        if(strstr(line, "Monitor")) continue;
        fputs(line, stdout);
    }

    fclose(f);
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
            print_from_file("output.txt");
        }
        else if(strncmp(command, "view_treasure", 13) == 0) {
            char hunt_id[100], treasure_id[100];
            if(sscanf(command, "view_treasure %s %s", hunt_id, treasure_id) == 2) {
                view_treasure(hunt_id, treasure_id);
                print_from_file("output.txt");
            }
            else {
                printf("usage: view_treasure <hunt_id> <treasure_id>\n");
            }
        }
        else if(strncmp(command, "list_treasures", 14) == 0) {
            char *p = strchr(command, ' ');
            if(p) {
                list_treasures(p + 1);
                print_from_file("output.txt");
            }
            else {
                printf("usage: list_treasures <hunt_id>\n");
            }
        }
        else if(strncmp(command, "calculate_score", 15) == 0) { // value total
            char hunt_id[100];
            if(sscanf(command, "calculate_score %s", hunt_id) == 1) {
                calculate_score(hunt_id);
                print_from_file("output.txt");
            }
            else {
                printf("usage: calculate_score <hunt_id>\n");
            }
        }
        else if(strncmp(command, "calc_score", 10) == 0) { // total value per given username in a hunt
            char hunt_id[100], user[100];
            if(sscanf(command, "calc_score %s %s", hunt_id, user) == 2) {
                calc_scores_by_user(hunt_id, user);
                print_from_file("output.txt");
            }
            else {
                printf("usage: calc_score <hunt_id> <username>\n");
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