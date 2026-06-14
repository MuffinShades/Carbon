#include "groupmem.hpp"

#define BLOCK_PAD_BYTE 0xFF //pad with 0xFF

GroupMem::GroupMem(size_t blockSz, size_t maxBlocksInAlloc) {
    this->aBlockSz = blockSz;
    this->maxGroupBlocksSaved = maxBlocksInAlloc;
    this->memBlockSz = (this->prePadSz + this->aBlockSz) * this->nSubBlocksPerMemBlock;
    this->good = true;
}

GroupMem::GroupMem() {
    this->good = false;
}


void *GroupMem::allocNBlocks(size_t nBlocks, void *fill, size_t fill_sz, void *buddy) {
    if (nBlocks == 0 || !this->good)
        return nullptr;

    const size_t res_sz = nBlocks * (this->aBlockSz + this->prePadSz);

    //find and setup region
    //TODO: account for the padding at the beginning and try and save the pointers right or what not idk it's 12:40am and i want to sleep
    void *tptr = nullptr;

    if (nBlocks > this->maxGroupBlocksSaved) {

    } else {
        mem_blk *check_block = this->root_block, *l_block;

        if (!check_block) this->_add_blok();
        check_block = this->root_block;
        if (!check_block) {
            std::cout << "Error: failed to create a new memory block!" << std::endl;
            return nullptr;
        }

        //find the free region
        free_rgn *fRgn = nullptr;
        i32 i, rgnIdx;

        do {
            for (i = nBlocks - 1; i < this->maxGroupBlocksSaved; i++) {
                fRgn = check_block->free_spaces[i];
                rgnIdx = i;
                if (fRgn) break;
            }
            l_block = check_block;
            check_block = check_block->next;
        } while (!fRgn && check_block);

        if (fRgn) {
            tptr = fRgn->begin;

            //target block is l_block
            //delete the free region
            l_block->free_spaces[rgnIdx] = fRgn->next;
            _safe_free_b(fRgn);
        } else {
            //allocate a new block
            if(!this->last_block || !this->root_block) this->_add_blok();
            if(!this->last_block || !this->root_block) {
                std::cout << "failed ot allocate: " << nBlocks << std::endl;
                return nullptr;
            }

            mem_blk *tBlock = this->last_block;

            if (tBlock->sz - res_sz < tBlock->wpos) {
                //TODO: add free reservation thing

                //tBlock->free_spaces[]
                tBlock->wpos = this->memBlockSz;
                this->_add_blok();
                tBlock = this->last_block;

                if (!tBlock || tBlock->sz - res_sz < tBlock->wpos) {
                    std::cout << "failed to allocate: " << nBlocks << std::endl;
                    return nullptr;
                }
            }

            //get pointer and inc the block wpos/pointer thingy
            tptr = (void*)(((byte*) tBlock->dat) + tBlock->wpos);
            tBlock->wpos += res_sz;
        }
    }

    //fill new region 
    if (fill && fill_sz > 0 && tptr)
        in_memcpy(tptr, fill, fill_sz);

    //add memory reservation
    mem_reservation res = {
        .sz = res_sz,
        .nBlocks = nBlocks,
        .loc = (uintptr_t) tptr
    };

    this->current_reservations.insert(reinterpret_cast<char*>(&res.loc), sizeof(uintptr_t), res);

    //return blok
    return tptr;
}

void *GroupMem::allocNBlocks(size_t nBlocks, void *buddy) {
    return this->allocNBlocks(nBlocks, nullptr, 0, buddy);
}

void *GroupMem::allocBySize(size_t sz, void *fill, size_t fill_sz, void *buddy) {
    //convert sz to number of blocks
}

void *GroupMem::allocBySize(size_t sz, void *buddy) {
    return this->allocBySize(sz, nullptr, 0, buddy);
}

void GroupMem::freeData(void *ptr) {
    if (!ptr || !this->good)
        return;

    _dat_loc LOC = _locate_data(ptr);

    if (!LOC.block)
        return; //no point in freeing

    mem_blk *t_block = LOC.block;

    //find size of data being freed
    uintptr_t uiptr = (uintptr_t) ptr;
    hash_node<mem_reservation>* res = current_reservations.seek(reinterpret_cast<char*>(&uiptr), sizeof(uintptr_t));

    //find the proper pointer
    while (res && res->val.loc != uiptr) {
        res = res->prev;
    }

    if (!res) {
        return; //data was never allocated
    }

    const size_t allocSz = res->val.sz;

    if (allocSz == 0) { //ensure the 2 pointers we found align and stuff was actually allocated
        std::cout << "group mem free err: size was zero!" << std::endl;
        return;
    }

    i32 ext;

    const size_t trueBsz = this->aBlockSz + this->prePadSz;

    if ((ext = (allocSz % trueBsz)) != 0) {
        std::cout << "group mem warning: freeing strange number of blocks" << std::endl;
    }

    //const size_t nBlocks = (allocSz / trueBsz) + (ext != 0 ? 1 : 0);
    const size_t nBlocks = res->val.nBlocks;

    //TODO: note the new free in the whole free block table thing
    auto *fContain = t_block->free_spaces[nBlocks];

    free_rgn *rgn = new free_rgn;

    ZeroMem(rgn, 1);

    rgn->begin = ptr;
    rgn->nBlocks = nBlocks;
    rgn->t_block = t_block;

    if (fContain) {
        t_block->free_spaces[nBlocks]->next = rgn;
        rgn->prev = t_block->free_spaces[nBlocks];
        t_block->free_spaces[nBlocks] = rgn;
    } else {
        t_block->free_spaces[nBlocks] = rgn;
    }

    //finally remove the node from reservations
    current_reservations.removeNode(res);
}

void GroupMem::freeAllBlocks() {
    if (!this->good) return;

    //look for any still existing reservations
    //TODO: the comment above

    //then free all le blocks
}

void GroupMem::_add_blok() {
    if (!this->good) return;

    mem_blk *mb = new mem_blk;

    if (!mb) {
        std::cout << "error failed to create new memblock : malloc failed!" << std::endl;
        return;
    }

    ZeroMem(mb, 1);

    //allocate data blocks
    mb->dat = (void*) new byte[this->memBlockSz];
    ZeroMem(mb->dat, this->memBlockSz);
    mb->sz = this->memBlockSz;

    //allocate free space location table
    mb->nFreeSpaces = mu_max(1, this->maxGroupBlocksSaved);
    mb->free_spaces = new free_rgn*[mb->nFreeSpaces];
    ZeroMem(mb->free_spaces, mb->nFreeSpaces);

    //add the block in
    if (!this->root_block) {
        if (this->last_block) {
            //fuck
            std::cout << "fuck dude (groupmem.cpp)" << std::endl;

            //TODO: reattach old blocks


            return;
        }

        //add as root block if one isnt there
        mb->prev = nullptr;
        this->root_block = (this->last_block = mb);
    } else {
        mb->prev = this->last_block;
        this->last_block->next = mb;
        this->last_block = mb;
    }
}

bool GroupMem::checkDataIntegrity(void *allocStart) {
    if (!allocStart || !this->good)
        return false;

    //first locate where the block is at
    _dat_loc LOC = _locate_data(allocStart);

    if (!LOC.block || LOC.offset < this->prePadSz) //could not find data or data starts too early
        return false;

    //now check integrity
    size_t checkByte = 0;

    do {
        if (*((byte*) allocStart - checkByte) != BLOCK_PAD_BYTE)
            return false;
    } while(++checkByte < this->prePadSz);

    return true;
}

GroupMem::_dat_loc GroupMem::_locate_data(void *dat) {
    _dat_loc loc;

    loc.block = nullptr;
    loc.offset = 0;

    if (!dat || !this->good)
        return loc;

    const uintptr_t dat_ptr = (uintptr_t) dat;

    //start checking blocks
    mem_blk *bloc = this->root_block;
    uintptr_t baseBlockPtr;

    while (bloc) {
        baseBlockPtr = (uintptr_t) bloc->dat;

        if (dat_ptr >= baseBlockPtr && dat_ptr < baseBlockPtr + bloc->sz) {
            loc.block = bloc;
            loc.offset = dat_ptr - baseBlockPtr;
            return loc;
        }

        bloc = bloc->next;
    }

    return loc;
}

void GroupMem::_free_block(mem_blk *bloc) {
    if (!bloc || !this->good) return;

    if (bloc->dat) _safe_free_a(bloc->dat);
    if (bloc->free_spaces) {
        auto *fs = bloc->free_spaces, *fs_end = fs + bloc->nFreeSpaces;

        while (fs < fs_end) {
            if (*fs) {
                free_rgn *rgn = *fs;

                while (rgn) {
                    auto *nx = rgn->next;
                    _safe_free_b(rgn);
                    rgn = nx;
                }
            }

            fs++;
        }
    }
    
    if (bloc->prev) {
        bloc->prev->next = bloc->next; //properly remove block from the chain
    }

    //delete le block
    _safe_free_b(bloc);
}