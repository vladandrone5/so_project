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

typedef struct hnt{
    char hunt_name[NAME_SIZE + 1];
    status_code h_code;
    treasure t[ARRAY_SIZE];
    int count_treasure;
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
void process_operation(char *operation, char *hunt_id, char *treasure_id);

treasure *create_treasure(char *treasure_id) {
    treasure *tmp = malloc(sizeof(treasure));

    strcpy(tmp->treasure_id, treasure_id);
    strcpy(tmp->username, "User");

    srand(time(NULL));

    tmp->latitude = rand() % 1000;
    tmp->longitude = rand() % 1000;

    strcpy(tmp->clue, "this is a clue");
    tmp->value = rand() % 10;

    tmp->t_code = EXISTS;

    return tmp;
}

hunt *create_hunt(char *name) {
    const char *current_path = "./";
    DIR *checkdir;

    if((checkdir = opendir(current_path)) == NULL) {
        perror("cannot open current directory\n");
        exit(-1);
    }

    struct dirent *data;

    while((data = readdir(checkdir)) != NULL) {
        if(strcmp(data->d_name, name) == 0 && data->d_type == DT_DIR) {
            printf("directory already exists\n");
            closedir(checkdir);
            exit(-1);
        }
    }

    hunt *tmp = malloc(sizeof(hunt));

    tmp->next = NULL;
    tmp->count_treasure = 0;
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
    strcpy(tmp->log, "logged_hunt_");
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

    char *text = "LOG\n-----\n";
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

void list(char *hunt_id) { // LIST ALL TREASURE FILES
    DIR *dir;
    struct dirent *entry;

    if((dir = opendir(hunt_id)) == NULL) {
        perror("error opening directory in function 'list'\n");
        exit(-1);
    }
    printf("List of all the treasure files:\n");
    while((entry = readdir(dir)) != NULL) {
        if(strstr(entry->d_name, "log") || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        printf("%s\n", entry->d_name);
    }

    closedir(dir);

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

void add(char *hunt_id) { // ADD TREASURE TO HUNT

    int count = count_files(hunt_id);
    char number[20];
    sprintf(number, "%d", count);
    number[strlen(number)] = '\0';

    char treasure_name[256];
    strcpy(treasure_name, "Treasure");
    strcat(treasure_name, number);
    strcat(treasure_name, ".txt");

    //treasure *tmp = create_treasure(treasure_name);

    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, treasure_name);

    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd == -1) {
        perror("error opening file\n");
        exit(-1);
    }

    const char *text = "File id:\n";
    ssize_t bytes_written = write(fd, text, strlen(text));

    if(bytes_written == -1) {
        perror("error writting to file\n");
        close(fd);
        exit(-1);
    }

    bytes_written = write(fd, treasure_name, strlen(treasure_name));
    bytes_written = write(fd, "\n", 1);
    


    if(bytes_written == -1) {
        perror("error writting to file\n");
        close(fd);
        exit(-1);
    }



    close(fd);

    // log writting 

    char log_path[1024];
    char log_name[1024];
    strcpy(log_name, "logged_hunt_");
    strcat(log_name, hunt_id);
    strcat(log_name, ".txt");
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, log_name);

    int fd_log = open(log_path, O_WRONLY);

    if(fd_log == -1) {
        perror("error opening the log file\n");
        exit(-1);
    }

    off_t position = lseek(fd_log, 0, SEEK_END);
    ssize_t bytes_written_to_log = write(fd_log, "added ", 6);
    bytes_written_to_log = write(fd_log, treasure_name, strlen(treasure_name));
    bytes_written_to_log = write(fd_log, "\n", 1);

    if(bytes_written_to_log == -1) {
        perror("error writting to log file\n");
        close(fd_log);
        exit(-1);
    }

    close(fd_log);

}

void remove_treasure(char *hunt_id, char *id) { // REMOVE TREASURE FUNCTION
    DIR *dir;

    if((dir = opendir(hunt_id)) == NULL) {
        perror("error opening directory\n");
        exit(-1);
    }

    int found_file = 0;
    struct dirent *entry;

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, id)) {
            found_file = 1;

            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", hunt_id, id);

            if(unlink(filepath) == -1) {
                perror("error deleting the file");
                closedir(dir);
                exit(-1);
            }
            else {
                break;
            }
        }
    }

    closedir(dir);

    if(!found_file) {
        printf("file not found\n");
        return;
    }

    char log_name[1024];
    char log_path[1024];
    sprintf(log_name, "logged_hunt_%s.txt", hunt_id);

    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, log_name);

    int fd_log = open(log_path, O_WRONLY);
    if(fd_log == -1) {
        perror("error opening log file\n");
        exit(-1);
    }

    char text[256];
    sprintf(text, "removed %s\n", id);
    text[strlen(text)] = '\0';

    if(lseek(fd_log, 0, SEEK_END) == -1) {
        perror("error seeking position\n");
        close(fd_log);
        exit(-1);
    }

    ssize_t bytes_written_to_log = write(fd_log, text, strlen(text));

    if(bytes_written_to_log == -1) {
        perror("error writting to log\n");
        close(fd_log);
        exit(-1);
    }

    close(fd_log);

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

    hunt *h = NULL;
    //h = create_hunt("Hunt001");
    //remove_hunt("Hunt001");
    //add("Hunt001");
    //remove_treasure("Hunt001", "Treasure1.txt");
    //list("Hunt001");
    return 0;
}