
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
        DoubleListAppend(Arena, NewHeader, Next, Prev);
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
        DoubleListRemove(Arena, CurrHeader, Next, Prev);        
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
