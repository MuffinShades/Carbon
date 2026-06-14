#pragma once
#include <iostream>
#include "msutil.hpp"

template<typename _Ty> struct hash_node {
    hash_node* prev = nullptr, *next = nullptr;
    bool bulkAlloc = false;
    size_t sz = 0;
    char* key = nullptr;
    u64 hash = 0, id = 0;
    _Ty val;
};

//recommended 15 bits
//TODO: consider adding a hash id so that you can't accidentally remove nodes from the hash that belong to other hashes just cause the ids match
// the check uses and OR instead of an AND so if the pointers or ids match the node can be removed --> although shouldn't cause too much of a problem,
// can completely screw up an entire root tree thingy (see removeNode code)
template<class _storeType, size_t hBits> class linked_map {
private:
    const size_t hashBits = hBits; //number of bits per hash
    size_t hashSz = 0;
    size_t nextId = 0;

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
            hash += (dat[i] * g);

            g <<= 8;
            g |= 0xff;

            if ((g >> 63) != 0)
                g = 0xff;
        }

        return hash & mask;
    }

    /**
     *
     *
     *
     */
    hash_node<_storeType>* _insert(hash_node<_storeType>* n) {
        if (hash >= this->hashSz) {
            return nullptr;
        }

        n->prev = this->roots[n->hash];
        this->roots[n->hash]->next = n;
        this->roots[n->hash] = n;
        return n->prev;
    }

    hash_node<_storeType>* _seek(u64 hash) {
        if (hash >= this->hashSz) {
            return nullptr;
        }

        return this->roots[hash];
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
                .id = this->nextId++,
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
            h_node->bulkAlloc = true;
            h_node->id = this->nextId++;

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

    hash_node<_storeType>* seek(char* key, const size_t key_sz) {
        const u64 hsh = this->computeHash(key, key_sz);
        return this->_seek(hsh);
    }

    hash_node<_storeType>* seek(std::string key) {
        return this->seek(
            const_cast<char*>(key.c_str()),
            key.length()
        );
    }

    template<class _KeyTy> hash_node<_storeType>* seek(_KeyTy key) {
        const size_t k_sz = sizeof(_KeyTy);
        void* k_mem = (void*)&key;

        return this->seek(
            (char*)k_mem,
            k_sz
        );
    }

    /**
     * To remove nodes
     */
    void removeNode(hash_node<_storeType> *node, bool free = true) {
        if (!node) return;

        //check if node is a root node
        if (node->hash >= this->hashSz) {
            std::cout << "Cannot remove hash node! Hash of " << node->hash << " exceeds hash max of " << (this->hashSz - 1) << "!" << std::endl;
            return;
        }

        auto *root = this->roots[node->hash];

        if (!root) {
            return;
        }

        //if the node is a root then properly assign it
        if ((uintptr_t) root == (uintptr_t) node || root->id == node->id) {
            if (node->next) { //this is a edge case that only occurs in the case that the pointers are fucked. This code should fix em tho
                auto *nd = node;

                do {
                    root = nd;
                    nd = nd->next;
                } while(nd);
            } else {
                root = node->prev;
            }

            this->roots[node->hash] = root; //idk if this does anything but ima put it here just incase
        }

        //remove le node
        node->next->prev = node->prev;
        node->prev->next = node->next;

        //oh and also free it if need be
        if (free) { //im not calling freeExtNode cause slow and i get dem extra lines of code not calling it :)
            ZeroMem(node, 1);
            if (!node->bulkAlloc) _safe_free_b(node);
        }
    }

    void freeExtNode(hash_node<_storeType> *node) {
        if (!node) return;
        ZeroMem(node, 1);
        if (!node->bulkAlloc) _safe_free_b(node);
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