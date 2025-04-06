#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

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

typedef struct hnt{
    char hunt_name[NAME_SIZE + 1];
    status_code h_code;
    treasure t[ARRAY_SIZE];
    char log[NAME_SIZE + 1];
    struct hnt *next;
}hunt;

/*------------------------------------------------------*/

void usage(char *name) {
    fprintf(stderr, "usage: %s <command> <hunt_id> <treasure_id>\n", name);
    exit(-1);
}

void add(char *hunt_id); // adds new treasure to the specified hunt
void list(char *hunt_id); // list all treasures in specified hunt
void view(char *hunt_id, char *treasure_id); // view details of specific treasure
void remove_treasure(char *hunt_id, char *id); // removes a treasure
void remove_hunt(char *hunt_id); // removes a hunt

hunt *create_hunt(char *name) {
    hunt *tmp = malloc(sizeof(hunt));

    tmp->next = NULL;
    strcpy(tmp->hunt_name, name);

    int dir_op = mkdir(name, 0777);
    if(dir_op == -1) {
        perror("error creating specified directory\n");
        exit(-1);
    }

    tmp->h_code = EXISTS;

    DIR *dir;
    if((dir = opendir(name)) == NULL) {
        perror("error opening directory\n");
        exit(-1);
    }

    //creating a log part
    strcpy(tmp->log, "log_");
    strcat(tmp->log, name);
    strcat(tmp->log, ".txt");
    tmp->log[strlen(tmp->log)] = '\0';

    char filepath[PATH_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s", name, tmp->log); // creating the filepath

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd == -1) {
        perror("error opening log file\n");
        exit(-1);
    }

    char *text = "LOG\n";
    ssize_t bytes_written = write(fd, text, strlen(text));
    if(bytes_written == -1) {
        perror("error writting to log file\n");
        close(fd);
        exit(-1);
    }

    close(fd);

    closedir(dir);

    return tmp;
}

void list(char *hunt_id) {
    DIR *dir;
    struct dirent *entry;

    if((dir = opendir(hunt_id)) == NULL) {
        perror("error opening directory in function 'list'\n");
        exit(-1);
    }

    while((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);

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
    return;
}

int main(void) {

    //hunt *huntRef = NULL;
    //huntRef = create_hunt_directory("hunt1");
    //printf("%d\n", check_hunt_status(huntRef, "hunt1"));

    //hunt *h = NULL;
    //h = create_hunt("Hunt001");
    remove_hunt("Hunt001");

    return 0;
}