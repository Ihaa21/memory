
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
// NOTE: Free List Macros
//

#define FreeListAppend()

#define FreeListRemove(FreeList, RemoveElement, Next, Prev) \
    if ((RemoveElement)->Next)                              \
    {                                                       \
        (RemoveElement)->Next->Prev = 0;                    \
    }                                                       \
    (FreeList) = (RemoveElement)->Next;                     \


//
// NOTE: Double Linked List Macros
//

#define DoubleListAppend(List, Element, Next, Prev) \
    if ((List)->Prev)                               \
    {                                               \
        (Element)->Prev = (List)->Prev;             \
        (List)->Prev->Next = (Element);             \
    }                                               \
    (List)->Prev = (Element);                       \
    if (!(List)->Next)                              \
    {                                               \
        (List)->Next = (Element);                   \
    }                                               \

#define DoubleListRemove(List, Element, Next, Prev) \
    if (!(Element)->Prev)                           \
    {                                               \
        (List)->Next = (Element)->Next;             \
    }                                               \
    else                                            \
    {                                               \
        (Element)->Prev->Next = (Element)->Next;    \
    }                                               \
    if (!(Element)->Next)                           \
    {                                               \
        (List)->Prev = (Element)->Prev;             \
    }                                               \
    else                                            \
    {                                               \
        (Element)->Next->Prev = (Element)->Prev;    \
    }                                               \
         
//
// NOTE: Linked List Sentinel Macros
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

#include "memory_linear_arena.cpp"
#include "memory_dynamic_arena.cpp"
#include "memory_block_arena.cpp"
