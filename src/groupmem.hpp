#include "msutil.hpp"
#include "linked_map.hpp"

//kinda like a mini kernel
/**
 * HOW TO USE
 *
 * Create an instance of groupmem ie
 *    GroupMem groupMemThing = GroupMem([BLOCK SIZE], [MAX BLOCKS IN ONE ALLOCATION]);
 *
 * BLOCK SIZE --> how many bytes each block of allocated memory will take up
 *     * When allocating you allocate in sizes of blocks
 * MAX BLOCKS IN ONE ALLOCATION --> many number of blocks you can allocate at once
 *     * it is recommended this value is below 65,535 (ideally a small number like 8 to 16)
 *
 * To allocate call groupMemThing.allocNBlocks([NUMBER OF BLOCKS], [FILL], [FILL SIZE], [BUDDY]);
 *
 * NUMBER OF BLOCKS --> number of memory blocks to be allocated
 * FILL --> void pointer to a region of memory that the newly allocated bytes can be filled with
 * FILL SIZE --> how many bytes to copy from the fill region
 * BUDDY --> set to nullptr, later this will be used to be able to easily allocate regions of memory next to eachother
 * 
 * * Note do not call any of the other alloc functions since I haven't fully implemented them yet
 *
 * To free data simply call groupMemThing.freeData([PTR])
 *
 * PTR --> the pointer that you want to free that was returned from the alloc call
 *
 * When done with the memgroup make sure to free all pointers returned from the function via freeData and finally call freeAllBlocks
 * Note not calling freeData isn't a major issue it just may result in more bloated memory usage. Just make sure to NOT USE ANY POINTERS
 * RETURNED BY GROUP MEM AFTER CALLING freeAllBlocks!!!
 */

class GroupMem {
private:
    size_t aBlockSz = 0xff, memBlockSz = 0, nSubBlocksPerMemBlock = 0xff; //size of a alloc block
    size_t maxGroupBlocksSaved = 8;
    struct free_rgn;
    struct mem_blk {
        void *dat = nullptr;
        size_t sz, nFreeSpaces;
        mem_blk *next, *prev;
        free_rgn **free_spaces;
        size_t wpos;
    };
    struct free_rgn {
        size_t nBlocks;
        void *begin, *prev;
        mem_blk *t_block;
        free_rgn *next;
    };
    mem_blk *root_block = nullptr, *last_block = nullptr;
    void _add_blok();
    bool good = false;
    const size_t prePadSz = 4;
    struct _dat_loc {
        GroupMem::mem_blk *block;
        size_t offset;
    };
    _dat_loc _locate_data(void *dat);
    void _free_block(mem_blk *bloc);
    struct mem_reservation {
        size_t sz, nBlocks;
        uintptr_t loc;
    };
    linked_map<mem_reservation, 15> current_reservations = linked_map<mem_reservation, 15>();
public:
    GroupMem(size_t blockSz, size_t maxBlocksInAlloc);
    GroupMem();
    void ini(size_t blockSz, size_t maxBlocksInAlloc);
    void *allocNBlocks(size_t nBlocks, void *buddy = nullptr);
    void *allocNBlocks(size_t nBlocks, void *fill, size_t fill_sz, void *buddy = nullptr);
    void *allocBySize(size_t sz, void *buddy = nullptr);
    void *allocBySize(size_t sz, void *fill, size_t fill_sz, void *buddy = nullptr);
    bool checkDataIntegrity(void *allocStart);
    void freeData(void *ptr);
    void freeAllBlocks();
};