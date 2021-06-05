#pragma once

/*
    NOTE: Below we have a duo type of arena going on:

      - Platform Block Arena which allocates large chunks of memory from the OS on each time we try to expand it
      - Block Arena which suballocates fixed size blocks from the platform block arena and calls it when it needs more space or to free
        space

      The idea is we can have one platform block arena used by many block arenas that allocate for various systems from the same arena. This
      could be implemented as a ton of dynamic arenas but then each one might call the OS since they all expand together and each one might
      have more empty space in it due to page sizes. Having one platform arena and many sub arenas lets us call the OS less often and have
      less empty space potentially due to large OS allocations.
  
 */

//
// NOTE: Platform Block Arena
//

struct block;
struct platform_block_header
{
    // NOTE: Next/Prev blocks in our link
    platform_block_header* Next;
    platform_block_header* Prev;

    // NOTE: Next/Prev blocks in our free list
    platform_block_header* FreeNext;
    platform_block_header* FreePrev;

    mm NumFreeBlocks;
    block* FreeBlocks;
};

struct platform_block_arena
{
    platform_block_header* Next;
    platform_block_header* Prev;

    platform_block_header* FreeList;
    
    mm PlatformBlockSize;
    mm BlockSize;
};

//
// NOTE: Block Arena
//

struct block
{
    platform_block_header* ParentBlock;
    block* Next;
    block* Prev;
};

struct block_arena
{
    // NOTE: Next/Prev blocks we allocated
    block* Next;
    block* Prev;

    mm LastBlockUsed;
    mm BlockSpace; // NOTE: Use this incase we want padding at the end of our block
    platform_block_arena* PlatformArena;
};

//
// =======================================================================================================================================
//

#if 0

struct block_header
{
    block_header* Next;
};

struct block_platform_arena
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

struct block_sentinel : public block
{
    union
    {
        mm NumBytesInLastBlock;
        uint NumElementsInLastBlock;
    };
};

#endif
