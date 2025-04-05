#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#define ARRAY_SIZE 400
#define NAME_SIZE 100
#define LINE_SIZE 1000

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

void add(char *hunt_id);
void list(char *hunt_id);
void view(char *hunt_id, char *treasure_id);
void remove_treasure(char *hunt_id, char *id);
void remove_hunt(char *hunt_id);

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

int main(int argc, char **argv) {
    if(argc < 3) {
        usage(argv[0]);
    }

    //hunt *huntRef = NULL;
    //huntRef = create_hunt_directory("hunt1");
    //printf("%d\n", check_hunt_status(huntRef, "hunt1"));

    

    return 0;
}