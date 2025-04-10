#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>

#define ARRAY_SIZE 400
#define PATH_SIZE 256
#define NAME_SIZE 100
#define LINE_SIZE 1000

//for testing purposes for the moment will be one treasure per file

typedef enum {EXISTS, NON_EXISTENT}status_code;

/*structuri specifice-----------------------------------*/

typedef struct {
    char treasure_id[10];
    char username[NAME_SIZE + 1];
    double latitude, longitude;
    char clue[LINE_SIZE + 1];
    int value;
    status_code t_code;
}treasure;

/*------------------------------------------------------*/

void usage(char *name) {
    fprintf(stderr, "usage: %s <command> <hunt_id> <treasure_id>\n", name);
    exit(-1);
}

//functii de implementat

void add(char *hunt_id); // adds new treasure to the specified hunt
void list(char *hunt_id); // list all treasures in specified hunt
void view(char *hunt_id, char *treasure_id); // view details of specific treasure
void remove_treasure(const char *hunt_id, const char *treasure_id); // removes a treasure
void remove_hunt(char *hunt_id); // removes a hunt
void process_operation(char *operation, char *hunt_id, char *treasure_id);

treasure create_treasure(char *treasure_id) {
    treasure tmp;

    strcpy(tmp.treasure_id, treasure_id);
    sprintf(tmp.username, "%s", "user");

    srand(time(NULL));

    tmp.latitude = rand() % 1000;
    tmp.longitude = rand() % 1000;

    strcpy(tmp.clue, "this is a clue");
    tmp.value = rand() % 10;

    tmp.t_code = EXISTS;

    return tmp;
}

void create_hunt(const char *hunt_id) {
    char path[100];

    snprintf(path, sizeof(path), "./%s", hunt_id);

    if(mkdir(path, 0777) == -1) {
        perror("error creating directory, maybe exists\n");
        exit(-1);
    }

    char log_path[100];
    snprintf(log_path, sizeof(log_path), "%s/%s", path, "logged_hunt");

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644); // rw-r-r
    if(fd == -1) {
        perror("error opening log file\n");
        exit(-1);
    }
    char *head = "LOG\n-----\n";
    if(write(fd, head, strlen(head)) == -1) {
        perror("error writting to log\n");
        close(fd);
        exit(-1);
    }

    close(fd);

    char symlink_name[100];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);

    if(symlink(log_path, symlink_name) == -1) {
        perror("error creating symlink\n");
        exit(-1);
    }
}

void log_action(const char *hunt_id, const char *action) {
    char path[100];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, "logged_hunt");

    int fd = open(path, O_WRONLY | O_APPEND);
    if(fd == -1) {
        perror("error opening the log file\n");
        exit(-1);
    }

    if(write(fd, action, strlen(action)) == -1) {
        perror("error writting to log file\n");
        close(fd);
        exit(-1);
    }

    return;
}

void add_treasure(const char *hunt_id, treasure t) {
    char path[100];
    snprintf(path, sizeof(path), "%s/%s", hunt_id, "treasures.dat");

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if(fd == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }

    //input based treasure
    t = (treasure){0};
    char line[1000];
    printf("Enter id: ");
    fgets(line, sizeof(line), stdin);
    printf("Enter username: ");
    fgets(line, sizeof(line), stdin);
    printf("Enter coordinates: ");
    scanf("%lf%lf", &t.latitude, &t.longitude);
    printf("Enter clue: ");
    fgets(line, sizeof(line), stdin);
    printf("Enter value: ");
    fgets(line, sizeof(line), stdin);

    if(write(fd, &t, sizeof(treasure)) == -1) {
        perror("error writting to treasure file\n");
        close(fd);
        exit(-1);
    }

    close(fd);

    char action[100];
    sprintf(action, "added treasure%s\n", t.treasure_id);
    log_action(hunt_id, action);

    return;

}

void view_treasure(const char *hunt_id, char *treasure_id) {
    char path[100];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id); // folosesc .dat pt a pune orice tip de date

    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }

    treasure t;
    ssize_t read_size;
    while((read_size = read(fd, &t, sizeof(treasure))) > 0) { // citesc direct tot struct-ul
        if(strcmp(treasure_id, t.treasure_id) == 0) {
            printf("Treasure ID: %s\n", t.treasure_id);
            printf("Username: %s\n", t.username);
            printf("Coordinates: (%f, %f)\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            break;
        }
    }

    close(fd);

    char action[100];
    sprintf(action, "viewed treasure%s\n", t.treasure_id);
    log_action(hunt_id, action);

    return;
}

void list(char *hunt_id) { // LIST ALL TREASURE FILES
    DIR *dir;
    struct dirent *entry;

    if((dir = opendir(hunt_id)) == NULL) { 
        perror("error opening directory in function 'list'\n");
        exit(-1);
    }
    printf("List of all the treasure files:\n");
    while((entry = readdir(dir)) != NULL) { // parcurg tot directory-ul
        if(strstr(entry->d_name, "log") || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        printf("%s\n", entry->d_name);
    }

    closedir(dir);

    return;
}

void list_treasures(const char *hunt_id) {
    char path[100];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("error opening treasure files\n");
        exit(-1);
    }

    struct stat file_stat;

    if(stat(path, &file_stat) == 0) {
        printf("Hunt name: %s\n", hunt_id);
        printf("File size: %lld\n", file_stat.st_size);
        printf("Modified: %s\n", ctime(&file_stat.st_mtime));
    }

    treasure t;

    ssize_t read_size;
    while((read_size = read(fd, &t, sizeof(treasure))) > 0) {
        printf("Treasure ID: %s\n", t.treasure_id);
        printf("Username: %s\n", t.username);
        printf("Coordinates: (%f, %f)\n", t.latitude, t.longitude);
        printf("Clue: %s\n", t.clue);
        printf("Value: %d\n\n", t.value);
    }

    close(fd);

    char action[100];
    sprintf(action, "listed treasures from hunt %s\n", hunt_id);
    log_action(hunt_id, action);

    return;
}

int count_files(char *directory_path) {
    DIR *dir;
    if((dir = opendir(directory_path)) == NULL) {
        perror("error opening the directory\n");
        exit(-1);
    }

    struct dirent *entry;

    int count = 0;

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        count++;
    }

    closedir(dir);
    return count;
}

void remove_treasure(const char *hunt_id, const char *treasure_id) {
    char path[100];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }

    treasure t[100];
    int cnt = 0;
    ssize_t read_size;
    while((read_size = read(fd, &t[cnt], sizeof(treasure))) > 0) { // numar treasure-urile
        cnt++;
    }

    close(fd);

    if((fd = open(path, O_RDWR | O_TRUNC)) == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }

    for(int i = 0; i < cnt; i++) {
        if(strcmp(t[i].treasure_id, treasure_id) != 0) {
            if(write(fd, &t[i], sizeof(treasure)) == -1) {
                perror("error writting in file\n");
                exit(-1);
            }
        }
    }
    
    close(fd);
    char action[100];
    sprintf(action, "removed treasure%s\n", treasure_id);
    log_action(hunt_id, action);

    return;

}

int has_files_in_directory(char *dirpath) {
    DIR *dir;
    if((dir = opendir(dirpath)) == NULL) {
        perror("error opening directory\n");
        exit(-1);
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            closedir(dir);
            return 1;
        }
    }

    closedir(dir);
    return 0;
}

void delete_files(char *directory_path) {
    DIR *dir;
    if((dir = opendir(directory_path)) == NULL) {
        perror("error opening directory\n");
        exit(-1);
    }

    struct dirent *entry;
    char file_path[256];

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(file_path, sizeof(file_path), "%s/%s", directory_path, entry->d_name);

        if(unlink(file_path) == -1) {
            perror("error deleting file\n");
            exit(-1);
        }
    }

    closedir(dir);

    return;
}

void remove_hunt(char *hunt_id) {
    delete_files(hunt_id);
    rmdir(hunt_id);

    char path[200];
    sprintf(path, "./logged_hunt-%s", hunt_id);
    if(unlink(path) == -1) {
        perror("error deleting symbolic link\n");
        exit(-1);
    }

    return;
}

void process_operation(char *operation, char *hunt_id, char *treasure_id) {
    if(strcmp(operation, "--add") == 0) {
        treasure t = {0};
        add_treasure(hunt_id, t);
        return;
    }
    else if(strcmp(operation, "--remove") == 0) {
        remove_treasure(hunt_id, treasure_id);
        return;
    }
    else if(strcmp(operation, "--create_hunt") == 0) {
        create_hunt(hunt_id);
        return;
    }
    else if(strcmp(operation, "--list") == 0) {
        list_treasures(hunt_id);
        return;
    }
    else if(strcmp(operation, "--remove_hunt") == 0) {
        remove_hunt(hunt_id);
        return;
    }
    else if(strcmp(operation, "--view_treasure") == 0) {
        view_treasure(hunt_id, treasure_id);
        return;
    }
    else if(strcmp(operation, "--create_hunt") == 0) {
        create_hunt(hunt_id);
        return;
    }

    return;
}

int main(int argc, char **argv) {

    if(argc < 3) {
        usage(argv[0]);
    }

    process_operation(argv[1], argv[2], argv[3]);


    return 0;
}