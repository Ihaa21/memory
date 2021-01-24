#pragma once

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

