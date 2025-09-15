#pragma once
#include <iostream>
#include "msutil.hpp"

template<typename _Ty> struct hash_node {
    hash_node* prev = nullptr;
    bool bulkAlloc = false;
    size_t sz = 0;
    char* key = nullptr;
    u64 hash = 0;
    _Ty val;
};

//recommended 15 bits
template<class _storeType, size_t hBits> class linked_map {
private:

    const size_t hashBits = hBits; //number of bits per hash
    size_t hashSz = 0;

    hash_node<_storeType>** roots = nullptr;
    hash_node<_storeType>* preAllocNodes = nullptr;

    size_t preAllocLeft = 0, preAllocCur = 0, nPreAllocNodes = 0;

    bool preAllocAll = false;

    /**
     * linked_map::allocHash
     *
     * allocates hash data for le hash
     * woah :O
     *
     */
    void allocHash() {
        this->free();
        this->hashSz = 1 << this->hashBits;
        this->roots = new hash_node<_storeType>*[this->hashSz];
        ZeroMem(this->roots, this->hashSz);
    }

    /**
     *
     *
     *
     */
    u64 computeHash(char* dat, const size_t len) {
        const u64 mask = (1 << this->hashBits) - 1;

        u64 hash = 0, g = 0xff;

        for (size_t i = 0; i < len; i++) {
            hash + (dat[i] * g);
            
            g <<= 8;
            g |= 0xff;

            if ((g >> 63) != 0)
                g = 0xff;
        }

        return (hash % mask) & mask;
    }

    /**
     *
     *
     *
     */
    hash_node<_storeType>* _insert(hash_node<_storeType>* n) {
        n->prev = this->roots[n->hash];
        this->roots[n->hash] = n;
        return n->prev;
    }
public:

    /**
     *
     *
     *
     */
    void free() {
        this->clear();
        if (this->roots != nullptr) {
            delete[] this->roots;
            this->roots = nullptr;
        }
        if (this->preAllocNodes) {
            delete[] this->preAllocNodes;
            this->preAllocNodes = nullptr;
        }
        this->hashSz = 0;
    }

    /**
     *
     *
     *
     */
    //TODO: work on making this FASTER
    hash_node<_storeType>* insert(char* key, const size_t key_sz, const _storeType dat) {
        u64 hsh = this->computeHash(key, key_sz);

        if (this->preAllocLeft == 0 && !this->preAllocAll) {
            return this->_insert(new hash_node<_storeType>{
                .sz = key_sz,
                .key = key,
                .hash = hsh,
                .val = dat
            });
        }
        else {
            if (this->preAllocLeft == 0) {
                std::cout << "Error, no hash space left!!!" << std::endl;
                return nullptr;
            }

            hash_node<_storeType>* h_node = &this->preAllocNodes[this->preAllocCur++];
            this->preAllocLeft--;

            h_node->sz = key_sz;
            h_node->key = key;
            h_node->hash = hsh;
            h_node->val = dat;

            return this->_insert(h_node);
        }

        std::cout << "Error, no hash space left!!!" << std::endl;
        return nullptr;
    }

    /**
     *
     *
     *
     */
    hash_node<_storeType>* insert(std::string key, const _storeType dat) {
        return this->insert(
            const_cast<char*>(key.c_str()),
            key.length(),
            dat
        );
    }

    /**
     *
     *
     *
     */
    template<class _KeyTy> hash_node<_storeType>* insert(_KeyTy key, const _storeType dat) {
        const size_t k_sz = sizeof(_KeyTy);
        void* k_mem = (void*)&key;

        return this->insert(
            (char*)k_mem,
            k_sz,
            dat
        );
    }

    /**
     *
     *
     *
     */
    void clear() {
        if (!this->preAllocAll) {
            for (size_t i = 0; i < this->hashSz; i++) {
                hash_node<_storeType>* cur = this->roots[i];
                while (cur) {
                    //std::cout << "Deleting:" << cur << std::endl;
                    hash_node<_storeType>* prev = cur->prev;
                    if (!cur->bulkAlloc) _safe_free_b(cur);
                    cur = prev;
                }

                this->roots[i] = nullptr;
            }
        }
        else {
            ZeroMem(this->roots, this->hashSz);
        }

        ZeroMem(this->preAllocNodes, this->nPreAllocNodes);
        this->preAllocLeft = this->nPreAllocNodes;
        this->preAllocCur = 0;
    }

    void preAlloc(size_t nNodes) {
        if (this->preAllocNodes) {
            this->free();
        }

        this->preAllocNodes = new hash_node<_storeType>[nNodes];
        ZeroMem(this->preAllocNodes, nNodes);
        this->preAllocLeft = nNodes;
        this->nPreAllocNodes = nNodes;
    }

    void EnablePreAllocMode() {
        this->preAllocAll = true;
    }

    void DisablePreAllocMode() {
        this->preAllocAll = false;
    }

    /**
     *
     * uhh alloc ye ye
     *
     */
    linked_map() {
        this->allocHash();
    }
};