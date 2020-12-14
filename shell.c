#include <stdio.h>
#include <string.h>
// #include <stdlib.h>
#include "./main.c"

int getcmd(char *buf, int nbuf)
{
    printf("$ ");
    memset(buf, 0, nbuf);
    fgets(buf, nbuf, stdin);
    if (buf[0] == 0)
        return -1;
    return 0;
}

int parse_cmd(char *buf)
{
    char path_buf[40];
    char command_buf[20];
    int mod = 4;
    int flag = 0;
    int i;
    for (i = 0; i <= strlen(buf); i++)
    {
        if (buf[i] == ' ' || buf[i] == '\n')
        {
            strncpy(command_buf, buf, i);
            command_buf[i] = '\0';
            break;
        }
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
        strcpy(path_buf, buf + i + 1);
        path_buf[strlen(path_buf) - 1] = '\0';
        create_dir(path_buf);
        break;
    case 1:
        strcpy(path_buf, buf + i + 1);
        path_buf[strlen(path_buf) - 1] = '\0';
        create_file(path_buf);
        break;
    case 2:
        strcpy(path_buf, buf + i + 1);
        path_buf[strlen(path_buf) - 1] = '\0';
        ls(path_buf);
        break;
    case 3:
        strcpy(path_buf, buf + i + 1);
        path_buf[strlen(path_buf) - 1] = '\0';
        while (path_buf[i] != ' ')
            i++;
        path_buf[i] = '\0';
        copy_file(path_buf, path_buf + i + 1);
        break;
    default:
        shutdown();
        return 0;
    }
    return 1;
}

int main(void)
{
    static char buf[100];

    fs_init();

    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        if (!parse_cmd(buf))
            return 0;
    }
    return -1;
}