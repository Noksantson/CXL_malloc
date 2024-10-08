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
    int key;  // �ڴ��ʶ
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

/*-Host����ϸ���ڴ���Ϣ��������ݽṹ-*/
struct objtable_entry {
    int key;                            // �ڴ���ʶ
    struct CXL_free_block* free_block;  // ���������ַ
    size_t
        size;  // �����С�������С�Ƿ�����������Ĵ�С���������û�����Ĵ�С��
    bool isshared;
    int master_id;  // ��������id��ͨ���жϵ�ǰ��������host_id==master_id?���ж������Ƿ��ڱ���
    int client_id;
    int lock_host;  // ������������-1 ��ʾû�б�����
};


struct memtable_entry
{

};
// 2MB���뺯��
size_t roundup(size_t size) {
    const size_t alignment = 2 * 1024 * 1024; // 2MB
    return (size + alignment - 1) & ~(alignment - 1);
}
//�������size�޸�Ϊ2���ݣ�>=1��MB��size������buddy��Ĵ�С
size_t fix_size(size_t size) {
    // ����Сת��ΪMB��ȷ������Ϊ2MB
    size_t mbSize = (size + (1024 * 1024) - 1) / (1024 * 1024);

    // ȷ������Ϊ2MB
    if (mbSize < 2) {
        mbSize = 2;
    }

    // �ҵ���һ�� 2 ����
    size_t newSize = 1;
    while (newSize < mbSize) {
        newSize *= 2;
    }

    // �������ֽ�Ϊ��λ�Ľ��
    return newSize * (1024 * 1024);
}

