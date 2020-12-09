
#include "math\types.h"

//
// NOTE: Memory functions
//

inline void* MemoryAllocate(mm AllocSize)
{
    void* Result = VirtualAlloc(0, AllocSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return Result;
}

inline void MemoryFree(void* Mem)
{
    BOOL Result = VirtualFree(Mem, 0, MEM_RELEASE);
    DWORD Error = GetLastError();
    Assert(Result);
}

inline void ZeroMem(void* Mem, mm Size)
{
    u8* CurrByte = (u8*)Mem;
    for (mm Byte = 0; Byte < Size; ++Byte)
    {
        *CurrByte++ = 0;
    }
}

#define CopyArray(Mem, Dest, Type, Count) Copy(Mem, Dest, sizeof(Type)*(Count))
inline void Copy(const void* Mem, void* Dest, mm Size)
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

inline u32 GetAlignOffset(u32 Address, u32 Alignment)
{
    Assert(Alignment != 0);
    u32 Result = ((Address + (Alignment-1)) & ~(Alignment-1)) - Address;
    return Result;
}

inline u64 GetAlignOffset(u64 Address, u64 Alignment)
{
    Assert(Alignment != 0);
    u64 Result = ((Address + (Alignment-1)) & ~(Alignment-1)) - Address;
    return Result;
}

inline mm GetAlignOffset(void* Address, mm Alignment)
{
    Assert(Alignment != 0);
    mm AddressMm = mm(Address);
    mm Result = GetAlignOffset(AddressMm, Alignment);
    return Result;
}

inline u32 AlignAddress(u32 Address, u32 Alignment)
{
    // IMPORTANT: We assume a power of 2 alignment
    u32 Result = Address + GetAlignOffset(Address, Alignment);

    return Result;
}

inline u64 AlignAddress(u64 Address, u64 Alignment)
{
    // IMPORTANT: We assume a power of 2 alignment
    u64 Result = Address + GetAlignOffset(Address, Alignment);

    return Result;
}

inline mm AlignAddress(void* Address, mm Alignment)
{
    mm Result = mm(Address) + GetAlignOffset(mm(Address), Alignment);
    return Result;
}

//
// NOTE: Data Structure Macros
//

#define LinkedListSentinelCreate(Sentinel)      \
    {                                           \
        (Sentinel).Prev = &(Sentinel);          \
        (Sentinel).Next = &(Sentinel);          \
    }

#define LinkedListSentinelAppend(Sentinel, NewElement)  \
    {                                                   \
        (NewElement)->Prev = (Sentinel).Prev;           \
        (NewElement)->Next = &(Sentinel);               \
        (NewElement)->Next->Prev = NewElement;          \
        (NewElement)->Prev->Next = NewElement;          \
    }

#define LinkedListSentinelRemove(RemoveElement)                 \
    {                                                           \
        (RemoveElement)->Next->Prev = (RemoveElement)->Prev;    \
        (RemoveElement)->Prev->Next = (RemoveElement)->Next;    \
    }

#define LinkedListSentinelFirst(Sentinel)       \
    ((Sentinel).Next)

#define LinkedListSentinelLast(Sentinel)        \
    ((Sentinel).Prev)

#define LinkedListSentinelEmpty(Sentinel)       \
    ((Sentinel).Prev == &(Sentinel))

//
// NOTE: Memory Alloc Macros
//

#define PushStruct(Arena, Type) (Type*)PushSizeAligned(Arena, sizeof(Type), 1)
#define PushStructAligned(Arena, Type, Alignment) (Type*)PushSizeAligned(Arena, sizeof(Type), Alignment)

#define PushArray(Arena, Type, Count) (Type*)PushSizeAligned(Arena, sizeof(Type)*(Count), 1)
#define PushArrayAligned(Arena, Type, Count, Alignment) (Type*)PushSizeAligned(Arena, sizeof(Type)*(Count), Alignment)

#define PushSize(Arena, Size) PushSizeAligned(Arena, Size, 1)

//
// NOTE: Linear arena
//

inline linear_arena LinearArenaCreate(void* Mem, mm Size)
{
    linear_arena Result = {};
    Result.Size = Size;
    Result.Used = 0;
    Result.Mem = (u8*)Mem;
    
    return Result;
}

inline void LinearArenaClear(linear_arena* Arena)
{
    Arena->Used = 0;
}

inline mm LinearArenaGetRemainingSize(linear_arena* Arena)
{
    mm Result = Arena->Size - Arena->Used;
    return Result;
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

inline void* PushSizeAligned(linear_arena* Arena, mm Size, mm Alignment)
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

#define PushString(Arena, String) PushStringAligned(Arena, String, 1)
inline char* PushStringAligned(linear_arena* Arena, char* String, mm Alignment)
{
    char* Result = 0;
    
    mm AlignedOffset = AlignAddress(Arena->Used, Alignment);
    Assert(AlignedOffset <= Arena->Size);
    Result = (char*)(Arena->Mem + AlignedOffset);

    char* SrcChar = String;
    char* DstChar = Result;
    while (*SrcChar != 0)
    {
        *DstChar++ = *SrcChar++;
    }
    *DstChar++ = 0;

    mm StringSize = mm(DstChar - Result);
    Assert(AlignedOffset + StringSize <= Arena->Size);
    Arena->Used = AlignedOffset + StringSize;

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
// NOTE: Dynamic Arena
//

inline mm DynamicArenaGetBlockSize(mm AllocSize)
{
    // NOTE: We allocate to nearest page size, so +1 to fit the header
    // TODO: Get page size on other platforms here
    mm PageSize = KiloBytes(4);
    mm NumPages = mm(CeilF32(f32(AllocSize) / f32(PageSize))) + 1;
    mm Result = PageSize * NumPages;

    return Result;
}

inline dynamic_arena_header* DynamicArenaAllocHeader(mm Size)
{
    mm AllocSize = DynamicArenaGetBlockSize(Size);
    dynamic_arena_header* Result = (dynamic_arena_header*)MemoryAllocate(AllocSize);
    Result->Used = sizeof(dynamic_arena_header);
    Result->Size = AllocSize;

    return Result;
}

inline dynamic_arena DynamicArenaCreate(mm MinBlockSize)
{
    dynamic_arena Result = {};
    Result.MinBlockSize = MinBlockSize;

    return Result;
}

// TODO: Make size/used be hidden? Or just don't use push to put the header, it complicates eveyrthing
inline mm DynamicArenaHeaderGetSize(dynamic_arena_header* Header)
{
    mm Result = Header->Used - sizeof(*Header);
    return Result;
}

inline void* DynamicArenaHeaderGetData(dynamic_arena_header* Header)
{
    // TODO: We don't take into account alignment here, its probably better to move headers to the bottom so alignment is more predictable
    void* Result = (void*)(Header + 1);
    return Result;
}

inline void* PushSizeAligned(dynamic_arena* Arena, mm Size, mm Alignment = 4)
{
    void* Result = 0;
    dynamic_arena_header* Header = Arena->Prev;
    
    // IMPORTANT: Default Alignment = 4 since ARM requires it
    mm AlignedOffset = Header ? AlignAddress(Header->Used, Alignment) : 0;
    if (!Header || (AlignedOffset + Size) > Header->Size)
    {
        // NOTE: Allocate a new block
        dynamic_arena_header* NewHeader = DynamicArenaAllocHeader(Max(Size, Arena->MinBlockSize));

        // NOTE: Append to linked list
        if (Arena->Prev)
        {
            NewHeader->Prev = Arena->Prev;
            Arena->Prev->Next = NewHeader;
        }
        Arena->Prev = NewHeader;
        if (!Arena->Next)
        {
            Arena->Next = NewHeader;
        }
        
        Header = NewHeader;
        AlignedOffset = AlignAddress(Header->Used, Alignment);
    }

    // NOTE: Suballocate a page
    u8* BasePtr = (u8*)Header;
    Result = BasePtr + AlignedOffset;
    Header->Used = AlignedOffset + Size;
    
#if DEBUG_MEMORY_PROFILING
    DebugRecordAllocation(Arena);
#endif

    return Result;
}

inline void ArenaClear(dynamic_arena* Arena)
{
    for (dynamic_arena_header* Header = Arena->Next;
         Header;
         )
    {
        dynamic_arena_header* CurrHeader = Header;
        Header = Header->Next;

        // NOTE: Remove from linked list
        if (CurrHeader->Prev)
        {
            CurrHeader->Prev->Next = CurrHeader->Next;
        }
        else
        {
            Arena->Prev = 0;
        }
        if (CurrHeader->Next)
        {
            CurrHeader->Next->Prev = CurrHeader->Prev;
        }
        else
        {
            Arena->Next = 0;
        }
        
        MemoryFree(CurrHeader);
    }
}

inline dynamic_temp_mem BeginTempMem(dynamic_arena* Arena)
{
    dynamic_temp_mem Result = {};
    Result.Arena = Arena;
    Result.Header = Arena->Prev;
    Result.Used = Result.Header ? Result.Header->Used : 0;

    return Result;
};

inline void EndTempMem(dynamic_temp_mem TempMem)
{
    dynamic_arena_header* Header = TempMem.Arena->Prev;
    while(Header != TempMem.Header)
    {
        dynamic_arena_header* CurrHeader = Header;
        Header = Header->Prev;

        // NOTE: Remove from linked list
        if (CurrHeader->Prev)
        {
            CurrHeader->Prev->Next = CurrHeader->Next;
        }
        else
        {
            TempMem.Arena->Prev = 0;
        }
        if (CurrHeader->Next)
        {
            CurrHeader->Next->Prev = CurrHeader->Prev;
        }
        else
        {
            TempMem.Arena->Next = 0;
        }

        MemoryFree(CurrHeader);
    }

    if (Header)
    {
        Header->Used = TempMem.Used;
    }
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

inline block_arena BlockArenaCreate(void* Mem, mm BlockSize, mm NumBlocks)
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
    Result = BlockArenaCreate(Mem, BlockSize, NumBlocks);

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
