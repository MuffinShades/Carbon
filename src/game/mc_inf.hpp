#include <iostream>
#include "../msutil.hpp"
#include "../vec.hpp"

enum class BlockID {
    Air,
    Grass,
    Dirt,
    CommandBlock,
    Unknown
};

struct BlockInf {
    uvec2 tex[6];
};

#define DEF_BLOCK(id, texX, texY)

const BlockInf mc_blockData[] = {
    //Air 
    {},
    //grass
    {
        {
            {3,0},{3,0},
            {3,0},{3,0},
            {0,0},{3,0}
        }
    },
    //dirt
    {
        {
            {2,0},{2,0},
            {2,0},{2,0},
            {2,0},{2,0}
        }
    },
    //command block
    {
        {
            {8,11},{8,11},
            {8,11},{8,11},
            {8,11},{8,11}
        }
    },
    //unknown
    {
        {
            {22,15},{22,15},
            {22,15},{22,15},
            {22,15},{22,15}
        }
    }
};