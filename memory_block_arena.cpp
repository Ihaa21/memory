
//
// NOTE: Block Platform Arena
//

/*

  Block arena is a linked list of memory blocks, each some large value (like a megabyte or more). The idea is that each block is
  going to be partitioned into smaller blocks that various other systems can allocate. We also want these systems to have the ability
  to free these allocations, which just adds these blocks to a free list. If all blocks in a allocation are free, we free the entire
  large allocation. 
  
 */

inline mm PlatformBlockArenaNumBlocks(platform_block_arena* Arena)
{
    mm Result = (Arena->PlatformBlockSize - sizeof(platform_block_header)) / Arena->BlockSize;
    return Result;
}

inline platform_block_arena PlatformBlockArenaCreate(mm PlatformBlockSize, mm NumBlocks)
{
    platform_block_arena Result = {};
    Result.PlatformBlockSize = PlatformBlockSize;
    Result.BlockSize = (PlatformBlockSize - sizeof(platform_block_header)) / NumBlocks;

    return Result;
}

inline block* PlatformBlockArenaAllocate(platform_block_arena* Arena)
{
    block* Result = 0;
    
    if (Arena->FreeList)
    {
        // NOTE: Free list has space so grab a free block
        platform_block_header* CurrPlatformHeader = Arena->FreeList;
        Assert(CurrPlatformHeader->FreeBlocks && CurrPlatformHeader->NumFreeBlocks > 0);
        CurrPlatformHeader->NumFreeBlocks -= 1;
        if (CurrPlatformHeader->NumFreeBlocks == 0)
        {
            // NOTE: Remove the platform block from free list
            FreeListRemove(Arena->FreeList, CurrPlatformHeader, FreeNext, FreePrev);
        }

        // NOTE: Remove the block from the free list
        Result = CurrPlatformHeader->FreeBlocks;
        FreeListRemove(CurrPlatformHeader->FreeBlocks, Result, Next, Prev);
        *Result = {};
        Result->ParentBlock = CurrPlatformHeader;
    }
    else
    {
        // NOTE: Free list is empty so allocate a new platform block
        platform_block_header* PlatformHeader = (platform_block_header*)MemoryAllocate(Arena->PlatformBlockSize);
        *PlatformHeader = {};
        PlatformHeader->NumFreeBlocks = PlatformBlockArenaNumBlocks(Arena) - 1;

        // NOTE: Chain header to our list of headers
        DoubleListAppend(Arena, PlatformHeader, Next, Prev);
        
        // NOTE: Grab first block for our result (its next and prev will get linked by block arena)
        mm BlockId = 0;
        Result = (block*)(PlatformHeader + 1);
        Result->ParentBlock = PlatformHeader;
        BlockId += 1;
        
        // NOTE: Add all blocks except first to free list
        if (BlockId < PlatformBlockArenaNumBlocks(Arena))
        {
            // NOTE: Chain header to our list of headers with free space
            // TODO: Make this a macro
            PlatformHeader->FreeNext = Arena->FreeList;
            PlatformHeader->FreePrev = 0;
            if (PlatformHeader->FreeNext)
            {
                PlatformHeader->FreeNext->FreePrev = PlatformHeader;
            }
            Arena->FreeList = PlatformHeader;

            u8* StartBlockPointer = (u8*)Result + Arena->BlockSize;
            PlatformHeader->FreeBlocks = (block*)StartBlockPointer;
            PlatformHeader->FreeBlocks->Prev = 0;
            PlatformHeader->FreeBlocks->ParentBlock = PlatformHeader;
            StartBlockPointer += Arena->BlockSize;
            BlockId += 1;

            for (; BlockId < PlatformHeader->NumFreeBlocks; ++BlockId, StartBlockPointer += Arena->BlockSize)
            {
                block* CurrFreeBlock = (block*)StartBlockPointer;
                CurrFreeBlock->ParentBlock = PlatformHeader;
                CurrFreeBlock->Next = (block*)(StartBlockPointer + Arena->BlockSize);
                CurrFreeBlock->Prev = (block*)(StartBlockPointer - Arena->BlockSize);
                CurrFreeBlock->Prev->Next = CurrFreeBlock;
            }

            block* CurrFreeBlock = (block*)StartBlockPointer;
            CurrFreeBlock->ParentBlock = PlatformHeader;
            CurrFreeBlock->Next = 0;
            CurrFreeBlock->Prev = (block*)(StartBlockPointer - Arena->BlockSize);
            CurrFreeBlock->Prev->Next = CurrFreeBlock;
        }        
    }

    return Result;
}

inline void PlatformBlockArenaFree(platform_block_arena* Arena, block* Block)
{
    platform_block_header* PlatformHeader = Block->ParentBlock;

    PlatformHeader->NumFreeBlocks += 1;
    if (PlatformHeader->NumFreeBlocks == PlatformBlockArenaNumBlocks(Arena))
    {
        // NOTE: This platform block is completely empty so we can free it

        // NOTE: Unlink from list of platform blocks
        DoubleListRemove(Arena, PlatformHeader, Next, Prev);
#if 0
        // TODO: Make this a macro
        if (PlatformHeader->Prev)
        {
            PlatformHeader->Prev->Next = PlatformHeader->Next;
        }
        else
        {
            Arena->Prev = 0;
        }
        
        if (PlatformHeader->Next)
        {
            PlatformHeader->Next->Prev = PlatformHeader->Prev;
        }
        else
        {
            Arena->Next = 0;
        }
#endif
        
        // NOTE: Unlink from list of platform free blocks
        if (PlatformHeader->FreeNext)
        {
            PlatformHeader->FreeNext->FreePrev = PlatformHeader->FreePrev;
        }
        if (!PlatformHeader->Prev)
        {
            Arena->FreeList = PlatformHeader->FreeNext;
        }
        
        MemoryFree(PlatformHeader);
    }
    else
    {
        // NOTE: We still have blocks in use so add to free list
        // TODO: Make this a macro
        Block->Next = PlatformHeader->FreeBlocks;
        if (Block->Next)
        {
            Block->Next->Prev = Block;
        }
        Block->Prev = 0;
    }
}

inline void ArenaClear(platform_block_arena* Arena)
{
    for (platform_block_header* Header = Arena->Next;
         Header;
         )
    {
        platform_block_header* CurrHeader = Header;
        Header = Header->Next;

        // NOTE: Remove from linked list
        DoubleListRemove(Arena, CurrHeader, Next, Prev);        
        MemoryFree(CurrHeader);
    }

    Arena->FreeList = 0;
}

//
// NOTE: Block Arena
//

inline mm BlockArenaGetBlockSize(block_arena* Arena)
{
    mm Result = Arena->BlockSpace;
    return Result;
}

inline block_arena BlockArenaCreate(platform_block_arena* PlatformArena)
{
    block_arena Result = {};
    Result.PlatformArena = PlatformArena;
    Result.BlockSpace = (PlatformArena->BlockSize - sizeof(block));

    return Result;
}

inline block_arena BlockArenaCreate(platform_block_arena* PlatformArena, mm ElementSize)
{
    block_arena Result = {};
    Result.PlatformArena = PlatformArena;
    Result.BlockSpace = ((PlatformArena->BlockSize - sizeof(block)) / ElementSize) * ElementSize;

    return Result;
}

inline void* PushSizeAligned(block_arena* Arena, mm Size, mm Alignment = 4)
{
    Assert(Size <= BlockArenaGetBlockSize(Arena));
    
    mm NewUsed = AlignAddress(Arena->LastBlockUsed, Alignment) + Size;
    if (NewUsed > (Arena->BlockSpace + sizeof(block)) || !Arena->Next)
    {
        // NOTE: Allocate a new block, no more empty space in arena
        block* NewBlock = PlatformBlockArenaAllocate(Arena->PlatformArena);
        DoubleListAppend(Arena, NewBlock, Next, Prev);
        Arena->LastBlockUsed = sizeof(block);
    }

    void* Result = (void*)AlignAddress((u8*)Arena->Prev + Arena->LastBlockUsed, Alignment);
    Arena->LastBlockUsed = mm(Result) - mm(Arena->Prev) + Size;
    
    return Result;
}

inline void* FreeSize()
{
    // TODO: Implement for ECS (not needed for UI)
}

inline void ArenaClear(block_arena* Arena)
{
    // NOTE: Free all allocated blocks (unless platform arena already cleared)
    if (Arena->PlatformArena->Next)
    {
        for (block* CurrBlock = Arena->Next; CurrBlock; CurrBlock = Arena->Next)
        {
            DoubleListRemove(Arena, CurrBlock, Next, Prev);
            PlatformBlockArenaFree(Arena->PlatformArena, CurrBlock);
        }
    }
    
    Arena->Next = 0;
    Arena->Prev = 0;
    Arena->LastBlockUsed = 0;
}

#define BlockGetData(block, type) (type*)BlockGetData_(block)
inline void* BlockGetData_(block* Block)
{
    void* Result = (void*)(Block + 1);
    return Result;
}
