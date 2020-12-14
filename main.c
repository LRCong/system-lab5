#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "./disk.c"

typedef struct super_block
{
    int32_t magic_num;        // 幻数
    int32_t free_block_count; // 空闲数据块数
    int32_t free_inode_count; // 空闲inode数
    int32_t dir_inode_count;  // 目录inode数
    uint32_t block_map[128];  // 数据块占用位图
    uint32_t inode_map[32];   // inode占用位图
} sp_block;

typedef struct inode
{
    uint32_t size;           // 文件大小
    uint16_t file_type;      // 文件类型（文件/文件夹）
    uint16_t link;           // 连接数
    uint32_t block_point[6]; // 数据块指针
} inode;

typedef struct dir_item
{                      // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id; // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;    // 当前目录项是否有效
    uint8_t type;      // 当前目录项类型（文件/目录）
    char name[121];    // 目录项表示的文件/目录的文件名/目录名
} dir_item;

// 读取数据块
int read_block(unsigned int block_num, char *buf)
{
    block_num = 2 * block_num;
    disk_read_block(block_num, buf);
    buf += DEVICE_BLOCK_SIZE;
    disk_read_block(block_num + 1, buf);
}

// 写入数据块
int write_block(unsigned int block_num, char *buf)
{
    block_num = 2 * block_num;
    disk_write_block(block_num, buf);
    buf += DEVICE_BLOCK_SIZE;
    disk_write_block(block_num + 1, buf);
}

// 读入inode_table
void read_inode(inode *inode_table)
{
    for (int i = 1; i < 32; i++)
    {
        read_block(i, (char *)&(inode_table[32 * (i - 1)]));
    }
}

// 写入inode_table
void write_inode(inode *inode_table)
{
    for (int i = 1; i < 32; i++)
    {
        write_block(i, (char *)&(inode_table[32 * (i - 1)]));
    }
}

// 读入spBlock
void read_sp(sp_block *spBlock)
{
    char buf[1024];
    char *p;
    read_block(0, buf);
    p = (char *)spBlock;
    for (int i = 0; i < sizeof(sp_block); i++)
    {
        *p = buf[i];
        p++;
    }
}

// 写入spBlock
void write_sp(sp_block *spBlock)
{
    char buf[1024];
    char *p;
    // 读取超级块
    p = (char *)spBlock;
    for (int i = 0; i < sizeof(sp_block); i++)
    {
        buf[i] = *p;
        p++;
    }
    write_block(0, buf);
}

// 找到空闲inode
int find_free_inode()
{
    int num = -1;
    uint32_t buf;
    sp_block *spBlock;
    spBlock = (sp_block *)malloc(sizeof(sp_block));
    read_sp(spBlock);
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            buf = (spBlock->inode_map[i] >> (31 - j)) & (uint16_t)1;
            if (buf == 0)
            {
                num = i * 32 + j;
                spBlock->inode_map[i] = spBlock->inode_map[i] | (1 << (31 - j));
                break;
            }
        }
        if (buf == 0)
            break;
    }
    spBlock->free_inode_count--;
    write_sp(spBlock);
    return num;
}

// 找到一个空闲块
int find_free_block()
{
    int num = -1;
    uint32_t buf;
    sp_block *spBlock;
    spBlock = (sp_block *)malloc(sizeof(sp_block));
    read_sp(spBlock);
    for (int i = 0; i < 128; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            buf = (spBlock->block_map[i] >> (31 - j)) & (uint16_t)1;
            if (buf == 0)
            {
                num = 32 * i + j;
                spBlock->block_map[i] = spBlock->block_map[i] | (1 << (31 - j));
                break;
            }
        }
        if (buf == 0)
            break;
    }
    spBlock->free_block_count--;
    write_sp(spBlock);
    return num;
}

// 初始化fs, 如果不是本文件系统，则初始化
void fs_init()
{
    // int i;
    char *p;
    char buf[1024];
    sp_block *spBlock;
    inode inode_table[1024];
    dir_item block_buffer[8];
    spBlock = (sp_block *)malloc(sizeof(sp_block));
    open_disk();
    read_sp(spBlock);
    // 发现不是本文件系统，则进行初始化
    if (spBlock->magic_num != 0xdec0de)
    {
        puts("Create new file system");
        // 初始化超级块
        spBlock->magic_num = 0xdec0de;
        spBlock->free_block_count = 4096 - 1 - 32 - 1;
        spBlock->free_inode_count = 1024 - 1;
        spBlock->dir_inode_count = 1;

        memset(spBlock->block_map, 0, sizeof(spBlock->block_map));
        memset(spBlock->inode_map, 0, sizeof(spBlock->inode_map));

        spBlock->inode_map[0] = (1 << 31);
        spBlock->block_map[0] = ~(spBlock->block_map[0]);
        spBlock->block_map[1] = (1 << 31) | (1 << 30);

        // 初始化第一个inode
        memset(inode_table, 0, sizeof(inode_table));
        inode_table[0].block_point[0] = 33;
        inode_table[0].file_type = 1;
        inode_table[0].size = 0;
        memset(block_buffer, 0, sizeof(block_buffer));

        write_block(33, (char *)block_buffer);
        write_inode(inode_table);
        write_sp(spBlock);
    }
    return;
}

// 查找路径对应的inode_id
int find_inode(char *path)
{
    if (strlen(path) <= 1)
        return 0;
    inode inode_table[1024];
    read_inode(inode_table);
    dir_item block_buffer[8];
    int a = 0;
    int last = 0;
    char name_buf[20];
    int len = strlen(path);
    for (int o = 0; o <= len; o++)
    {
        if (o > 0 && path[o] == '/' || o == len)
        {
            memset(name_buf, 0, sizeof(name_buf));
            strncpy(name_buf, path + last + 1, o - last - 1);
            last = o;
            for (int i = 0; i < 6; i++)
            {
                int flag = 0;
                memset(block_buffer, 0, sizeof(block_buffer));
                read_block(inode_table[a].block_point[i], (char *)block_buffer);
                for (int j = 0; j < 8; j++)
                {
                    if (block_buffer[j].valid == 0)
                        break;
                    if (strcmp(name_buf, block_buffer[j].name) == 0)
                    {
                        a = block_buffer[j].inode_id;
                        flag = 1;
                        break;
                    }
                }
                if (flag == 1)
                    break;
            }
        }
    }
    return a;
}

// 列出文件
void ls(char *path)
{
    inode inode_table[1024];
    read_inode(inode_table);
    int len = strlen(path);
    int inode_id = 0;
    dir_item block_buffer[8];
    if (len > 1)
    {
        inode_id = find_inode(path);
    }

    for (int i = 0; i < inode_table[inode_id].size; i++)
    {
        int flag = 0;
        memset(block_buffer, 0, sizeof(block_buffer));
        read_block(inode_table[inode_id].block_point[i], (char *)block_buffer);
        for (int j = 0; j < 8; j++)
        {
            if (block_buffer[j].valid == 0)
            {
                flag = 1;
                break;
            }
            printf("%s  \n", block_buffer[j].name);
        }
        if (flag == 1)
            break;
    }
    return;
}

// 创建文件
int create_file(char *path)
{
    inode inode_table[1024];
    read_inode(inode_table);
    dir_item block_buffer[8];
    char path_buf[20], name_buf[20];
    int father_inode_id = 1;
    int len = strlen(path);
    int inode_id = find_free_inode();
    int flag = 0;
    inode_table[inode_id].file_type = 1;
    memset(name_buf, 0, sizeof(name_buf));
    memset(path_buf, 0, sizeof(path_buf));
    for (int i = len - 1; i >= 0; i--)
    {
        if (i == 0 || path[i] == '/')
        {
            strncpy(name_buf, path + i + 1, len - i - 1);
            strncpy(path_buf, path, i);
            break;
        }
    }
    if (strlen(path_buf) > 1)
    {
        father_inode_id = find_inode(path_buf);
    }
    else
    {
        father_inode_id = 0;
    }
    if (inode_table[father_inode_id].size > 0)
    {
        write_inode(inode_table);
        memset(block_buffer, 0, sizeof(block_buffer));
        read_block(inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1], (char *)block_buffer);
        for (int j = 0; j < 8; j++)
        {
            if (block_buffer[j].valid == 0)
            {
                block_buffer[j].valid = 1;
                block_buffer[j].inode_id = inode_id;
                block_buffer[j].type = 0;
                strcpy(block_buffer[j].name, name_buf);
                write_block(inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1], (char *)block_buffer);
                flag = 1;
                break;
            }
        }
    }
    if (flag == 0)
    {
        inode_table[father_inode_id].size++;
        inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1] = find_free_block();
        write_inode(inode_table);
        memset(block_buffer, 0, sizeof(block_buffer));
        block_buffer[0].valid = 1;
        block_buffer[0].inode_id = inode_id;
        block_buffer[0].type = 0;
        strcpy(block_buffer[0].name, name_buf);
        write_block(inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1], (char *)block_buffer);
    }
    return inode_id;
}

// 创建文件夹
int create_dir(char *path)
{
    inode inode_table[1024];
    read_inode(inode_table);
    dir_item block_buffer[8];
    char path_buf[20], name_buf[20];
    int father_inode_id = 1;
    int len = strlen(path);
    int inode_id = find_free_inode();
    int flag = 0;
    inode_table[inode_id].file_type = 1;
    memset(name_buf, 0, sizeof(name_buf));
    memset(path_buf, 0, sizeof(path_buf));
    for (int i = len - 1; i >= 0; i--)
    {
        if (i == 0 || path[i] == '/')
        {
            strncpy(name_buf, path + i + 1, len - i - 1);
            strncpy(path_buf, path, i);
            break;
        }
    }
    if (strlen(path_buf) > 1)
    {
        father_inode_id = find_inode(path_buf);
    }
    else
    {
        father_inode_id = 0;
    }
    if (inode_table[father_inode_id].size > 0)
    {
        write_inode(inode_table);
        memset(block_buffer, 0, sizeof(block_buffer));
        read_block(inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1], (char *)block_buffer);
        for (int j = 0; j < 8; j++)
        {
            if (block_buffer[j].valid == 0)
            {
                block_buffer[j].valid = 1;
                block_buffer[j].inode_id = inode_id;
                block_buffer[j].type = 1;
                strcpy(block_buffer[j].name, name_buf);
                write_block(inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1], (char *)block_buffer);
                flag = 1;
                break;
            }
        }
    }
    if (flag == 0)
    {
        inode_table[father_inode_id].size++;
        inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1] = find_free_block();
        write_inode(inode_table);
        memset(block_buffer, 0, sizeof(block_buffer));
        block_buffer[0].valid = 1;
        block_buffer[0].inode_id = inode_id;
        block_buffer[0].type = 1;
        strcpy(block_buffer[0].name, name_buf);
        write_block(inode_table[father_inode_id].block_point[inode_table[father_inode_id].size - 1], (char *)block_buffer);
    }
    return inode_id;
}

// 复制文件
void copy_file(char *from, char *to)
{
    inode inode_table[1024];
    read_inode(inode_table);
    int inode_id = create_file(to);
    int from_id = find_inode(from);
    char buf[1024];
    for (int i = 0; i < inode_table[from_id].size; i++)
    {
        read_block(inode_table[from_id].block_point[i], buf);
        inode_table[inode_id].size++;
        int block_id = inode_table[inode_id].block_point[i] = find_free_block();
        write_block(block_id, buf);
    }
}

// 关闭系统
void shutdown()
{
    close_disk();
}