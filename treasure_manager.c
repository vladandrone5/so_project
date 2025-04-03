#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

#define NAME_SIZE 100
#define LINE_SIZE 1000

typedef struct {
    char treasure_id[10];
    char username[NAME_SIZE + 1];
    double latitude, longitude;
    char clue[LINE_SIZE + 1];
    int value;
}treasure;

int main(int argc, char **argv) {
    if(argc < 3) {
        fprintf(stderr, "usage: %s <command> <hunt_id> <treasure_id>\n", argv[0]);
        exit(-1);
    }


    return 0;
}