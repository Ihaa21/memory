#pragma once

// TODO: Add a diff between addresses macro
// TODO: Add a get array index func into platform.h

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

struct linear_double_arena
{
    mm Size;
    mm UsedTop;
    mm UsedBot;
    u8* Mem;
};

struct temp_double_mem
{
    linear_double_arena* Arena;
    mm UsedTop;
    mm UsedBot;
};

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
