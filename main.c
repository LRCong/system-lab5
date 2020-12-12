#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <disk.h>

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

sp_block *spBlock;

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

// 找到空闲inode
int find_free_inode()
{
    int num = -1;
    uint16_t buf;
    for (int i = 0; i < 32; i++)
    {
        for (int j = 0; j < 32; j++)
        {
            buf = (spBlock->inode_map[i] >> (31 - j)) & (uint16_t)1;
            if (buf == 0)
            {
                num = i * 32 + j;
                spBlock->inode_map[i] = spBlock->inode_map[i] | (1 << (31 - j));
            }
        }
    }
}

// 找到一个空闲块
int find_free_block();

// 初始化fs, 如果不是本文件系统，则初始化
void fs_init()
{
    // int i;
    char *p;
    char buf[100];
    dir_item block_buffer[8];
    inode inode_table[32];
    spBlock = (sp_block *)malloc(sizeof(sp_block));
    open_disk();
    read_block(0, buf);
    // 读取超级块
    p = spBlock;
    for (int i = 0; i < sizeof(sp_block) / 8; i++)
    {
        *p = buf[i];
        p++;
    }
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

        spBlock->inode_map[0] = (1 << 32);

        // 初始化第一个inode
        inode_table[0].block_point[0] = 33;
        inode_table[0].file_type = 1;
        inode_table[0].size = 1;

        //初始化两个block '.'与'..'
        memset(block_buffer, 0, 2 * DEVICE_BLOCK_SIZE);
        block_buffer[0].inode_id = 0;
        strcpy(block_buffer[0].name, ".");
        block_buffer[0].type = 0;
        block_buffer[0].valid = 1;

        block_buffer[1].inode_id = 0;
        strcpy(block_buffer[1].name, "..");
        block_buffer[1].type = 0;
        block_buffer[1].valid = 1;

        write_block(33, block_buffer);
        write_block(1, inode_table);
        write_block(0, spBlock);
    }
    return;
}

// 列出文件
void ls(char *path);

// 创建文件
void create_file(char *path, int size);

// 复制文件
void copy_file(char *from, char *to);

// 关闭系统
void shutdown();