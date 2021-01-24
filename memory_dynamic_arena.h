#pragma once

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

