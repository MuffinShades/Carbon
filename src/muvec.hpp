#pragma once
#include "msutil.hpp"

/*

Better implementation of vector cause std::vector has a 
fucking deconstructor which fucks up vectors with groupmem

*/

struct mvec_config {

};

template<class _Ty> class mu_vec {
private:
    struct dBlock {
        _Ty *elm;
        size_t off = 0, len = 0;
        dBlock *next = nullptr, *tail = nullptr;
    };

    struct _daccess {
        size_t off;
        dBlock *block = nullptr;
    };

    struct aBlock {
        _daccess *datAccess = nullptr;
        size_t off = 0, len = 0;
        aBlock *next = nullptr, *tail = nullptr;
    };

    size_t elemPerDataBlock = 0xffff;
    size_t elemPerAccessBlock = 0xfff;

    dBlock *root = nullptr, *tail = nullptr;
    aBlock *aRoot = nullptr, *aTail = nullptr;

    size_t sz;

    void _add_data_block() noexcept {
        dBlock *bloc = new dBlock;

        if (!block) {
            std::cout << "error failed to add data block: bad alloc" << std::endl;
            return;
        }

        bloc->elm = new _Ty[elemPerDataBlock];
        ZeroMem(bloc->elm, elemPerDataBlock);
        bloc->len = elemPerDataBlock;

        _Add_block_To(bloc, &root, &tail);
    }

    void _add_access_block() noexcept {
        aBlock *bloc = new aBlock;

        if (!block) {
            std::cout << "error failed to add access block: bad alloc" << std::endl;
            return;
        }

        bloc->datAccess = new _daccess[elemPerAccessBlock];
        ZeroMem(bloc->datAccess, elemPerAccessBlock);
        bloc->len = elemPerAccessBlock;

        _Add_block_To(bloc, &aRoot, &aTail);
    }

    template <typename _By> void _Add_block_To(_By *bloc, _By** root, _By** tail) noexcept {
        if (!bloc || !root || !tail) return;

        if (!(*root)) {
            *root = (*tail = bloc);
        } else {
            (*tail)->next = bloc;
            bloc->prev = tail;
            *tail = bloc;
        }
    }

    struct advDatAccess {
        _daccess acc;
        aBlock *abloc = nullptr;
        size_t aoff;
    };

    advDatAccess _get_access_from_index(size_t idx) {
        const size_t ablocId = (idx / elemPerAccessBlock);
        const size_t off = idx - (ablocId * elemPerAccessBlock);

        //get the target access block
        auto *taBlock = this->aRoot;
        for (; ablocId > 0 && taBlock; ablocId--) taBlock = taBlock->next;

        if (!taBlock) {
            throw std::exception("Could not find a valid access block for index: "+idx);
        }

        return {
            .acc = taBlock->datAccess[off],
            .abloc = taBlock,
            .aoff = off
        };
    }
public: 
    void push(_Ty val) {
        if (!tail || ++tail->off == tail->len) this->_add_data_block();
        if (!aTail || ++aTail->off == aTail->len) this->_add_access_block();
        if (!tail || !aTail || !tail->elm || !aTail->datAccess) return;

        tail->elm[tail->off] = val;
        aTail->datAccess[aTail->off] = {
            .off = tail->off,
            .block = tail
        };

        this->len++;
    }

    _Ty pop() {
        //easy peasy
        if (!this->tail || this->len == 0) {
            throw std::out_of_range("Cannot pop on an empty vector!");
        }

        if (!this->tail->elm) {
            this->tail->elm = new 
        }

        if (this->tail->off > 0) {
            this->tail->off--;
            this->sz--;
            return this->tail->elm[this->tail->off];
        } else {
            this->tail = this->tail->prev;

            if (!this->tail) {
                throw std::exception();
            }

            return this->;  
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

        this->len = 0;
    }

    void softClear() {

    }

    //slow af but whatever
    void insert(size_t idx, _Ty val) {
        advDatAccess ia = _get_access_from_index(idx);

        if (!ia.abloc) {
            throw std::exception("failed to get proper block for index: "+idx);
        }

        if (!tail || ++tail->off == tail->len) this->_add_data_block();
        if (!aTail || ++aTail->off == aTail->len) this->_add_access_block();
        if (!tail || !aTail || !tail->elm || !aTail->datAccess) {
            throw std::exception("Could not add data blocks or something idk im too tired to give a good description for all of these exception lmfao");
        }

        tail->elm[tail->off] = val;
    }

    void remove(size_t idx) {

    }

    _Ty operator[](size_t idx) {
        if (idx >= this->len) {
            throw std::out_of_range(idx + " is out of range of " + this->len);
        }

        //get val
        //TODO: maybe not call _get_access_from_index to minimize jumps so indexing the vector is even faster
        const advDatAccess valLoc = _get_access_from_index(idx);        
        return valLoc.acc.block->elm[valLoc.acc.block->off];
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
            throw std::exception("could not swap indexes "+idx1+" and "+idx2);
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
};

/*

Like the above vector just with the address indexing feature thingies (not super crazy important)

*/
template<class _Ty> class _mu_wip_magicContainer {

};