
//
// NOTE: Memory functions
//

inline void ZeroMem(void* Mem, mm Size)
{
    u8* CurrByte = (u8*)Mem;
    for (mm Byte = 0; Byte < Size; ++Byte)
    {
        *CurrByte++ = 0;
    }
}

inline void Copy(void* Mem, void* Dest, mm Size)
{
    u8* CurrentByte = (u8*)Mem;
    u8* DestByte = (u8*)Dest;
    for (mm Byte = 0; Byte < Size; ++Byte)
    {
        *DestByte++ = *CurrentByte++;
    }
}

// TODO: Macro to not have to make copies??
#define ShiftPtrByBytes(Ptr, Step, Type) (Type*)ShiftPtrByBytes_((u8*)Ptr, Step)
inline u8* ShiftPtrByBytes_(u8* Ptr, mm Step)
{
    u8* Result = Ptr + Step;
    return Result;
}

inline mm GetAlignOffset(mm Address, mm Alignment)
{
    mm Result = ((Address + (Alignment-1)) & ~(Alignment-1)) - Address;
    return Result;
}

inline mm GetAlignOffset(void* Address, mm Alignment)
{
    mm AddressMm = mm(Address);
    mm Result = ((AddressMm + (Alignment-1)) & ~(Alignment-1)) - AddressMm;
    return Result;
}

inline mm AlignAddress(mm Address, mm Alignment)
{
    Assert(Alignment != 0);
    // IMPORTANT: We assume a power of 2 alignment
    mm Result = (Address + (Alignment-1)) & ~(Alignment-1);

    return Result;
}

//
// NOTE: Linear arena
//

inline linear_arena LinearArenaInit(void* Mem, mm Size)
{
    linear_arena Result = {};
    Result.Size = Size;
    Result.Used = 0;
    Result.Mem = (u8*)Mem;
    
    return Result;
}

inline void ArenaClear(linear_arena* Arena)
{
    Arena->Used = 0;
}

inline temp_mem BeginTempMem(linear_arena* Arena)
{
    // NOTE: This function lets us take all memory allocated past this point and later
    // free it
    temp_mem TempMem = {};
    TempMem.Arena = Arena;
    TempMem.Used = Arena->Used;

    return TempMem;
}

inline void EndTempMem(temp_mem TempMem)
{
    TempMem.Arena->Used = TempMem.Used;
}

#define MemoryExpand_(X) X

#define PushStruct3(Arena, Type, Alignment) (Type*)PushSize(Arena, sizeof(Type), Alignment)
#define PushStruct2(Arena, Type) (Type*)PushSize(Arena, sizeof(Type))
#define PushStructOverload_(_1,_2,_3,NAME,...) NAME
#define PushStruct(...) MemoryExpand_(PushStructOverload_(__VA_ARGS__, PushStruct3, PushStruct2)(__VA_ARGS__))

#define PushArray4(Arena, Type, Count, Alignment) (Type*)PushSize(Arena, sizeof(Type)*(Count), Alignment)
#define PushArray3(Arena, Type, Count) (Type*)PushSize(Arena, sizeof(Type)*(Count))
#define PushArrayOverload_(_1,_2,_3,_4,NAME,...) NAME
#define PushArray(...) MemoryExpand_(PushArrayOverload_(__VA_ARGS__, PushArray4, PushArray3)(__VA_ARGS__))

inline void* PushSize(linear_arena* Arena, mm Size, mm Alignment = 4)
{
    // IMPORTANT: Default Alignment = 4 since ARM requires it
    // IMPORTANT: Its assumed the memory in this allocator is aligned to the highest alignment we will need
    // so we align from the front
    mm AlignedOffset = AlignAddress(Arena->Used, Alignment);
    
    Assert((AlignedOffset + Size) <= Arena->Size);
    void* Result = Arena->Mem + AlignedOffset;
    Arena->Used = AlignedOffset + Size;

#if DEBUG_MEMORY_PROFILING
    DebugRecordAllocation(Arena);
#endif
    
    return Result;
}

inline linear_arena LinearSubArena(linear_arena* Arena, mm Size)
{
    linear_arena Result = {};
    Result.Size = Size;
    Result.Used = 0;
    Result.Mem = (u8*)PushSize(Arena, Size);

    return Result;
}

//
// NOTE: Double linear arena
//

inline linear_double_arena InitDoubleArena(void* Mem, mm Size)
{
    linear_double_arena Result = {};
    Result.Size = Size;
    Result.UsedTop = 0;
    Result.UsedBot = 0;
    Result.Mem = (u8*)Mem;

    return Result;
}

inline void ClearArena(linear_double_arena* Arena)
{
    Arena->UsedTop = 0;
    Arena->UsedBot = 0;
}

inline temp_double_mem BeginTempMem(linear_double_arena* Arena)
{
    // NOTE: This function lets us take all memory allocated past this point and later
    // free it
    temp_double_mem TempMem = {};
    TempMem.Arena = Arena;
    TempMem.UsedTop = Arena->UsedTop;
    TempMem.UsedBot = Arena->UsedBot;

    return TempMem;
}

inline void EndTempMem(temp_double_mem TempMem)
{
    TempMem.Arena->UsedTop = TempMem.UsedTop;
    TempMem.Arena->UsedBot = TempMem.UsedBot;
}

inline void* PushSize(linear_double_arena* Arena, mm Size)
{
    Assert(Arena->UsedTop + Arena->UsedBot + Size <= Arena->Size);
    void* Result = Arena->Mem + Arena->UsedTop;
    Arena->UsedTop += Size;

    // TODO: Do we just wanna zero everything out?
    
    return Result;
}

#define BotPushStruct(Arena, type) (type*)BotPushSize(Arena, sizeof(type))
#define BotPushArray(Arena, type, count) (type*)BotPushSize(Arena, sizeof(type)*count)
inline void* BotPushSize(linear_double_arena* Arena, mm Size)
{
    Assert(Arena->UsedTop + Arena->UsedBot + Size <= Arena->Size);
    Arena->UsedBot += Size;
    void* Result = Arena->Mem + Arena->Size - Arena->UsedBot;
    // TODO: Do we just wanna zero everything out?
    
    return Result;
}

//
// NOTE: Block Arena
//

inline void ArenaClear(block_arena* Arena)
{
    // NOTE: Run through all blocks and set them to free
    Arena->FreeBlockHeaders = (block_header*)Arena->Mem;

    u8* CurrPtr = Arena->Mem;
    for (mm BlockId = 0; BlockId < Arena->NumBlocks; ++BlockId)
    {
        block_header* BlockHeader = (block_header*)CurrPtr;
        
        if (BlockId == (Arena->NumBlocks - 1))
        {
            // NOTE: Set last block to have a null next ptr
            BlockHeader->Next = 0;
        }
        else
        {
            BlockHeader->Next = (block_header*)(CurrPtr + Arena->BlockSize);
        }

        CurrPtr += Arena->BlockSize;
    }
}

inline block_arena BlockArenaInit(void* Mem, mm BlockSize, mm NumBlocks)
{
    Assert(BlockSize >= sizeof(block_header));
    
    block_arena Result = {};
    Result.BlockSize = BlockSize;
    Result.NumBlocks = NumBlocks;
    Result.Mem = (u8*)Mem;

    // NOTE: Set all blocks as free
    ArenaClear(&Result);
    
    return Result;
}

inline block_arena BlockSubArena(linear_arena* Arena, mm BlockSize, mm NumBlocks)
{
    block_arena Result = {};

    void* Mem = (u8*)PushSize(Arena, BlockSize*NumBlocks);
    Result = BlockArenaInit(Mem, BlockSize, NumBlocks);

    return Result;
}

#define PushBlockStruct(Arena, Type) (Type*)PushBlock(Arena)
inline void* PushBlock(block_arena* Arena)
{
    Assert(Arena->FreeBlockHeaders);
    
    // NOTE: Get the first free block
    block_header* BlockHeader = Arena->FreeBlockHeaders;
    Arena->FreeBlockHeaders = Arena->FreeBlockHeaders->Next;

    void* Result = (void*)BlockHeader;
    
    return Result;
}

inline void BlockArenaFreeBlock(block_arena* Arena, void* Mem)
{
    Assert(((uptr(Mem) - uptr(Arena->Mem)) % Arena->BlockSize) == 0);

    block_header* Header = (block_header*)Mem;
    Header->Next = Arena->FreeBlockHeaders;
    Arena->FreeBlockHeaders = Header;
}

//
// NOTE: Block List Arena
//

inline void BlockListInitSentinel(block_list_block* Sentinel)
{
    *Sentinel = {};
    Sentinel->Prev = Sentinel;
    Sentinel->Next = Sentinel;
}

inline b32 BlockListIsEmpty(block_list_block* Sentinel)
{
    b32 Result = Sentinel->Next == Sentinel;
    return Result;
}

inline b32 BlockListIsLast(block_list_block* Sentinel, block_list_block* Block)
{
    b32 Result = Sentinel->Prev == Block;
    return Result;
}

inline block_list_block* BlockListAddBlock(block_arena* Arena, block_list_block* Sentinel, mm Alignment)
{
    block_list_block* Result = PushBlockStruct(Arena, block_list_block);
    *Result = {};
    mm AlignedOffset = AlignAddress(sizeof(block_list_block), Alignment);
    Result->Data = (u8*)Result + AlignedOffset;

    // NOTE: Append to the end of the list
    Result->Prev = Sentinel->Prev;
    Result->Next = Sentinel;

    Result->Prev->Next = Result;
    Result->Next->Prev = Result;

    return Result;
}

inline void BlockListFreeBlock(block_arena* Arena, block_list_block* Block)
{
    // NOTE: Remove ourselves from the list
    Block->Prev->Next = Block->Next;
    Block->Next->Prev = Block->Prev;

    BlockArenaFreeBlock(Arena, Block);
}

inline void BlockListClear(block_arena* Arena, block_list_block* Sentinel)
{
    block_list_block* CurrEntry = Sentinel->Next;
    while (CurrEntry != Sentinel)
    {
        block_list_block* SavedCurrEntry = CurrEntry;
        CurrEntry = CurrEntry->Next;
        BlockListFreeBlock(Arena, SavedCurrEntry);
    }
}

#define BlockListAddEntry(Arena, Sentinel, Type) (Type*)BlockListAddEntry_(Arena, Sentinel, sizeof(Type), 1)
#define BlockListAddEntryAligned(Arena, Sentinel, Type, Alignment) (Type*)BlockListAddEntry_(Arena, Sentinel, sizeof(Type), Alignment)
inline void* BlockListAddEntry_(block_arena* Arena, block_list_block* Sentinel, mm TypeSize, mm Alignment)
{
    Assert((TypeSize % Alignment) == 0);
    void* Result = 0;

    // IMPORTANT: We assume that we are aligned to a value less than our data size :P
    mm AlignedStart = AlignAddress(sizeof(block_list_block), Alignment);
    mm MaxNumEntries = mm(f32(Arena->BlockSize - AlignedStart) / f32(TypeSize));
    block_list_block* LastBlock = Sentinel->Prev;
    if (BlockListIsEmpty(Sentinel) || LastBlock->NumEntries == MaxNumEntries)
    {
        // NOTE: We need to allocate a new block
        block_list_block* NewHeader = BlockListAddBlock(Arena, Sentinel, Alignment);
        Result = (u8*)NewHeader->Data + NewHeader->NumEntries*TypeSize;
        NewHeader->NumEntries += 1;
    }
    else
    {
        // NOTE: We can grab a entry from the current last block
        Result = (u8*)LastBlock->Data + LastBlock->NumEntries*TypeSize;
        LastBlock->NumEntries += 1;
    }

    return Result;
}