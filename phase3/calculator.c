#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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
    if(argc != 3) {
        perror("format incorect!\n");
        exit(-1);
    }

    treasure t;
    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    int fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }

    ssize_t bytes_read;
    int found = 0;
    while((bytes_read = read(fd, &t, sizeof(treasure))) > 0) {
        if(strcmp(t.username, argv[2]) == 0) {
            found = 1;
            break;
        }
    }

    if(found == 0) {
        perror("username not found in Hunt\n");
        exit(-1);
    }
    close(fd);

    fd = open(path, O_RDONLY);
    if(fd == -1) {
        perror("error opening treasure file\n");
        exit(-1);
    }

    ssize_t read_size;
    int s = 0;
    while((read_size = read(fd, &t, sizeof(treasure))) > 0) {
        if(strcmp(t.username, argv[2]) == 0) {
            s += t.value;
        }
    }

    close(fd);

    printf("\nScore from %s for the user \"%s\" is: %d\n", argv[1], argv[2], s);
    fflush(stdout); // fortez output-ul


    return 0;
}