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

int parse_cmd(char *buf)
{
    char path_buf[20];
    char command_buf[20];
    int mod = 0;
    int flag = 0;
    for (int i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == ' ')
        {
            strncpy(command_buf, buf, i);
            strcpy(path_buf, buf + i + 1);
            command_buf[i] = '\0';
            path_buf[strlen(buf) - i - 2] = '\0';
            flag = 1;
            break;
        }
    }
    if (flag == 0)
    {
        strcpy(command_buf, buf);
        command_buf[strlen(buf) - 1] = '\0';
    }
    if (strcmp("ls", command_buf) == 0)
        mod = 2;
    else if (strcmp("mkdir", command_buf) == 0)
        mod = 0;
    else if (strcmp(command_buf, "touch") == 0)
        mod = 1;
    else if (strcmp(command_buf, "cp") == 0)
        mod = 3;
    switch (mod)
    {
    case 0:
        printf("%s", "'step1'");
        create_dir(path_buf);
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    }
    return 0;
}

int main(void)
{
    static char buf[100];

    fs_init();
    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        parse_cmd(buf);
    }
    return 0;
}