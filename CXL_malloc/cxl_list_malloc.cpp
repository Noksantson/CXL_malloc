#include "CXL_api.h"

/*
find_block_api
struct CXL_area* find_block(
    const std::unordered_map<int, memtable_entry*>& memtable, int size,
    bool isshared, int host_id)
*/

/*
auto obj_entry = new objtable_entry{
                mtbl_entry->key, nullptr, mtbl_entry->size, is_shared, host_id,
                client_id,       -1};
struct CXL_area* area = find_block(memtable, size, is_shared, host_id);
auto obj_entry = cxl_small_malloc(area, size, is_shared, host_id);
*/

/*-Host本地细化内存信息表表项数据结构-*/

void init_area(struct CXL_area* area) {
    //判断参数是否合法
    if (area == nullptr) return;
    //每一个area的大小为256字节
    uint64_t node_size = 256 * MB;
    //初始化area结构体中用于维护分配和释放的二叉树节点的值
    for (int i = 0; i < NODENUM; i++) {
        if (IS_POWER_OF_2(i + 1))
            node_size /= 2;
        area->longest[i] = node_size;
    }
}

struct objtable_entry* cxl_small_malloc(CXL_area* area, size_t size, bool is_shared, int host_id) {
    //判断参数是否合法
    if (area == nullptr || size <= 0)
        return nullptr;
    size_t fixsize = fix_size(size);
    uint64_t index = 0;//从0号节点开始往下递归
    uint64_t nodesize = MAXarea;//
    uint64_t offset = 0;//找到相对偏移量
    //如果area中的最大连续内存不够时返回nullptr
    if (area->longest[index] < fixsize)
        return nullptr;
    //二叉树向下寻找到合适的节点
    for (;nodesize != fixsize; nodesize /= 2) {
        if (area->longest[LEFT_LEAF(index)] >= fixsize)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);
    }
    if (index >= NODENUM) 
        return nullptr;
    //找到节点后维护节点信息，计算相对偏移地址
    area->longest[index] = 0;
    offset = (index + 1) * nodesize - MAXarea;
    //二叉树向上维护祖先节点信息
    while (index) {
        index = PARENT(index);
        area->longest[index] = MAX(area->longest[LEFT_LEAF(index)], area->longest[RIGHT_LEAF(index)]);
    }
    //计算返回的虚拟地址
    uint64_t retaddr = offset + (uint64_t)(area->addr);
    //打包结构体
    struct list_head free_block_entry;
    free_block_entry.next = nullptr;
    free_block_entry.prev = nullptr;
    struct CXL_free_block* free_block = new CXL_free_block();
    free_block->addr = (void*)retaddr;
    free_block->key = area->key;
    free_block->size = fixsize;
    free_block->free_block_entry = free_block_entry;
    struct objtable_entry* obj = new objtable_entry();
    obj->isshared = is_shared;
    obj->client_id = host_id;
    obj->key = area->key;
    obj->size = fixsize;
    obj->free_block = free_block;
    obj->lock_host = 1;
    return obj;
}


bool CXL_small_free(struct objtable_entry* obj, struct CXL_area* area) {
    if (obj == nullptr || area == nullptr)
        return false;
    //free时从二叉树叶节点往上找
    uint64_t nodesize = 2*MB;
    uint64_t index = 0;
    uint64_t left_longest, right_longest;
    //计算相对偏移地址
    uint64_t offset = (uint64_t)(obj->free_block->addr) - (uint64_t)(area->addr);
    //通过相对偏移地址找到第一个被分配出去的叶节点
    index = offset + MAXarea - 1;
    //向上找到值为0的祖先节点
    for (;area->longest[index];index = PARENT(index)) {
        nodesize *= 2;
        if (index == 0)
            return false;
    }
    area->longest[index] = nodesize;
    //进行buddy算法的合并兄弟节点
    while (index) {
        index = PARENT(index);
        nodesize *= 2;
        left_longest = area->longest[LEFT_LEAF(index)];
        right_longest = area->longest[RIGHT_LEAF(index)];
        if (left_longest + right_longest == nodesize)
            area->longest[index] = nodesize;
        else
            area->longest[index] = MAX(left_longest, right_longest);

    }
    return true;
}
/*
* int main() {

    return 0;
}
*/






