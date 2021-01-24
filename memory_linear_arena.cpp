
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
