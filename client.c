#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

int main(void) {
    uint8_t a;
    scanf("%hhd", &a);
    printf("%hhd", a);
    return 0;
}