#pragma once

// TODO: Add a diff between addresses macro
// TODO: Add a get array index func into platform.h

//
// NOTE: Linear Arena
//

struct linear_arena
{
    mm Size;
    mm Used;
    u8* Mem;
};

struct temp_mem
{
    linear_arena* Arena;
    mm Used;
};

//
// NOTE: Dynamic Arena
//

struct dynamic_arena_header
{
    // NOTE: Stored at the top of pages
    dynamic_arena_header* Next;
    dynamic_arena_header* Prev;
    mm Used;
    mm Size;
};

struct dynamic_arena
{
    // IMPORTANT: We don't do a sentinel cuz then we can't return by value
    dynamic_arena_header* Prev;
    dynamic_arena_header* Next;
    mm MinBlockSize;
};

struct dynamic_temp_mem
{
    dynamic_arena* Arena;
    dynamic_arena_header* Header;
    mm Used;
};

// TODO: Use the below to create a arena that suballocates a dynamic arena, and calls it when we need more memory
//
// NOTE: Block Arena
//

struct block_header
{
    block_header* Next;
};

struct block_arena
{
    mm BlockSize;
    mm NumBlocks;
    
    u8* Mem;
    block_header* FreeBlockHeaders;
};

// NOTE: This is a helper to block arena to have a arena where we have a linked list of arrays
// TODO: Figure out how to make this type safe?
struct block_list_block
{
    u32 NumEntries;
    void* Data;
    block_list_block* Next;
    block_list_block* Prev;
};

#include "memory.cpp"
