#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define NAME_SIZE 100
#define LINE_SIZE 1000

typedef enum {EXISTS, NON_EXISTENT}status_code;

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
    struct hnt *next;
}hunt;

void usage(char *name) {
    fprintf(stderr, "usage: %s <command> <hunt_id> <treasure_id>\n", name);
    exit(-1);
}

void add(char *hunt_id);
void list(char *hunt_id);
void view(char *hunt_id, char *treasure_id);
void remove_treasure(char *hunt_id, char *id);
void remove_hunt(char *hunt_id);

hunt *create_hunt_directory(char *dir_name) {
    hunt *h = malloc(sizeof(hunt));
    strcpy(h->hunt_name, dir_name);
    mkdir(dir_name, 0777);
    h->next = NULL;
    h->h_code = EXISTS;

    return h;
}

status_code check_hunt_status(hunt *huntRef, char *hunt_id) {
    hunt *tmp = huntRef;
    while(strcmp(huntRef->hunt_name, hunt_id) != 0) {
        tmp = tmp->next;
    }

    return (tmp->h_code == EXISTS) ? EXISTS : NON_EXISTENT;
}

void add_hunt(hunt *huntRef, char *hunt_id) {
    if(huntRef == NULL) {
        huntRef = create_hunt_directory(hunt_id);
        return;
    }

    hunt *tmp = huntRef;
    while(tmp->next != NULL) {
        if(strcmp(tmp->hunt_name, hunt_id) == 0) {
            return;
        }
        tmp = tmp->next;
    }
    hunt *newHunt = create_hunt_directory(hunt_id);
    tmp->next = newHunt;

    return;
}

void print_hunt_list(hunt *huntRef) {
    hunt *tmp = huntRef;

    while(tmp) {
        printf("%s\n", tmp->hunt_name);
        tmp = tmp->next;
    }
}

int main(int argc, char **argv) {
    if(argc < 3) {
        usage(argv[0]);
    }

    //hunt *huntRef = NULL;
    //huntRef = create_hunt_directory("hunt1");
    //printf("%d\n", check_hunt_status(huntRef, "hunt1"));

    int data = 12345;
    
    return 0;
}