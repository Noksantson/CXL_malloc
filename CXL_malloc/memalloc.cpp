#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <iostream>

uint64_t pool_start_address = 0x660000000000;
uint64_t pool_end_address   = 0x800000000000;

struct block {
    size_t size;
    struct block *next;
    struct block *prev;
};

size_t roundup(size_t size) {
    const size_t alignment = 8;  // 对齐到 8 字节
    if (size % alignment == 0) {
        return size;  // 已经对齐
    }
    return (size + alignment - 1) & ~(alignment - 1);
}

size_t get_size(struct block *block) {
    return block->size - (block->size & 0x7);
}

bool get_allocated(struct block *block) { return (block->size & 0x1); }

void set_size(struct block *block, size_t size) {
    block->size &= 0x7;
    block->size |= size;
}

void set_allocated(struct block *block, bool allocated) {
    block->size -= block->size & 0x7;
    block->size |= allocated;
}

struct block *get_next_block(struct block *block) {
    return block->next;
}

struct block *get_prev_block(struct block *block) {
    return block->prev;
}

void split(struct block* block, size_t size) {
    size_t blocksize = get_size(block);
    if (blocksize - size < sizeof(struct block*))
        return;

    struct block *next = block + size;
    set_size(block, size);
    set_allocated(block, true);

    set_size(next, blocksize - size);
    set_allocated(next, false);

    block->next = next;
    next->prev = block;
    next->next = nullptr;
}

void merge(struct block* block, struct block* next) {
    size_t size1 = get_size(block);
    size_t size2 = get_size(next);

    set_size(block, size1 + size2);
    set_size(next, 0);

    block->next = next->next;
    block->next->prev = block;
    
}

void init() {
    struct block *block = (struct block *)pool_start_address;
    block->next = nullptr;
    block->prev = nullptr;
    size_t size = roundup(pool_start_address - pool_end_address - 8);
    set_size(block, size);
    set_allocated(block, false);
}

void* malloc(size_t reqsize) {
    struct block *block = (struct block *)pool_start_address;
    size_t size = roundup(reqsize) + sizeof(struct block);
    while (block)
    {
        size_t blocksize = get_size(block);
        if (get_allocated(block) || blocksize < size) {
            block = block->next;
            continue;
        }
        set_allocated(block, true);
        if (blocksize > size)
            split(block, size);
        break;
    }
    if (block != nullptr) {
        return (void *)(block - 1);
    }
    return nullptr;
}

void free(void *ptr) {
    struct block *block = (struct block*)ptr;
    if (!get_allocated(block))
        return;

    struct block *next = get_next_block(block);
    struct block *prev = get_prev_block(block);

    bool next_allocated = get_allocated(next);
    bool prev_allocated = get_allocated(prev);

    set_allocated(block, false);

    if (!next_allocated && prev_allocated) {
        merge(block, next);
    } else if (next_allocated && !prev_allocated) {
        merge(prev, block);
    } else if (!next_allocated && !prev_allocated) {
        merge(prev, block);
        merge(prev, next);
    }
}

int main() {
    init();
    
    return 0;
}

