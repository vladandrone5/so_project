#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// calculeaza scor in functie de user

#define NAME_SIZE 100
#define LINE_SIZE 1000
#define MAX_USERS 100

typedef struct {
    char treasure_id[10];
    char username[NAME_SIZE];
    double latitude, longitude;
    char clue[LINE_SIZE];
    int value;
} treasure;

int main(int argc, char **argv) {
    if(argc != 2) {
        perror("format incorect\n");
        exit(-1);
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }  

    treasure t;
    int total_score = 0;
    ssize_t read_size;
    while((read_size = read(fd, &t, sizeof(treasure))) > 0) {
        total_score += t.value;
    }
    close(fd);

    printf("Total score from %s is: %d\n", argv[1], total_score);
    return 0;
}