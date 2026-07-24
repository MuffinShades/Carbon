#pragma once
#include "msutil.hpp"
#include <string>

/*

Better implementation of vector cause std::vector has a 
fucking deconstructor which fucks up vectors with groupmem

*/

struct mvec_config {

};

template<class _Tyy> struct mu_itrptr {
    _Tyy *begin, *end;
};

template<class _Ty> class mu_vec {
private:
    struct dBlock {
        _Ty *elm;
        size_t off = 0, len = 0;
        dBlock *next = nullptr, *prev = nullptr;
    };

    struct _daccess {
        size_t off;
        dBlock *block = nullptr;
        volatile u64 __padd_align_32 = 0; //for 256 bit alignment in memory
    };

    struct aBlock {
        _daccess *datAccess = nullptr;
        size_t off = 0, len = 0;
        aBlock *next = nullptr, *prev = nullptr;
    };

    struct bBlock {
        size_t *idxTrack = nullptr;
        size_t off = 0, len = 0;
        bBlock *next = nullptr, *prev = nullptr;
    };

    size_t elemPerDataBlock = 0x1fff;
    size_t elemPerAccessBlock = 0x2fff;

    dBlock *root = nullptr, *tail = nullptr;
    aBlock *aRoot = nullptr, *aTail = nullptr;
    bBlock *bRoot = nullptr, *bTail = nullptr;

    size_t sz = 0;

    //block del queue
    template<class _Bty> struct delQ {
        delQ *next;
        size_t prevCountAtAdd = 0; //so you can subtract however many calls it took to delete the last block
        size_t countLeft = 0;
        _Bty *tBlock = nullptr;
    };

    const size_t delsBeforeProperDeletion = 16; //number of deletions before the a block is actually deleted

    delQ<dBlock> *dqp_root = nullptr, *dqp_last = nullptr;
    delQ<aBlock> *dqa_root = nullptr, *dqa_last = nullptr;

    void _add_data_block() noexcept {
        if (dqp_root) {
            auto *dqi = dqp_root, *dqi_old = (delQ<dBlock>*) nullptr;

            dBlock *bloc = dqi->tBlock;

            while (!bloc) {
                dqi_old = dqi;
                dqi = dqi_old->next;

                _safe_free_b(dqi_old);

                if (!dqi)
                    break;

                bloc = dqi->tBlock;
            }

            dqp_root = dqi ? dqi->next : nullptr;

            if (!dqi || !bloc)
                goto _def_Add;

            //now add le block and check if
            if (!bloc->elm || bloc->len == 0) {
                if (bloc->elm) _safe_free_a(bloc->elm);

                bloc->elm = new _Ty[elemPerDataBlock];
                bloc->len = elemPerDataBlock;
            }

            ZeroMem(bloc->elm, bloc->len);

            _Add_block_To(bloc, &root, &tail);

            //mem stuff
            _safe_free_b(dqi);
            return;
        }

        _def_Add:

        dBlock *bloc = new dBlock;

        if (!bloc) {
            std::cout << "error failed to add data block: bad alloc" << std::endl;
            return;
        }

        bloc->elm = new _Ty[elemPerDataBlock];
        ZeroMem(bloc->elm, elemPerDataBlock);
        bloc->len = elemPerDataBlock;

        _Add_block_To(bloc, &root, &tail);
    }

    void _add_access_block() noexcept {
        if (dqa_root) {
            auto *dqi = dqa_root, *dqi_old = (delQ<aBlock>*) nullptr;

            aBlock *bloc = dqi->tBlock;

            while (!bloc) {
                dqi_old = dqi;
                dqi = dqi_old->next;

                _safe_free_b(dqi_old);

                if (!dqi)
                    break;

                bloc = dqi->tBlock;
            }

            dqa_root = dqi ? dqi->next : nullptr;

            if (!dqi || !bloc)
                goto _def_Add;

            //now add le block and check if
            if (!bloc->datAccess || bloc->len == 0) {
                if (bloc->datAccess) _safe_free_a(bloc->datAccess);

                bloc->datAccess = new _daccess[elemPerAccessBlock];
                bloc->len = elemPerAccessBlock;
            }

            ZeroMem(bloc->datAccess, bloc->len);

            _Add_block_To(bloc, &aRoot, &aTail);

            //mem stuff
            _safe_free_b(dqi);
            return;
        }

        _def_Add:

        aBlock *bloc = new aBlock;

        if (!bloc) {
            std::cout << "error failed to add access block: bad alloc" << std::endl;
            return;
        }

        bloc->datAccess = new _daccess[elemPerAccessBlock];
        ZeroMem(bloc->datAccess, elemPerAccessBlock);
        bloc->len = elemPerAccessBlock;

        _Add_block_To(bloc, &aRoot, &aTail);
    }

    void _add_iTrack_block() noexcept {
        bBlock *bloc = new bBlock;

        if (!bloc) {
            std::cout << "error failed to add access block: bad alloc" << std::endl;
            return;
        }

        bloc->idxTrack = new size_t[elemPerAccessBlock];
        ZeroMem(bloc->idxTrack, elemPerAccessBlock);
        bloc->len = elemPerAccessBlock;

        _Add_block_To(bloc, &bRoot, &bTail);
    }

    template <typename _By> void _Add_block_To(_By *bloc, _By** root, _By** tail) noexcept {
        //std::cout << "adding bloc: " << (uintptr_t) bloc << " " << (uintptr_t) root << " " << (uintptr_t) tail << std::endl;

        if (!bloc || !root || !tail) return;

        if (!(*root)) {
            *root = (*tail = bloc);
        } else {
            (*tail)->next = bloc;
            bloc->prev = *tail;
            *tail = bloc;
        }
    }

    struct advDatAccess {
        _daccess acc;
        aBlock *abloc = nullptr;
        size_t aoff;
    };

    inline advDatAccess _get_access_from_index(size_t idx) {
        size_t ablocId = ((i64) idx / (i64) elemPerAccessBlock);
        const size_t off = idx - (ablocId * elemPerAccessBlock);

        //std::cout << "ooooooff: " << off << std::endl;

        //get the target access block
        auto *taBlock = this->aRoot;
        for (; ablocId > 0 && taBlock; ablocId--) taBlock = taBlock->next;

        if (!taBlock) {
            throw std::runtime_error("Could not find a valid access block for index: ");
        }

        //std::cout << "ee: " << taBlock->datAccess[off].off << " boc: " << (uintptr_t) taBlock->datAccess[off].block << std::endl;

        return {
            .acc = taBlock->datAccess[off],
            .abloc = taBlock,
            .aoff = off
        };
    }

    inline size_t _get_true_index_from_index(size_t idx) {
        size_t bblocId = (idx / elemPerAccessBlock);
        const size_t off = idx - (bblocId * elemPerAccessBlock);

        //get the target access block
        auto *tbBlock = this->bRoot;
        for (; bblocId > 0 && tbBlock; bblocId--) tbBlock = tbBlock->next;

        if (!tbBlock) {
            throw std::runtime_error("Could not find a valid access block for index: "+idx);
        }

        return tbBlock->idxTrack[off];
    }

    //consolidates all the vector's data into one big block
    void _consolidate_data() {
        if ((uintptr_t) this->root == (uintptr_t) this->tail || !this->root || !this->tail)
            return; //data is already consolidated

        if (this->sz == 0) {
            //delete old blocks and stuff
            this->clear();
            return;
        }

        _Ty *dat = new _Ty[this->sz];

        if (!dat) {
            throw std::runtime_error("failed to alloc for vector consolidation!");
        }

        _Ty *copyTo = dat;
        dBlock *cBloc = root, *fBloc;
        
        while (cBloc) {
            in_memcpy(copyTo, cBloc->elm, sizeof(_Ty) * cBloc->len);
            copyTo += cBloc->len;
            fBloc = cBloc;
            cBloc = cBloc->next;
            _safe_free_a(fBloc->elm);
            _safe_free_b(fBloc);
        }

        dBlock *coBloc = new dBlock;

        if (!coBloc) {
            throw std::runtime_error("failed to allocate new memory block for vector consolidation");
        }

        coBloc->elm = dat;
        coBloc->len = this->sz;

        this->root = (this->tail = coBloc);
    }

    inline void _rswap(size_t idx1, size_t idx2) {
        advDatAccess i1 = _get_access_from_index(idx1),
                     i2 = _get_access_from_index(idx2);

        //checks
        if (!i1.abloc || !i2.abloc || !i1.acc.block || !i2.acc.block)
            return;

        //swap primary elm buffer
        auto tmp = std::move(i1.acc.block->elm[i1.acc.off]);
        i1.acc.block->elm[i1.acc.off] = std::move(i2.acc.block->elm[i2.acc.off]);
        i2.acc.block->elm[i2.acc.off] = std::move(tmp);

        //swap a
        auto tmpa = std::move(i2.abloc[i2.aoff]);
        i2.abloc[i2.aoff] = std::move(i1.abloc[i1.aoff]);
        i1.abloc[i1.aoff] = std::move(tmpa);
    }
    
    //swap data
    inline void _dswap(size_t idx1, size_t idx2) {
        advDatAccess i1 = _get_access_from_index(idx1),
                     i2 = _get_access_from_index(idx2);

        //checks
        if (!i1.abloc || !i2.abloc || !i1.acc.block || !i2.acc.block)
            return;

        //swap primary elm buffer
        auto tmp = std::move(i1.acc.block->elm[i1.acc.off]);
        i1.acc.block->elm[i1.acc.off] = std::move(i2.acc.block->elm[i2.acc.off]);
        i2.acc.block->elm[i2.acc.off] = std::move(tmp);
    }

    //swap access
    inline void _aswap(size_t idx1, size_t idx2) {
        advDatAccess i1 = _get_access_from_index(idx1),
                     i2 = _get_access_from_index(idx2);

        //checks
        if (!i1.abloc || !i2.abloc || !i1.acc.block || !i2.acc.block)
            return;

        //swap a
        auto tmpa = std::move(i2.abloc[i2.aoff]);
        i2.abloc[i2.aoff] = std::move(i1.abloc[i1.aoff]);
        i1.abloc[i1.aoff] = std::move(tmpa);
    }

    template<class _Bty> void _fufill_Del(delQ<_Bty> *qi, bool free_qi = true) {
        if (!qi)
            return;

        if (qi->tBlock) {
            //adjust the prev block and next block
            if (qi->tBlock->next) qi->tBlock->next->prev = qi->tBlock->prev;
            if (qi->tBlock->prev) qi->tBlock->prev->next = qi->tBlock->next;

            _safe_free_b(qi->tBlock);
        }

        if (free_qi)
            _safe_free_b(qi);
    }

    template<class _Bty> void _dq_tick(delQ<_Bty>*& Q_root) {
        if (!Q_root)
            return;

        if (Q_root->countLeft == 0) {
            auto *qnxt = Q_root->next;
            _fufill_Del(Q_root, true);
            Q_root = qnxt;
        } else
            Q_root->countLeft--;
    }

    template<class _Bty> void _dq_add(_Bty *bloc, delQ<_Bty>** Q_last) {
        if (!bloc || !Q_last || !*Q_last)
            return;

        delQ<_Bty>* lnk = new delQ<_Bty>;

        if (!lnk) {
            std::cout << "error failed to " << std::endl;
            _del_block(bloc);
        }

        (*Q_last)->next = lnk;
        lnk->prevCountAtAdd = (*Q_last)->countLeft;
        *Q_last = lnk;
    }
    
    //block deletion functions
    template<class _Bty> void _del_block(_Bty *bloc) {
        if (!bloc)
            return;
        
        _safe_free_b(bloc);
    }

    template<> void _del_block(dBlock *bloc) {
        if (!bloc)
            return;

        if (bloc->elm) _safe_free_a(bloc->elm);
        _safe_free_b(bloc);
    }

    template<> void _del_block(aBlock *bloc) {
        if (!bloc)
            return;
        
        if (bloc->datAccess) _safe_free_a(bloc->datAccess);
        _safe_free_b(bloc);
    }

    template<> void _del_block(bBlock *bloc) {
        if (!bloc)
            return;
        
        if (bloc->idxTrack) _safe_free_a(bloc->idxTrack);
        _safe_free_b(bloc);
    }

    template<class _tty> inline void _sim_pop_ptr(_tty*& tail, delQ<_tty>** del_root) {
        if (!tail || !del_root || !*del_root)
            return;

        if (tail->off > 0)
            tail->off--;
        else {
            //(attempt to) delete last block
            auto *pt = std::move(tail);
            tail = tail->prev;
            _dq_add<_tty>(pt, del_root);

            //now modify prev last block
            if (tail) tail->off--;
        }
    }

    inline void _2pop_no_ret() {
        if (!this->tail)
            return;

        _sim_pop_ptr<dBlock>(this->tail, &dqp_root); //elements
        _sim_pop_ptr<aBlock>(this->aTail, &dqa_root); //data access
    }
public: 

    /*
    
    TODO: just combine all offsets into one maybe??
    Heck actually just combine all blocks into one!
        --> actually don't do this because of swapping issues
            and alignment can make certain operations wayyyy faster
    
    */

    void push(_Ty val) {
        if (!tail || tail->off == tail->len) this->_add_data_block();
        if (!aTail || aTail->off == aTail->len) this->_add_access_block();
        //if (!bTail || bTail->off == bTail->len) this->_add_iTrack_block();
        if (!tail || !aTail || !tail->elm || !aTail->datAccess) {
            std::cout << "push failed: \n" 
                      << "\ttail: "<< (uintptr_t) tail <<"\n"
                      << "\tatail: "<< (uintptr_t) aTail <<"\n"
                      << "\ttail elm: "<< (uintptr_t) tail->elm <<"\n"
                      << "\tatail dta: "<< (uintptr_t) aTail->datAccess << std::endl;
            return;
        }

        tail->elm[tail->off++] = val;
        aTail->datAccess[aTail->off++] = {
            .off = (tail->off-1),
            .block = tail
        };

        //std::cout << aTail->datAccess[aTail->off-1].block << " OFF: " << (tail->off-1) << std::endl;
        //bTail->idxTrack[bTail->off++] = this->len;

        this->sz++;
    }

    _Ty pop() {
        //easy peasy
        if (!this->tail || this->sz == 0) {
            throw std::out_of_range("Cannot pop on an empty vector!");
        }

        if (!this->tail->elm) {
            
        }

        this->sz--;

        if (this->tail->off > 0) {
            _sim_pop_ptr<aBlock>(this->aTail);
            return this->tail->elm[this->tail->off--];
        } else {
            this->tail = this->tail->prev;

            if (!this->tail) {
                throw std::runtime_error("lll no tail :P");
            }

            _sim_pop_ptr<aBlock>(this->aTail);
            return this->tail->elm[this->tail->off--];  
        }
    }

    void clear() {
        //clear roots
        while (root) {
            auto *b = root->next;
            _safe_free_b(root);
            root = b;
        }

        root = (tail = nullptr);

        //clear access
        while (aRoot) {
            auto *b = aRoot->next;
            _safe_free_b(aRoot);
            aRoot = b;
        }

        aRoot = (aTail = nullptr);

        this->sz = 0;
    }

    void softClear() {

    }

    //slow af but whatever
    void insert(size_t idx, _Ty val) {
        advDatAccess ia = _get_access_from_index(idx);

        if (!ia.abloc) {
            throw std::runtime_error("failed to get proper block for index: "+idx);
        }

        if (!tail || tail->off == tail->len) this->_add_data_block();
        if (!aTail || aTail->off == aTail->len) this->_add_access_block();
        if (!tail || !aTail || !tail->elm || !aTail->datAccess) {
            throw std::runtime_error("Could not add data blocks or something idk im too tired to give a good description for all of these exception lmfao");
        }

        tail->elm[tail->off++] = val;

        //adjust the access things
        _daccess vAcc = {
            .off = tail->off,
            .block = tail
        };

        _daccess carry = vAcc;
        aBlock *curAdjust = ia.abloc;
        size_t copyStart = ia.aoff;

        while (curAdjust) {
            auto next_carry = std::move(curAdjust->datAccess[mu_min(curAdjust->len, curAdjust->off)-1]);
            //todo: figure out if a memcpy or a std::move is better
            in_memcpy(
                (curAdjust->datAccess+copyStart+1), 
                (curAdjust->datAccess+copyStart), 
                ((curAdjust->off - copyStart) - 1) * sizeof(_daccess)
            );
            curAdjust->datAccess[copyStart] = std::move(carry);
            carry = std::move(next_carry);
            copyStart = 0;
            curAdjust = curAdjust->next;

            if (curAdjust->off < curAdjust->len) {
                curAdjust->off++;
                break;
            }
        }

        //adjust length
        this->sz++;
    }

    void remove(size_t idx) {
        //std::cout << "sz: " << this->sz << std::endl;
        advDatAccess iinf = _get_access_from_index(idx);

        if (!iinf.abloc) {
            throw std::runtime_error(idx + " is out of mu_vec bounds!");
        }

        this->_rswap(idx, this->sz-1);

        //decrease other vals
        i32 i = iinf.aoff;
        auto *caBlock = iinf.abloc;
        _daccess *pSet = nullptr;

        /* off
        block */
        while (caBlock && caBlock->datAccess) {
            if (pSet)
                *pSet = caBlock->datAccess[0];
            if (caBlock->off > 0) {
                for (; i < caBlock->off - 1; i++) {
                    caBlock->datAccess[i].off = std::move(caBlock->datAccess[i+1].off);
                    caBlock->datAccess[i].block = std::move(caBlock->datAccess[i+1].block);
                }

                pSet = caBlock->datAccess + (caBlock->off - 1);
            } else {
                pSet = nullptr;
            }
            caBlock = caBlock->next;
            i=0;
        }

        //swap last 2 if needed
        if (this->sz >= 2) this->_aswap(this->sz-2, this->sz-1);

        //now delete le element
        _2pop_no_ret();
        this->sz--;
    }

    _Ty operator[](size_t idx) {
        if (idx >= this->sz) {
            throw std::out_of_range(idx + " is out of range of " + this->sz);
        }

        //get val
        //TODO: maybe not call _get_access_from_index to minimize jumps so indexing the vector is even faster
        //i just made it inline so everything is faster
        const advDatAccess valLoc = _get_access_from_index(idx);    
        return valLoc.acc.block->elm[valLoc.acc.off];
    }

    size_t len() {
        return this->sz;
    }

    void free() {
        this->clear();
    }

    void intSwap(size_t idx1, size_t idx2) {
        advDatAccess i1 = _get_access_from_index(idx1),
                i2 = _get_access_from_index(idx2);

        if (!i1.acc.block || !i2.acc.block) {
            throw std::runtime_error("could not swap indexes "+std::to_string((i64) idx1)+" and "+std::to_string((i64) idx2));
        }

        //swap le indexes
        //TODO: make sure all these block accesses aren't fucking up performance
        //TODO: also swap the access buffers thingys
        auto tmp = std::move(i1.acc.block->elm[i1.off]);
        i1.acc.block->elm[i1.off] = std::move(i2.acc.block->elm[i2.off]);
        i2.acc.block->elm[i2.off] = std::move(tmp);
    }

    //Todo: later
    void configure(mvec_config cfg, bool recalcAll) {

    }

    //get itr functions
    mu_itrptr<_Ty> getPtrsFor(size_t startIdx, size_t len) {

    }

    _Ty *begin() {
        this->_consolidate_data();
        if (this->root)
            return this->root->elm;
        else
            return nullptr;
    }

    _Ty *end() {
        this->_consolidate_data();
        if (this->root)
            return this->root->elm + this->root->len - 1;
        else
            return nullptr;
    }
};