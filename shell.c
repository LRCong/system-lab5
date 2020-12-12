#include <stdio.h>
#include <string.h>
// #include <stdlib.h>
#include "./main.c"

int getcmd(char *buf, int nbuf)
{
    printf("$ ");
    memset(buf, 0, nbuf);
    fgets(buf, nbuf, stdin);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

int main(void)
{
    static char buf[100];
    int fd;

    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        printf("%s", buf);
    }
    return 0;
}