#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <iostream>
#include <unordered_map>

#define MAX_ORDER 
#define NODENUM 255
#define MB 1024*1024
#define MAXarea 256*1024*1024

#define LEFT_LEAF(index) ((index) * 2 + 1)
#define RIGHT_LEAF(index) ((index) * 2 + 2)
#define PARENT(index) ( ((index) + 1) / 2 - 1)

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct list_head {
    struct list_head* next, * prev;
};

struct CXL_free_block_head
{
    struct list_head free_block_list;
    int nr_free;
};

struct CXL_area {
    int key;  // 内存标识
    /*be able to alloc max size=buddy_size[max_index]*/
    int max_index;
    void* addr;
    uint64_t longest[NODENUM];
    struct CXL_free_block_head block_head[MAX_ORDER];
};

struct CXL_free_block {
    void* addr;
    size_t size;
    int key;
    struct list_head free_block_entry;

};

/*-Host本地细化内存信息表表项数据结构-*/
struct objtable_entry {
    int key;                            // 内存块标识
    struct CXL_free_block* free_block;  // 对象虚拟地址
    size_t
        size;  // 对象大小（这个大小是分配器分配出的大小，而不是用户请求的大小）
    bool isshared;
    int master_id;  // 管理主机id，通过判断当前进程所属host_id==master_id?来判断锁表是否在本地
    int client_id;
    int lock_host;  // 加锁的主机，-1 表示没有被加锁
};


struct memtable_entry
{

};
// 2MB对齐函数
size_t roundup(size_t size) {
    const size_t alignment = 2 * 1024 * 1024; // 2MB
    return (size + alignment - 1) & ~(alignment - 1);
}
//将传入的size修改为2的幂（>=1）MB的size，符合buddy块的大小
size_t fix_size(size_t size) {
    // 将大小转换为MB并确保至少为2MB
    size_t mbSize = (size + (1024 * 1024) - 1) / (1024 * 1024);

    // 确保至少为2MB
    if (mbSize < 2) {
        mbSize = 2;
    }

    // 找到下一个 2 的幂
    size_t newSize = 1;
    while (newSize < mbSize) {
        newSize *= 2;
    }

    // 返回以字节为单位的结果
    return newSize * (1024 * 1024);
}

