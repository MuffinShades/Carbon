 /*

 Future TODOS:

 take large compression abouts and compression data as blocks
 also create functions to take a file and compression as blocks
 for low memory usage


 also add multithreading when doing the above compression for
 even faster speeds

 use the better bitstream class present in MuffinMedia and for
 _inflate_block_none, use the better stream and add a function
 to copy a certain number of bytes to a buffer instead of iter-
 ating the output buffer and setting each value after calling
 stream->readByte();

 Current TODO:

 figure out why inflate ends on a different byte then deflate when using blocks
  -fixed check sum was being written at the end of a block not the whole file

  make deflate more modular since there are lots of inflate and deflate functions
    -> functions for reading and writing zlib and chunk headerS

 Functions that need optimizing:
    WriteVBits to stream -> slow for loop and branches
    lz77_encode -> new operator for each node is really slow

Possible optimizations
    * Reduce branches
    * Don't zeromem buffers that are fully filled by in_memcpy in the next instruction <- fr
    * De-clutter code
    * lz77
    * The "bitReverse" function -> could make it a consteval function in a way which would make it much faster
    * use consteval and constexpr when appropriate
    * etc.
    * remove the f*cking std::couts << :skull:
    * use a 65kb memory region instead of std::vector for inflating a block of data
    * improve speeds of lz77 back ref in inflate
    * make bytestream support a read only from a data source mode or add non "data own" chunks to the stream

POSSIBLE BUGS
    * bobs bit length service may be needed on distance trees; possible fix is adding a maxLen param when calling
      GenCanonicalTreeInf

Known Bugs
    * Inflate level 0 doesn't work -> (I think)

Resolved Bugs
    * trees with 1 node are not created - resolved 6/14/2025

A lot of this code was written before I knew how to hyper-optimize code well
 */

#define BALLOON_DEBUG
#include "balloon.hpp"
#include "bytestream.hpp"
#include "bitstream.hpp"
#include "linked_map.hpp"
#include "micro_vector.hpp"
#include <algorithm>
#include <iomanip>
#include <vector>
#include <assert.h>
#include <cmath>
#include <stdlib.h>
#include <queue>
#include "realigned_buffer.hpp"
#include "memcpy.hpp"
#include <chrono>

 //bin
struct bin {
    byte* dat;
    size_t sz;
};

//MACROS and constants
#define INRANGE(v,m,x) ((v) >= (m) && (v) < (x))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define WINDOW_BITS 0xf
#define MIN_MATCH 3
#define MEM_LEVEL 8

#ifndef INT_MAX
#define INT_MAX 0xffffffff
#endif

#define LZ77_MIN_MATCH 0x003 // min match size for back reference
#define LZ77_MAX_MATCH 0x102

#define MAX_CODE_LENGTH 15

#define BLOCK_SIZE_MAX 0xffff //deflate wants a max block size of 64kb, idk why

//from zlib on github
const int compression_table_old[10][5] = {
    /*      good lazy nice max_chain_length */
    /* 0 */ {0,    0,  0,    0},    /* store only */
    /* 1 */ {4,    4,  8,    4},    /* max speed, no lazy matches */
    /* 2 */ {4,    5, 16,    8},
    /* 3 */ {4,    6, 32,   32},
    /* 4 */ {4,    4, 16,   16},    /* lazy matches */
    /* 5 */ {8,   16, 32,   32},
    /* 6 */ {8,   16, 128, 128},
    /* 7 */ {8,   32, 128, 256},
    /* 8 */ {32, 128, 258, 1024},
    /* 9 */ {32, 258, 258, 4096}    /* max compression */
};

/**
 *
 * Tables used throughout various steps of decoding and ecoding
 *
 * LengthExtraBits - lz77 encode & decode
 * LengthBase - lx77 encode & decode
 * DistanceExtraBits - lz77 encode & decode
 * DistanceBase - lz77 encode & decode
 * CodeLengthCodesOrder - trees encode & decode
 *
 */
constexpr u32 LengthExtraBits[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr u32 LengthBase[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr u32 DistanceExtraBits[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
constexpr u32 DistanceBase[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr u32 CodeLengthCodesOrder[] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };

/**
 *
 * Constants used during checksum generation and
 * checking
 *
 */
constexpr u32 MOD_ADLER = 65521; //0xfff1
constexpr u32 ADLER_MASK = 0xffff;

#define ADLER32_BASE 1

//chck sum function
void adler32_compute_next(u32& cur, const byte val) {
    u32 a = cur & ADLER_MASK,
        b = (cur >> 0x10) & ADLER_MASK;

    a = (a + val) % MOD_ADLER;
    b = (b + a) % MOD_ADLER;

    cur = (b << 0x10) | a;
}

/**
 *
 * Constants used during tree encoding
 *
 */
#define RLE_Z2_BASE 0xb
#define RLE_Z1_BASE 0x3

#define RLE_Z2_MASK 0x7f
#define RLE_Z1_MASK 0x7

#define RLE_L_BITS  0x2
#define RLE_Z1_BITS 0x3
#define RLE_Z2_BITS 0x7

#define RLE_L_MASK 0x3
#define RLE_L_BASE 0x3

struct block_settings {
    const byte cmf, flg;
};

u64 bitReverse(u64 v, size_t nBits) {
    u64 rs = 0x00;
    for (i32 i = nBits - 1; i >= 0; i--)
        rs |= (((v & (1ULL << i)) >> i) & 1ULL) << ((nBits - 1ULL) - i); //yes just a lot of bit shifting stuff
    return rs;
}

//this is where cool stuff begins

//
struct treeComparison {
    bool operator()(huffman_node* a, huffman_node* b) {
        if (!a || !b) {
            std::cout << "Something ain't right ._." << std::endl;
            return false;
        }

        if (a->cmpCount == b->cmpCount)
            return a->depth > b->depth;
        return a->cmpCount > b->cmpCount;
    }
};

//generates the symbol codes for a given tree
void _GenSymCodes(huffman_node* root, huffman_node* node, i32 cCode = 0x00) {
    if (!root || !node) return;

    if (node->left != nullptr || node->right != nullptr) {
        if (node->left != nullptr)  _GenSymCodes(root, node->left, (cCode << 1));
        if (node->right != nullptr) _GenSymCodes(root, node->right, (cCode << 1) | 0x1);
    }
    else if (node->val < root->alphaSz)
        root->symCodes[node->val] = cCode;
}

//debug
void PrintTree(huffman_node* tree, std::string tab = "", std::string code = "") {
    if (tree == nullptr) return;
    if (tree->right == nullptr && tree->left == nullptr) {
        std::cout << tab << "  - " << tree->count << " " << code << " " << "  " << tree->val << "   " << (char)tree->val << std::endl;
        return;
    }

    std::cout << tab << "|_ " << tree->count << std::endl;
    if (tree->right != nullptr)
        PrintTree(tree->right, tab + " ", code + "1");

    if (tree->left != nullptr)
        PrintTree(tree->left, tab + " ", code + "0");
}

/**
 *
 * Generate Code Table
 *
 * Function to generate all character codes based off a given tree.
 * This must be called on a tree before you call Encode Symbol because
 * all Encode Symbol does is reference this table.
 *
 * *Note if this is not
 * called then EncodeSymbol will call it for you.
 *
 * If there is no good alphabet size then the alphabet size will be set
 * to DEFAULT_ALPHABET_SIZE (defined below)
 *
 * [HuffmanTreeNode*] tree -> huffman tree to generate code table for
 * [size_t] aSize -> size of the tree's alphabet
 *
 */

#define DEFAULT_ALPHABET_SIZE 288

 //
void GenerateCodeTable(huffman_node* tree, size_t alphaSz = 0) {
    if (tree->alphaSz == 0) {
        if (alphaSz <= 0)
            alphaSz = DEFAULT_ALPHABET_SIZE;

        tree->alphaSz = alphaSz;
    }

    //allocate code table
    if (tree->symCodes)
        _safe_free_a(tree->symCodes);

    tree->symCodes = new i32[alphaSz];
    ZeroMem(tree->symCodes, alphaSz);

    //generate codes
    _GenSymCodes(tree, tree);
}

/**
 *
 * TreeFree
 *
 * Used to free huffman trees
 *
 */
void Huffman::TreeFree(huffman_node *root) {
    if (root == nullptr) return;

    if (root->symCodes)
        _safe_free_a(root->symCodes);

    if (root->left) TreeFree(root->left);
    if (root->right) TreeFree(root->right);

    delete root;
    root = nullptr;
}

void Huffman::TreeFree(huffman_tree *tree) {
    Huffman::TreeFree(tree->root);
    _safe_free_a(tree->bitLens);
    ZeroMem(tree, sizeof(huffman_tree));
}

/**
 *
 * InsertNode
 *
 * Function used to insert values into a huffman tree during
 * the Inflate process. This is to construct the tree from a
 * list of codes generated by blListToTree
 *
 */

void Huffman::InsertNodeIntoTree(const u64 code, const size_t codeLen, const u64 val, huffman_node* tree) {
    huffman_node* current = tree;

    if (!current) current = new huffman_node{ .count = 0 };

    i64 i;

    for (i = codeLen - 1; i >= 0; i--) {
        bit b = (code >> i) & 1;

        if (!!b) {
            if (!current->right)
                current->right = new huffman_node{
                    .count = 0
                };

            current = current->right;
        }
        else {
            if (!current->left)
                current->left = new huffman_node{
                    .count = 0
                };

            current = current->left;
        }
    }

    current->val = val;
}

void Huffman::InsertNodeIntoTree(const u64 code, const size_t codeLen, const u64 val, huffman_tree* tree) {
    if (!tree) return;

    Huffman::InsertNodeIntoTree(code, codeLen, val, tree->root);
}

/**
 *
 * GenTreeFromCounts
 *
 * Generate a huffman tree from a bunch of char counts
 * Will auto filter out any characters with a count of 0
 *
 * Params:
 *  template: [size_t] alphaSz - size of the alphat (number of characters in count array)
 *  [size_t*] counts[alphaSz] - array containing the character counts in the given message
 *
 * Return:
 *  huffman_node * - root of the constructed huffman tree
 *
 * Errors:
 *  will return nullptr if counts are in valid in anyway
 *
 */

huffman_node* Huffman::GenTreeFromCounts(size_t* counts, size_t alphabetSize, i32 maxLen) {
    if (counts == nullptr || alphabetSize <= 0)
        return nullptr;

    //convert all of the char counts into a priority queue of huffman nodes
    std::priority_queue<huffman_node*, std::vector<huffman_node*>, treeComparison> tNodes;
    size_t _val = 0;
    foreach_ptr(size_t, count, counts, alphabetSize)
        if (*count > 0)
            tNodes.push(new huffman_node{
                .val = (u32)_val++,
                .count = *count,
                .cmpCount = *count,
                .leaf = true
            });
        else _val++;


    //tree root
    huffman_node* root = nullptr;

    //construct tree
    while (tNodes.size() > 1) {
        huffman_node* newNode = new huffman_node;
        newNode->left = tNodes.top();
        tNodes.pop();
        newNode->right = tNodes.top();
        tNodes.pop();
        newNode->depth = MAX(newNode->left->depth, newNode->right->depth) + 1;
        newNode->cmpCount = newNode->count = newNode->left->count + newNode->right->count;

        //max length thingy for le tree
        if (maxLen >= 0 && newNode->depth >= maxLen - 1) newNode->cmpCount = INT_MAX;

        root = newNode;
        tNodes.push(newNode);
    }

    return root;
}

/**
 *
 * getTreeBitLens
 *
 * gets the length of the bit lengths for a tree
 *
 */
u32* Huffman::GetTreeBitLens(huffman_node* tree, size_t alphabetSize, i32 currentLen, u32* cbl) {
    if (!cbl) {
        cbl = new u32[alphabetSize];
        ZeroMem(cbl, alphabetSize);
    }

    if (tree->right || tree->left) {
        if (tree->left) Huffman::GetTreeBitLens(tree->left, alphabetSize, currentLen + 1, cbl);
        if (tree->right) Huffman::GetTreeBitLens(tree->right, alphabetSize, currentLen + 1, cbl);
    }
    else {
        if (tree->val < alphabetSize)
            //cbl[tree->val] = currentLen > 0 ? currentLen : 1;
            cbl[tree->val] = mu_max(currentLen, 1);
        else
            std::cout << "Invalid bl_symbol [" << tree->val << "]" << "  " << alphabetSize << std::endl;
    }

    return cbl;
}

/**
 *
 * getBitLenCounts
 *
 * gets the bit lengths counts for a tree uh yeah
 *
 */
size_t* Huffman::GetBitLenCounts(u32* bitLens, const size_t blLen, const size_t MAX_BITS) {
    size_t* res = new size_t[MAX_BITS + 1];
    ZeroMem(res, MAX_BITS + 1);

    for (size_t i = 0; i < blLen; i++)
        res[mu_min(bitLens[i], MAX_BITS)]++;

    return res;
}

/**
 *
 * BitLengthsToHTree
 *
 * Converts the bit lengths of a tree into a canonical tree
 *
 * bitLens - bitLengths
 * blLEn - number of bit lengths
 *
 */
huffman_node* Huffman::BitLengthsToTree(u32* bitLens, size_t blLen, size_t aLen) {
    const size_t MAX_BITS = ArrMax<u32>(bitLens, blLen);
    size_t* blCount = Huffman::GetBitLenCounts(bitLens, blLen, MAX_BITS);

    if (!blCount)
        return nullptr;

    std::vector<i32> nextCode = { 0,0 };

    //get the next code for the tree items
    for (u32 codeIdx = 2; codeIdx <= MAX_BITS; codeIdx++)
        nextCode.push_back((nextCode[codeIdx - 1] + blCount[codeIdx - 1]) << 1);

    //construct tree
    i32 i = 0;
    huffman_node* res = new huffman_node();

    while (i < aLen && i < blLen) {
        if (bitLens[i] != 0) {
            Huffman::InsertNodeIntoTree(nextCode[bitLens[i]], bitLens[i], i, res);
            nextCode[bitLens[i]]++;
        }
        i++;
    }

    //free and return
    delete[] blCount;
    res->alphaSz = aLen;
    return res;
}

/**
 *
 *  CovnertTreeToCanonical
 *
 * converts a tree into its canonical form :O
 *
 */
huffman_node* Huffman::CovnertTreeToCanonical(huffman_node* tree, size_t alphabetSize, bool deleteOld) {
    //get bit lengths of each code
    u32* bitLens = Huffman::GetTreeBitLens(tree, alphabetSize);

    //start to create le tree
    huffman_node* cTree = Huffman::BitLengthsToTree(bitLens, alphabetSize, alphabetSize);

    //free memory and return new tree
    delete[] bitLens;

    if (deleteOld)
        Huffman::TreeFree(tree);

    cTree->alphaSz = alphabetSize;
    return cTree;
}

//helper function that increase bit length for all child nodes of a node
void _bl_inc(u32* bl, huffman_node* node, const size_t charMax) {
    if (!bl || !node) return;

    if (node->leaf) {
        if (node->val < charMax)
            bl[node->val]++; //increase bit length
    }
    else {
        if (node->left)
            _bl_inc(bl, node->left, charMax);
        if (node->right)
            _bl_inc(bl, node->right, charMax);
    }
}

class BobsDiganosis {
    bool operator()(u32* a, u32* b) {
        return *a > *b;
    }
};

/**
 *
 * Call Bob's bit length repair services today! to make
 * sure all your bit lengths are the right length!
 *
 * Bob's Bit Length Repair services have been the number
 * 1 bit length repair service for over 10 years!
 *
 * Don't delay! Call today!
 *
 * Call: 444-732-9191 for a price estimate
 *
 */

bool __cmp(u32* a, u32* b) {
    return *a > *b;
}

u32* BobsBitLengthRepairServices(u32* bitLens, size_t alphaSz, const i32 maxBl) {
    if (!bitLens || alphaSz == 0 || maxBl <= 0) return nullptr;

    //first sort the bit lengths into 2 vectors
    std::vector<u32*> good_bl, bad_bl;

    foreach_ptr(u32, B, bitLens, alphaSz)
        if (*B > maxBl)
            bad_bl.push_back(B);
        else if (*B < maxBl)
            good_bl.push_back(B);

    //then order the good vector by length
    std::sort(good_bl.begin(), good_bl.end(), __cmp);
    size_t g = 0;
    bool end = false;

    //next do the thing where you go bad bit length by bad bit length and subtract from it and add to the first element
    //in good until the bad bit length is good. 
    for (auto* bl : bad_bl) {
        if (end)
            break;
        while (*bl > maxBl) {
            (*bl)--;

            if (++(*good_bl[g]) >= maxBl - 1)
                g++;

            if (g >= good_bl.size()) {
                end = true;
                break;
            }
        }
    }

    //TODO: maybe verify and make sure debug went well

    return bitLens;
}

/**
 *
 * GenCanonicalTreeFromCounts
 *
 * generates a canonical tree slight faster then
 * creating a temp tree and them using convert
 * tree to canonical.
 *
 * Basically all it does is create a temp tree but
 * whilst creating it, bit lengths are counted so
 * you don't need to do the slow process of
 * also counting bit lengths
 *
 */
huffman_node* Huffman::GenCanonicalTreeFromCounts(size_t* counts, size_t alphaSz, i32 maxLen) {
    if (counts == nullptr || alphaSz <= 0)
        return nullptr;

    //convert all of the char counts into a priority queue of huffman nodes
    std::priority_queue<huffman_node*, std::vector<huffman_node*>, treeComparison> tNodes;
    size_t _val = 0;
    foreach_ptr(size_t, count, counts, alphaSz)
        if (*count > 0) {
            tNodes.push(new huffman_node{
                .val = (u32)_val++,
                .count = *count,
                .cmpCount = *count,
                .leaf = true
                });
        }
        else _val++;


    //tree root
    huffman_node* root = nullptr;

    u32* bitLens = new u32[alphaSz];
    ZeroMem(bitLens, alphaSz);

    bool callBob = false;

    //construct tree
    while (tNodes.size() > 1) {
        huffman_node* newNode = new huffman_node;
        newNode->left = tNodes.top();
        tNodes.pop();
        newNode->right = tNodes.top();
        tNodes.pop();
        newNode->depth = MAX(newNode->left->depth, newNode->right->depth) + 1;
        newNode->cmpCount = (newNode->count = (newNode->left->count + newNode->right->count));

        root = newNode;
        tNodes.push(newNode);

        //as we construct tree keep track of bitlengths
        _bl_inc(bitLens, newNode, alphaSz);

        if (maxLen >= 0 && newNode->depth > maxLen)
            callBob = true; //oh crap somethings wrong! must call Bob!
    }

    Huffman::TreeFree(root); //delete temporary tree

    if (callBob) {
        bitLens = BobsBitLengthRepairServices(bitLens, alphaSz, maxLen);

        if (!bitLens) {
            std::cout << "Error, Bob couldn't repair the bit lengths!" << std::endl;
            return nullptr;
        }
    }

    //generate canonical tree
    huffman_node* r = Huffman::BitLengthsToTree(bitLens, alphaSz, alphaSz);
    r->alphaSz = alphaSz;
    _safe_free_a(bitLens);
    return r;
}

/**
 *
 * GenCanonicalTreeInfFromCounts
 *
 * Same as GenCanonicalTreeFromCounts but
 * also returns the bit length info
 * generated during tree construction
 *
 */
huffman_tree Huffman::GenCanonicalTreeAndInfFromCounts(size_t* counts, size_t alphaSz, i32 maxLen) {
    if (!counts || alphaSz <= 0)
        return {};

    //convert all of the char counts into a priority queue of huffman nodes
    std::priority_queue<huffman_node*, std::vector<huffman_node*>, treeComparison> tNodes;
    size_t _val = 0;

    foreach_ptr(size_t, count, counts, alphaSz)
        if (*count > 0) {
            tNodes.push(new huffman_node{
                .val = (u32)_val++,
                .count = *count,
                .cmpCount = *count,
                .leaf = true
            });
        }
        else _val++;


    //tree root
    huffman_node* root = nullptr;
    huffman_tree t_inf = {
        .bitLens = new u32[alphaSz],
        .alphaSz = alphaSz
    };
    ZeroMem(t_inf.bitLens, alphaSz);

    bool callBob = false;

    //get bitlengths
    //TODO: there may be an invalid count if the last node isnt being counted but i doubt it
    if (tNodes.size() == 1) {
        _bl_inc(t_inf.bitLens, tNodes.top(), alphaSz);
    } 
    else while (tNodes.size() > 1) { 
        huffman_node* newNode = new huffman_node;
        newNode->left = tNodes.top();
        tNodes.pop();
        newNode->right = tNodes.top();
        tNodes.pop();
        newNode->depth = MAX(newNode->left->depth, newNode->right->depth) + 1;
        newNode->cmpCount = (newNode->count = (newNode->left->count + newNode->right->count));

        root = newNode;
        tNodes.push(newNode);

        //as we construct tree keep track of bitlengths
        _bl_inc(t_inf.bitLens, newNode, alphaSz);

        if (maxLen >= 0 && newNode->depth > maxLen)
            callBob = true; //oh crap somethings wrong! must call Bob!
    }

    TreeFree(root); //delete temporary tree

    if (callBob) {
        t_inf.bitLens = BobsBitLengthRepairServices(t_inf.bitLens, alphaSz, maxLen);

        if (!t_inf.bitLens) {
            std::cout << "Error, Bob couldn't repair the bit lengths!" << std::endl;
            return t_inf;
        }
    }

    //generate canonical tree
    t_inf.root = Huffman::BitLengthsToTree(t_inf.bitLens, alphaSz, alphaSz);
    t_inf.root->alphaSz = alphaSz;
    return t_inf;
}

/**
 *
 * GenCharacterCounts
 *
 * gets the count of characters in a bunch of bytes
 *
 * Params:
 *  [byte*] data - data to get the char counts of
 *  [size_t] sz - size of the data in bytes
 *
 * Return:
 *  u32* - char counts, will have a size of 0xff or BYTE_MAX
 *
 */

size_t* GetCharacterCounts(byte* data, size_t sz) {
    size_t* rCounts = new size_t[0xff];
    ZeroMem(rCounts, 0xff);
    byte* d_ptr = data;
    while (sz--) rCounts[*d_ptr++]++;
    return rCounts;
};

//same thing as prev function just for u32* instead of byte*
size_t* GetCharacterCounts(u32* data, size_t sz) {
    size_t* rCounts = new size_t[0xff];
    ZeroMem(rCounts, 0xff);
    u32* d_ptr = data;
    while (sz--) rCounts[*d_ptr++]++;
    return rCounts;
};

//some lz77 helper functions
size_t lz77_get_len_idx(size_t len) {
    size_t lastIdx = 0, i = 0;

    for (i32 l : LengthBase) {
        if (l > len) break;
        if (i++ == 0) continue;
        lastIdx++;
    }

    return lastIdx;
}

//get codes and indexs from distances
i32 lz77_get_dist_idx(size_t dist) {
    size_t lastIdx = 0, i = 0;

    for (i32 l : DistanceBase) {
        if (l > dist) break;
        if (i++ == 0) continue;
        lastIdx++;
    }

    return lastIdx;
}

/**
 *
 * lz77 encode
 *
 * function for the lz77 compression algorithm
 *
 */

/**
 *
 * lzr
 *
 * result from lz77 encoding
 *
 */
struct lzr {
    byte* encDat;
    size_t encSz, winBits, sz_per_ele = 2;
    huffman_tree distTree;
    size_t* charCounts;
    size_t charCountSz;
};

/**
 *
 * lz_inst
 *
 * internal instance thing used in lz77
 *
 */
struct lz_inst {
    byte* window = nullptr, * winStart = nullptr, * winMax = nullptr;
    size_t winBits = 0, winSz = 0, winPos = 0, lookAheadSz = 0;
};

struct _match {
    hash_node<byte*>* node;
    size_t matchSz;
};

struct match_settings {
    size_t maxLength = 0x100;
    size_t goodLength = 0xffff;
    size_t maxChainLength = 0xff; //how long hash chains can be
};

//gets longest match or something
//TODO: hyper optimize this function!!!
_match longest_match(hash_node<byte*>* firstMatch, lz_inst* ls, match_settings ms = {}) {
    if (!firstMatch || !ls)
        return {};

    if (!ls->window)
        return {};

    hash_node<byte*>* curMatch = firstMatch, *bestMatch = firstMatch;
    size_t bestSz = 0, chainLen = 0;

    const byte* _winEnd = ls->window + ls->lookAheadSz; //SUPER SLOW
    const byte* matchStart = ls->window;

    //TODO: copy upto 8 possible matches before comparing
    byte local_cache[2048] = {0};

    in_memcpy(local_cache, curMatch->val, 256);

    //get le best match
    while (curMatch && ++chainLen <= ms.maxChainLength) {
        size_t mSz = 0;

        //do le matching
        size_t curPos = 0, winEndPos = ls->lookAheadSz;

        //TODO: idk something with the cache is REALLY slow
        //somehow figure out how to write this code in a way there the curMatch
        //and all his friends are in the same region of memory;
        //maybe expriment more with optimizations in linked_map that could help
        //guarentee matchs are in close-by blocks of memory, or just make a 
        //hash node as few bytes of memory as possible by stuff stuff in a u32
        //since simple arithmetic operations take literally 0 time
        byte *cur = (byte*)matchStart,
             *comp = curMatch->val;

        if (!comp) {
            curMatch = curMatch->prev;
            continue;
        }
        
        do {} while (cur < _winEnd && comp < _winEnd && *cur++ == *comp++ && ++mSz < ms.maxLength);

        if (mSz > bestSz) {
            bestMatch = curMatch;
            bestSz = mSz;
        }

        if (mSz >= ms.goodLength) break;

        curMatch = curMatch->prev;
    }

    //return the best
    return {
        .node = bestMatch,
        .matchSz = bestSz
    };
}

/**
 *
 * EncodeSymbol
 *
 * Function to encode a symbol with a given huffman tree. This
 * is used by deflate for the huffman coding.
 *
 * [u32] sym -> symbol to be encoded
 * [HuffmanTreeNode*] tree -> huffman tree to encode the symbol
 * with
 *
 */


 //TODO: fix this function in this context
u32 Huffman::EncodeSymbol(u32 sym, huffman_node* tree) {
    assert(sym < tree->alphaSz);

    if (!tree->symCodes)
        GenerateCodeTable(tree, tree->alphaSz);

    if (tree->symCodes)
        return tree->symCodes[sym];
    else
        return 0u;
}

/**
 *
 * DecodeSymbol
 *
 * Function to decode a symbol with a given huffman tree. This
 * is used by inflate for basically all the huffman stuff
 *
 */

u32 Huffman::DecodeSymbol(BitStream* stream, huffman_node* tree) {
    huffman_node* n = tree;

    while (n->left || n->right) {
        bit b = stream->readBit();

        if (b) {
            if (!n->right) break;

            n = n->right;
        }
        else {
            if (!n->left) break;

            n = n->left;
        }
    }

    return n->val;
}

void lzr_free(lzr* l) {
    if (!l) return;

    if (l->encDat)
        _safe_free_a(l->encDat);

    if (l->distTree.root)
        Huffman::TreeFree(l->distTree.root);

    if (l->distTree.bitLens)
        _safe_free_a(l->distTree.bitLens);

    if (l->charCounts)
        _safe_free_a(l->charCounts);
}

/**
 *
 * Main lz77 function
 *
 * lz77_encode
 *
 * idk write somethign smart for this
 *
 * note when using function
 *
 * THE BUFFER RETURNED, STORES DATA AS
 * 3 BYTE INTEGERS!! IN A SIZE_T* (4 byte)
 * CREATE A CLASS OR SOMETHING TO HANDLE
 * DIFFERENT SIZED STREAM LIKE THIS ONE
 * TO READ 3 BYTE VALUES INSTEAD OF 4
 * BYTE
 *
 * could possibly fix if you didnt do copy
 * to buffer in the micro vector but oh
 * well
 *
 * Recommended solution: convert the size_t*
 * into a realigned buffer and then use the
 * buffer for further parsing
 *
 */
#define _WIN_SHIFT(ls, curByte, winShift) \
    (ls).window += (winShift); \
    (ls).winPos += (winShift); \
    (enc_byte) += (winShift)  

#define SIZE_PER_LZ_ELE 2
lzr lz77_encode(byte* dat, size_t sz, size_t winBits) {
    constexpr size_t nMallocs = 0xf;

    //just some window constants
    constexpr size_t lookAhead = 0x100;
    constexpr size_t winMem = 0x7fff - lookAhead; //32kb - loohAhead = other window memory

    //size of lz77 alphabet
    constexpr size_t lzAlphaSz = 0x120;

    //distance char counts for the distance tree
    constexpr size_t dsb_len = sizeof(DistanceBase) / sizeof(i32), lnb_len = sizeof(LengthBase) / sizeof(i32);
    size_t* distCharCount = new size_t[dsb_len];
    ZeroMem(distCharCount, dsb_len);

    //encode result
    lzr res = {
        .encDat = nullptr,
        .encSz = 0,
        .winBits = winBits,
        .sz_per_ele = SIZE_PER_LZ_ELE, //size per stored element (default is u16 2 bytes), but since i may change this later im leaving it variable
        .distTree = nullptr,
        .charCounts = nullptr,
        .charCountSz = lzAlphaSz
    };

    //construct important info and stuff
    lz_inst ls = {
        .window = dat,   //lz77 window ptr
        .winStart = dat, //where the window begins
        .winMax = dat + sz,
        .winBits = winBits,     //window bits (2 ^ b = s), where b is win bits and s is window size in bytes
        .winSz = (size_t)1 << winBits,  //size of window in bytes
        .winPos = 0,
        .lookAheadSz = lookAhead
    };

    //hash table
    linked_map<byte*, 15> hashTable;

    hashTable.EnablePreAllocMode();
    hashTable.preAlloc(winMem);

    //
    std::vector<u16> enc_dat;

    //enc_dat.reserve(sz); //reserve at least enough place for the number of characters

    //
    byte* enc_byte = dat, *end = dat + sz;

    //allocate char count container thing
    res.charCounts = new size_t[lzAlphaSz];
    ZeroMem(res.charCounts, lzAlphaSz);

    i32 bytesLeft = sz;

    //main encode loop
    do {
        //store the last
        //idk something with reinterpret cast is no good here
        //oh wait the error is that when reinterpreting, mem sanitize thinks the size is 1 when in
        //realtiy it's ~32kb
        const size_t tok_sz = mu_min(3, bytesLeft);

        //hash table is still kinda slow but not as bad as before :D
        hash_node<byte*>* last = hashTable.insert((char*) ls.window, tok_sz, ls.window);

        size_t winShift = 1;

        if (last) {
            //then back ref
            //SLOWEST PART OF THIS FUNCTION
            _match m = longest_match(last, &ls, {
                .maxLength = mu_min(lookAhead, bytesLeft)
            });

            if (!m.node) {
                _safe_free_a(distCharCount);
                //enc_dat.free();
                lzr_free(&res);
                return {};
            }

            if (m.matchSz <= 3) goto def_match; //make sure match is longer than 3 so it's worth it

            //encode back ref
            const size_t
                lenIdx = lz77_get_len_idx(m.matchSz),
                dist = (size_t)(ls.window - m.node->val), //get distance
                distIdx = lz77_get_dist_idx(dist); //get distance base

            //make sure things are valid
            if (distIdx >= dsb_len || distIdx < 0) {
                std::cout << "Balloon error: invalid match distance!";
                continue;
            }

            if (lenIdx >= lnb_len || lenIdx < 0) {
                std::cout << "Balloon error: invalid match length!";
                continue;
            }


            //get extras after quick error check
            const i32 distExtra = dist - DistanceBase[mu_min(distIdx, dsb_len)],
                lenExtra = m.matchSz - LengthBase[mu_min(lenIdx, lnb_len)];

            distCharCount[distIdx]++; //increase char count yk

            //if we get back ref change shift amount
            winShift = m.matchSz;

            //add back ref val
            const u16 sym = 257 + lenIdx;
            enc_dat.push_back(sym);
            res.charCounts[sym]++;

            enc_dat.push_back(distIdx);
            enc_dat.push_back(distExtra);
            enc_dat.push_back(lenExtra);
        }
        else {

        def_match:

            //encode basic character
            byte e_char = *ls.window;

            enc_dat.push_back(e_char); //slightly slow
            res.charCounts[e_char]++;
        }

        //window shifting
        _WIN_SHIFT(ls, curByte, winShift); //slightly slow
        bytesLeft -= winShift;

        if (ls.winPos >= winMem) {
            //SUPER SLOW!!
            hashTable.clear();
            ls.winPos = 0;
        }
    } while (enc_byte < end);

    hashTable.free();

    //construct distance tree and other stuff we need
    res.distTree = Huffman::GenCanonicalTreeAndInfFromCounts(distCharCount, dsb_len, MAX_CODE_LENGTH);

    if (!res.distTree.root || !res.distTree.bitLens) {
        std::cout << "Something went wrong when generating distance tree!" << std::endl;
    }

    //copy over data
    res.encSz = enc_dat.size() * res.sz_per_ele;
    res.encDat = new byte[res.encSz];
    ZeroMem(res.encDat, res.encSz);
    in_memcpy(res.encDat, enc_dat.data(), res.encSz);

    //mem management and return
    _safe_free_a(distCharCount);
    return res;
}

bool lzr_good(lzr* l) {
    if (!l) {
        std::cout << "lz77 res no good ;-;" << std::endl;
        return false;
    }

    if (!l->encDat) {
        std::cout << "Lz77 encDat is no good" << std::endl;
        return false;
    }

    if (!l->charCounts) {
        std::cout << "Lz77 char counts is no good" << std::endl;
        return false;
    }

    if (!l->distTree.root) {
        std::cout << "Lz77 dist tree is no good" << std::endl;
        return false;
    }

    if (!l->distTree.bitLens) {
        std::cout << "Lz77 bit lens is no good" << std::endl;
        return false;
    }

    return true;
}

//
struct _codeLenInf {
    size_t enc_sz = 0;
    u32* rle_dat;
    size_t* cl_counts;
};

//ccLens - combined code lengths
//nc - n_codes -> number of codes in combied code lengths
_codeLenInf _tree_rle_encode(u32* ccLens, size_t nc, const size_t codeLengthAlphaSz, const size_t bitLenMax) {
    std::vector<u32> e_dat;

    if (!ccLens || nc <= 0)
        return {};

    u32 pMatch; //match we comparing to

    //code length counts
    size_t* clCounts = new size_t[codeLengthAlphaSz];
    ZeroMem(clCounts, codeLengthAlphaSz);

    //
    for (
        u32* cur = ccLens, *end = ccLens + nc;
        cur < end;
        ) {

        //get longest match
        size_t mSz = 0;
        const size_t maxMatch = (!*cur) ? (RLE_Z2_MASK + RLE_Z2_BASE) : (RLE_L_MASK + RLE_L_BASE);
        pMatch = *cur;

        //make sure bitlength is valid
        if (*cur > bitLenMax) {
            std::cout << "RLE Encode Error! Invalid bitlength: " << *cur << " | Max is: " << bitLenMax << std::endl;
            _safe_free_a(clCounts);
            return {};
        }

        while (cur < end && *cur == pMatch && mSz < maxMatch) {
            cur++;
            mSz++;
        }

        //quick error check :3
        if (pMatch > codeLengthAlphaSz) {
            std::cout << "Error, invalid code length!" << std::endl;
            return {};
        }

        //increase count thing
        clCounts[pMatch] += mSz;

        //encode le boi
        if (!pMatch) {//pMatch is 0 so we so a special rle thing
            if (mSz >= RLE_Z1_BASE)
                if (mSz >= RLE_Z2_BASE) {
                    e_dat.push_back(18); //18 is back ref for match RLE_Z2_BASE < mSz
                    clCounts[18]++;
                    e_dat.push_back((mSz - RLE_Z2_BASE) & RLE_Z2_MASK);
                }
                else {
                    e_dat.push_back(17); //17 is back ref for match RLE_Z1_BASE <= mSz <= RLE_Z2_BASE
                    clCounts[17]++;
                    e_dat.push_back((mSz - RLE_Z1_BASE) & RLE_Z1_MASK);
                }
            else
                while (mSz--)
                    e_dat.push_back(0); //or you could do pMatch
        }
        else {//normal rle thing
            if (mSz > 0 && mSz - 1 >= RLE_L_BASE) {
                e_dat.push_back(pMatch);
                e_dat.push_back(16);
                clCounts[16]++;
                e_dat.push_back(((mSz - 1) - RLE_L_BASE) & RLE_L_MASK);
            }
            else
                while (mSz--)
                    e_dat.push_back(pMatch);
        }
    }

    //now create the little package that stores all the rle data nicely
    _codeLenInf res = {
        .enc_sz = e_dat.size(),
        .rle_dat = new u32[res.enc_sz],
        .cl_counts = clCounts
    };

    ZeroMem(res.rle_dat, res.enc_sz);
    in_memcpy(res.rle_dat, e_dat.data(), sizeof(u32) * res.enc_sz);

    return res;
}

//goal: encode literal and distance tree
//
//
void _stream_tree_write(BitStream* stream, huffman_tree* litTree, huffman_tree* distTree) {
    if (!litTree || !distTree || !stream) {
        std::cout << "Error invalid tree!" << std::endl;
        return;
    }

    //remember to probably omit trailing zeros or something
    //first combine the bit lengths from literal and distance tree
    const size_t ncbl = litTree->alphaSz + distTree->alphaSz;
    u32* combinedBl = new u32[ncbl];

    ZeroMem(combinedBl, ncbl);

    size_t litOff;

    in_memcpy(combinedBl, litTree->bitLens, (litOff = litTree->alphaSz) * sizeof(u32));
    in_memcpy(combinedBl + litOff, distTree->bitLens, sizeof(u32) * distTree->alphaSz);

    //next create the code length tree
    const size_t codeLengthAlphaSz = 19; //number of codes for code length alphabet

    _codeLenInf clRleInf = _tree_rle_encode(combinedBl, ncbl, codeLengthAlphaSz, MAX_CODE_LENGTH); //rle encoding

    if (!clRleInf.rle_dat) {
        std::cout << "Error failed to rle encode!" << std::endl;
        _safe_free_a(combinedBl);
        _safe_free_a(clRleInf.cl_counts);
        _safe_free_a(clRleInf.rle_dat);
        return;
    }

    i32 HLIT = litTree->alphaSz - 257,
        HDIST = distTree->alphaSz - 1,
        HCL = codeLengthAlphaSz - 4;

    if (HLIT < 0 || HDIST < 0 || HCL < 0) {
        std::cout << "Error invalid Lit tree length, Dist tree length, or Code length tree length!" << std::endl;
        _safe_free_a(combinedBl);
        _safe_free_a(clRleInf.cl_counts);
        _safe_free_a(clRleInf.rle_dat);
        return;
    }

    //write all le values to the stream
    stream->writeBits(HLIT, 5);
    stream->writeBits(HDIST, 5);
    stream->writeBits(HCL, 4);

    //make the code length tree
    const i32 CL_MAX_CODE_LENGTH = 7; //0b111 (since stored as 3 bits)
    huffman_tree clTree = Huffman::GenCanonicalTreeAndInfFromCounts(clRleInf.cl_counts, codeLengthAlphaSz, CL_MAX_CODE_LENGTH);

    //encode the literal and distance tree
    constexpr size_t __rESz = sizeof(u32); //size of 1 rle encoded element

    if (!clTree.bitLens || !clTree.root) {
        std::cout << "Failed to generate code length tree!" << std::endl;
        goto mem_free;
    }

    //write the code length tree bit length data to the stream
    for (i32 i = 0; i < codeLengthAlphaSz; i++) {
        const size_t clbl = clTree.bitLens[CodeLengthCodesOrder[i]];

        if (clbl > CL_MAX_CODE_LENGTH) {
            std::cout << "Error cl code length too long!" << std::endl;
            goto mem_free;
        }
        else
            stream->writeBits(clTree.bitLens[CodeLengthCodesOrder[i]], 3);
    }

    //TODO: NOT EVERY POINTER IS A VOID POINTER YOU DONUT
    for (
        u32* cur = clRleInf.rle_dat, *end = clRleInf.rle_dat + clRleInf.enc_sz;
        cur < end;
        cur++
        ) {
        u32 sym = *cur;
        size_t code_len = clTree.bitLens[sym];

        stream->writeBits(
            bitReverse(
                Huffman::EncodeSymbol(sym, clTree.root),
                code_len
            ),
            code_len
        );

        //have to add more data if it isn't just the symbol
        if (sym >= 16) {
            size_t nWBits = 0;

            //get number of bits that are gonna be written
            switch (sym) {
            case 16:
                nWBits = RLE_L_BITS;
                break;
            case 17:
                nWBits = RLE_Z1_BITS;
                break;
            case 18:
                nWBits = RLE_Z2_BITS;
                break;
            }

            if (nWBits <= 0) {
                std::cout << "[Warning] invalid number of bits during rle tree write!" << std::endl;
                continue;
            }

            //write the next value that is gonna be copied a bunch
            stream->writeBits(
                *++cur,
                nWBits
            );
        }
    }

    //memory cleanup
mem_free:
    _safe_free_a(combinedBl);
    _safe_free_a(clRleInf.cl_counts);
    _safe_free_a(clRleInf.rle_dat);
    _safe_free_a(clTree.bitLens);
    Huffman::TreeFree(clTree.root);
}

/**
 *
 * _lzr_stream_write
 *
 * Compresses and writes the data from a lz77 stream into a byte stream
 *
 * if writeCapByte is true it will write a final compressed 0x100 value
 * to the stream to indicate the end of a block
 *
 * just a thought, make sure lz77_encode doesn't write the 0x100 at all
 *
 */
void _lzr_stream_write(BitStream* stream, lzr* l, u32* checksum, bool writeCapByte = true) {
    if (!stream || !l)
        return;

    //generate encode tree
    if (writeCapByte && l->charCountSz >= 256)
        l->charCounts[256]++;
    huffman_tree enc_tree_inf = Huffman::GenCanonicalTreeAndInfFromCounts((size_t*)l->charCounts, l->charCountSz, MAX_CODE_LENGTH);

    if (!enc_tree_inf.bitLens)
        return;

    //write the trees
    _stream_tree_write(stream, &enc_tree_inf, &l->distTree);

    //encode all the data
    *checksum = ADLER32_BASE;

    realigned_buffer<byte, 2> dat_buf = realigned_buffer<byte, 2>(l->encDat, l->encSz);

    dat_buf.computeLength();

    const size_t buf_len = dat_buf.length;

    for (i32 i = 0; i < buf_len; i++) { //maybe this one
        u32 sym = dat_buf[i]; //this line is slow!!!

        adler32_compute_next(*checksum, sym);

        const size_t bl = enc_tree_inf.bitLens[sym];

        //back ref
        if (sym >= 257) {
            size_t
                distIdx = dat_buf[++i] & 0xff,
                distExtra = dat_buf[++i] & 0xffff,
                lenExtra = dat_buf[++i] & 0xffff,
                lenIdx = sym - 257;

            size_t distBl = l->distTree.bitLens[distIdx];

            //write the back reference
            stream->writeBits(bitReverse(Huffman::EncodeSymbol(sym & 0xfff, enc_tree_inf.root), bl), bl); //length base
            stream->writeBits(lenExtra, LengthExtraBits[lenIdx]); //length extra
            stream->writeBits(bitReverse(Huffman::EncodeSymbol(distIdx, l->distTree.root), distBl), distBl); //distance base
            stream->writeBits(distExtra, DistanceExtraBits[distIdx]); //distance extra
        }
        else
            stream->writeBits(
                bitReverse(
                    Huffman::EncodeSymbol(sym, enc_tree_inf.root),
                    bl
                ),
                bl
            );
    }
    
    //writes a 0x100 to the end to indicate the end of a block
    if (writeCapByte && enc_tree_inf.alphaSz >= 256) {
        const size_t bl = enc_tree_inf.bitLens[256];
        stream->writeBits(bitReverse(Huffman::EncodeSymbol(256, enc_tree_inf.root), bl), bl);
    }

    //stream->writeValue(*checksum); //ok don't write the checksum during blocks
    _safe_free_a(enc_tree_inf.bitLens);
    Huffman::TreeFree(enc_tree_inf.root);
}

/**
 *
 * DeflateBlock
 *
 * deflates a given block of data
 *
 */

enum deflate_block_type {
    dfb_none,
    dfb_static,
    dfb_dynamic
};

/**
 *
 * WriteDeflateBlockToStream
 *
 * Compresses the data given then writes a deflate block to the given stream
 *
 */
i32 WriteDeflateBlockToStream(BitStream* stream, bin* block_data, const size_t winBits, const i32 level, u32* checksum, bool finalBlock = false) {
    deflate_block_type bType = level > 0 ? dfb_dynamic : dfb_none;

    //padding for debug. WILL BREAK STREAM IF UNCOMMENTED!!!
    //stream->writeBits(0, 8-stream->subBitTell());
    //stream->writeUInt16(0xffff);

    //write some info
    stream->writeBit(finalBlock & 0b01);
    stream->writeBit((bType & 0b01));
    stream->writeBit((bType & 0b10) >> 1);

    //padding for debug. WILL BREAK STREAM IF UNCOMMENTED!!!
    //stream->writeBits(0, 8-stream->subBitTell());
    //stream->writeUInt16(0xffff);

    //compresss
    switch (level) {

    //no compression so we just write the raw bin data
    case 0: {
        const u16 blckSz = block_data->sz, nlen = 0;
        stream->writeUInt16(blckSz);
        stream->writeUInt16(nlen);
        stream->writeBytes(block_data->dat, block_data->sz);
        break;
    }

    //TODO: other levels from 0-10
    case 1: {
        lzr mimic = {
            .charCountSz = 0xff
        };

        mimic.charCounts = GetCharacterCounts(block_data->dat, block_data->sz);

        //mimic a distance tree
        u32* distBl = new u32[30];
        ZeroMem(distBl, 30);

        mimic.distTree = {
            .root = nullptr,
            .bitLens = distBl,
            .alphaSz = 30
        };


        //set data
        mimic.encDat = block_data->dat;
        mimic.encSz = block_data->sz;

        //set some more constants
        mimic.winBits = 15;
        mimic.sz_per_ele = 1;

        //encode to stream
        _lzr_stream_write(stream, &mimic, checksum, true); //reason for true is stated below
        lzr_free(&mimic); //VERY SLOW
        break;
    }

          //max compressoin
    case 2: {

        //lz77 encode the data
        lzr lzDat = lz77_encode(block_data->dat, block_data->sz, winBits);

        if (!lzr_good(&lzDat)) {
            std::cout << "Failed to encode block! LZ77 Fail!" << std::endl;
            lzr_free(&lzDat);
            return 1;
        }

        _lzr_stream_write(stream, &lzDat, checksum, true /*true value makes sure 0x100 value is added at the end*/); //write the lzr stream
        lzr_free(&lzDat); //VERY SLOW
        break;
    }
    default:
        return 0;
    }

    return 0;
}

/**
 *
 * Balloon::Deflate
 *
 * Core deflate function for zlib. Compresses all
 * the data and returns a result containing the
 * compressed data
 *
 */

balloon_result Balloon::Deflate(byte* data, size_t sz, u32 compressionLevel, const size_t winBits) {
    //quick error check
    if (!data || sz <= 0 || winBits > 15) return {};

    //BalloonStream rStream = BalloonStream(0xff);

    BitStream rStream = BitStream();

    //generate some of the fields

    /*
    
    cmf values:

    compression method: 8 (bits 1-4)
    winBits: 15 -> 7 (bits 5-8)
    
    */

    byte cmf = (0x08) | (((winBits - 8) & 0b1111) << 4);
    /*
    
    compression type values (bits 7-8)

    00 - fastest
    01 - fast
    10 - default <-
    11 - max compression

    Dictioanry values (bit 6)

    0 - no <-
    1 - yes
    
    */
    byte flg = ((2 << 1) | 0); //we use default compression so 0b10 << 1 and no dictionary (0)

    //flg = 0x1f - (cmf * 0x100) % 0x1f; //compute the flag checksum
    //better computation of flag check sum
    const byte flgExt = (cmf * 256 + flg) % 31;

    flg <<= 5; //make room for flag extension
    flg |= (31 - flgExt);

    auto start = std::chrono::high_resolution_clock::now();

    //write the cmf and flag bytes
    rStream.writeByte(cmf);
    rStream.writeByte(flg);

    //deflate everything officially
    //TODO: multiple blocks of data
    // do the above TODO when adding multi threading

    u32 checksum = 0;

    i64 bytesLeft = sz;
    byte* blockStart = data;

    size_t totalSz = 0, nBlocks = 0;

    while (bytesLeft > 0) {
        size_t blockSz = mu_min(bytesLeft, BLOCK_SIZE_MAX);

        bin block_dat = {
            .dat = blockStart,
            .sz = blockSz
        };

        if (WriteDeflateBlockToStream(&rStream, &block_dat, winBits, compressionLevel, &checksum, bytesLeft - BLOCK_SIZE_MAX <= 0))
            std::cout << "Error something went wrong when writing deflate block!" << std::endl;

        blockStart += blockSz;
        bytesLeft -= blockSz;
        totalSz += blockSz;
        nBlocks++;

#ifdef MSFL_DEBUG
        std::cout << "Wrote Block: " << nBlocks << std::endl;
        std::cout << "----------------------" << std::endl;
#endif
    }

    rStream.writeUInt32(checksum);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

#ifdef MSFL_DEBUG
    std::cout << "Encode Speed: " << (((double)totalSz / duration.count()) * 1e3) << " kb/s" << std::endl;
    std::cout << "N Blocks: " << nBlocks << std::endl;
#endif

    //construct result
    balloon_result res = {
        .data = rStream.getBytePtr(),
        .sz = rStream.size(),
        .checksum = checksum,
        .compressionMethod = (byte)(compressionLevel & 0xff)
    };

    //NOTE: dont free stream!!! (this is because we dont memcpy to result struct it just points to the streams memory)

    return res;
}

//structure for a simple inflate block
struct InflateBlock {
    byte* data;
    size_t sz;
    bool blockFinal;
    deflate_block_type block_type;
};

/**
 *
 * _decode_trees
 *
 * Decodes the distance and literal trees from the
 * bitstream
 *
 * Returns a pointer to 2 trees where the first is
 * the lit tree and the second is the distance tree
 *
 */
huffman_node** _decode_trees(BitStream* stream) {
    if (!stream)
        return nullptr;

    size_t nLitCodes = stream->readBits(5) + 257,
        nDistCodes = stream->readBits(5) + 1,
        nClCodes = stream->readBits(4) + 4;

#ifdef MSFL_DEBUG
    std::cout << "Tree Info:" << "\n\tNLitCodes: " << nLitCodes << "\n\tNDistCodes: " << nDistCodes << "\n\tNClCodes: " << nClCodes << std::endl;
#endif

    //extract cl bit lengths
    const size_t N_CLCODES = 19;
    u32* clBitLens = new u32[N_CLCODES];
    ZeroMem(clBitLens, N_CLCODES);

    forrange(nClCodes)
        clBitLens[CodeLengthCodesOrder[i]] = stream->readBits(3);

    //create code length tree
    huffman_node* clTree = Huffman::BitLengthsToTree(clBitLens, N_CLCODES, N_CLCODES);

    if (!clTree) {
        std::cout << "Inflate Error, failed to read or create code length tree!" << std::endl;
        return nullptr;
    }

    //read combined bit lengths
    const size_t nTotalCodes = nLitCodes + nDistCodes;
    u32* combinedBitLens = new u32[nTotalCodes];
    ZeroMem(combinedBitLens, nTotalCodes);

    foreach_ptr_m(u32, curBl, combinedBitLens, nTotalCodes) {
        u32 sym = Huffman::DecodeSymbol(stream, clTree);

        if (sym <= 15)
            *curBl++ = sym;
        else
            switch (sym) {

                //basic back ref
            case 16: {
                //verify we aren't gonna do an invalid back ref
                if (curBl <= combinedBitLens)
                    continue;

                //simple back ref
                size_t copyLen = stream->readBits(RLE_L_BITS) + RLE_L_BASE;
                u32 prev = *(curBl - 1);
                forrange(copyLen)
                    * curBl++ = prev;
                break;
            }
                   //zero back ref 1
            case 17: {
                size_t copyLen = stream->readBits(RLE_Z1_BITS) + RLE_Z1_BASE;
                forrange(copyLen)
                    * curBl++ = 0;
                break;
            }
                   //zero back ref 2
            case 18: {
                size_t copyLen = stream->readBits(RLE_Z2_BITS) + RLE_Z2_BASE;
                forrange(copyLen)
                    * curBl++ = 0;
                break;
            }
            default:
                std::cout << "Inflate warning, Invalid rle symbol: " << sym << std::endl;
                curBl++;
                break;
            }
    }

    //TODO: generate the trees based on the combined bit lengths

    //extract individual bit lengths
    u32* litBl = new u32[nLitCodes],
        * distBl = new u32[nDistCodes];

    ZeroMem(litBl, nLitCodes);
    ZeroMem(distBl, nDistCodes);

    in_memcpy(litBl, combinedBitLens, nLitCodes * sizeof(u32));
    in_memcpy(distBl, combinedBitLens + nLitCodes, nDistCodes * sizeof(u32));

    //create the 2 trees
    huffman_node* litTree = Huffman::BitLengthsToTree(litBl, nLitCodes, nLitCodes),
        * distTree = Huffman::BitLengthsToTree(distBl, nDistCodes, nDistCodes);

    //manage le memory and error check
    _safe_free_a(combinedBitLens);
    _safe_free_a(litBl);
    _safe_free_a(distBl);

    if (!litTree) {
        std::cout << "Failed to generate literal tree!" << std::endl;
        if (distTree)
            Huffman::TreeFree(distTree);
        return nullptr;
    }

    if (!distTree) {
        std::cout << "Failed to generate distance tree!" << std::endl;
        if (litTree)
            Huffman::TreeFree(litTree);
        return nullptr;
    }

    //create container for the 2 decoded trees
    huffman_node** _tContain = new huffman_node * [2];
    _tContain[0] = litTree;
    _tContain[1] = distTree;

    return _tContain;
}

void _inflate_block_generic(InflateBlock* block, InflateBlock* prev_block, BitStream* stream, huffman_node* litTree, huffman_node* distTree, const block_settings settings) {
    
    #ifdef MSFL_DEBUG
    std::cout << "Inflating Normal Block @" << stream->tell() << std::endl;
    #endif

    if (!litTree || !distTree)
        return;

    std::vector<byte> dec_data;

    for (;;) {
        const u32 sym = Huffman::DecodeSymbol(stream, litTree);

        if (sym <= 255) {
            dec_data.push_back((byte)sym & 0xff);
        } else if (sym == 256) {
            break;

        //back ref :O
        } else {
            const size_t lenIdx = sym - 257;

            //get length
            const u32 lenExtraBits = LengthExtraBits[lenIdx],
                lenExtra = stream->readBits(lenExtraBits),
                len = LengthBase[lenIdx] + lenExtra;
            //get distance
            const size_t distIdx = Huffman::DecodeSymbol(stream, distTree);

            const u32 distExtraBits = DistanceExtraBits[distIdx],
                distExtra = stream->readBits(distExtraBits),
                dist = DistanceBase[distIdx] + distExtra;

            //copy character from back ref
            size_t decSz = dec_data.size();
            forrange(len) {
                if (dist > decSz) {
                    if (prev_block) {
                        const size_t subDist = dist - decSz, prevSz = prev_block->sz;
                        if (subDist < prevSz) {
                            dec_data.push_back(prev_block->data[
                                prevSz - subDist
                            ]);
                        } else {
                            std::cout << "Balloon warning: accessing far far back data!" << std::endl;
                            dec_data.push_back(0);
                        }
                    } else {
                        std::cout << "Balloon warning: accessing far far back data!" << std::endl;
                        dec_data.push_back(0);
                    }
                } else {
                    byte dbg_sym;
                    dec_data.push_back(dbg_sym = dec_data[decSz - dist]);
                }
                decSz++;
            }
        }

        if (!stream->canAdv()) {
            std::cout << "Balloon warning: reached stream end without end token!" << std::endl;
            break;
        }
    }

    //copy over data into the block
    block->data = new byte[block->sz = dec_data.size()];
    ZeroMem(block->data, block->sz);
    in_memcpy(block->data, dec_data.data(), block->sz);
}

void _inflate_block_none(InflateBlock* block, BitStream* stream, const block_settings settings) {
    if (!block || !stream)
        return;

    block->sz = stream->readUInt16();
    const size_t nBlockSz = stream->readUInt16();

    block->data = new byte[block->sz];

    //read data
    foreach_ptr(byte, b, block->data, block->sz)
        *b = stream->readByte();
}

void _inflate_block_static(InflateBlock* block, InflateBlock* prev_block, BitStream* stream, const block_settings settings) {
    if (!block || !stream)
        return;

    u32* bitLens = new u32[DEFAULT_ALPHABET_SIZE];
    ZeroMem(bitLens, DEFAULT_ALPHABET_SIZE);
    const size_t u3sz = sizeof(u32);

    //copy values to bit length table (use memfill instead of memset since we are setting u32-by-u32 not byte-by-byte
    memfill(bitLens      , 8u, 144);
    memfill(bitLens + 144, 9u, 112);
    memfill(bitLens + 256, 7u, 24);
    memfill(bitLens + 280, 8u, 8);

    //create the literal trwee
    huffman_node* llTree = Huffman::BitLengthsToTree(bitLens, DEFAULT_ALPHABET_SIZE, DEFAULT_ALPHABET_SIZE);
    _safe_free_a(bitLens);

    //quick error check
    if (!llTree) {
        std::cout << "Huffman static ini failed!" << std::endl;
        return;
    }

    u32* distBitLens = new u32[30];
    ZeroMem(distBitLens, 30);

    memfill(distBitLens, 5u, 30);

    //create distance tree
    huffman_node* distTree = Huffman::BitLengthsToTree(distBitLens, 30, 30);
    _safe_free_a(distBitLens);

    //quick error check again
    if (!distTree) {
        std::cout << "Huffman static ini failed 2!" << std::endl;
        return;
    }

    //inflate block
    _inflate_block_generic(block, prev_block, stream, llTree, distTree, settings);

    Huffman::TreeFree(distTree);
    Huffman::TreeFree(llTree);
}

void _inflate_block_dynamic(InflateBlock* block, InflateBlock* prev_block, BitStream* stream, const block_settings settings) {
    if (!block || !stream)
        return;

    huffman_node** trees = _decode_trees(stream);

    if (!trees) {
        std::cout << "Error failed to decode trees!" << std::endl;
        return;
    }

    _inflate_block_generic(block, prev_block, stream, trees[0], trees[1], settings);

    //memory management
    Huffman::TreeFree(trees[0]);
    Huffman::TreeFree(trees[1]);
    _safe_free_a(trees);
}

//
InflateBlock _stream_block_inflate(BitStream* stream, InflateBlock* prev_block, block_settings blck_settings) {
    if (!stream)
        return { 0 };

    //basic block creationg and get settings
    InflateBlock block = {
        .data = nullptr,
        .sz = 0,
        .blockFinal = (bool)stream->readBit(),
        .block_type = (deflate_block_type)stream->readBits(2)
    };

    //decode block data
    switch (block.block_type) {
    case dfb_none:
        _inflate_block_none(&block, stream, blck_settings);
        break;
    case dfb_static:
        _inflate_block_static(&block, prev_block, stream, blck_settings);
        break;
    case dfb_dynamic:
        _inflate_block_dynamic(&block, prev_block, stream, blck_settings);
        break;
    default:
        std::cout << "Zlib error! Invalid block type @" << stream->tell() << "." << stream->subBitTell() << std::endl;
        stream->bitSeek((stream->tell() << 3));
        std::cout << "T-Byte: " << (i32)stream->readByte() << std::endl; 
        stream->__printDebugInfo();
        break;
    }

    return block;
}

//inflate
balloon_result Balloon::Inflate(byte* data, size_t sz) {
    if (!data || sz <= 0)
        return {};

    //create a simple stream
    BitStream datStream = BitStream(data, sz);

    const byte cmf = datStream.readByte();
    const byte flg = datStream.readByte();

    if ((cmf * 256 + flg) % 31 != 0) {
        std::cout << "Inflate Error, invalid flag checksum!!" << std::endl;
        return {};
    }

    //verify compression mode
    i32 compressionMode = cmf & 0xf;

    if (compressionMode != 8) {
        std::cout << "Inflate Error, invalid compression mode!!" << std::endl;
        return {};
    }

    //dictionary check
    const bool dictionary = (flg >> 5) & 1;

    if (dictionary) {
        std::cout << "Inflate Error, dictionary not supported!" << std::endl;
        return {};
    }

    //result creationg thing
    balloon_result res = {
        .compressionMethod = (byte)((cmf >> 6) & 3)
    };

    //now inflate the blocks
    bool eos = false;

    size_t totalBlockSize = 0;

    std::vector<InflateBlock> blocks;

    const block_settings _blck_set = {
        .cmf = cmf,
        .flg = flg
    };

    size_t nBlocks = 0 ;
    InflateBlock *prev_block = nullptr;

    while (!eos) {
        InflateBlock c_block = _stream_block_inflate(
            &datStream, 
            prev_block,
            _blck_set
        );
        //std::cout << "Block Start: " << totalBlockSize << std::endl;
        totalBlockSize += c_block.sz;
        blocks.push_back(c_block);
        prev_block = &blocks[nBlocks]; //access block from vector since c_block will be deleted when scope ends
        eos = c_block.blockFinal || !datStream.canAdv();
        nBlocks ++ ;
    }

#ifdef MSFL_DEBUG
    std::cout << "Decoded: " << nBlocks << " blocks" << std::endl;
#endif

    //assemble blocks
    res.data = new byte[res.sz = totalBlockSize];
    ZeroMem(res.data, res.sz);
    byte* curChunk = res.data, * datEnd = res.data + res.sz;

    for (auto& block : blocks) {
        if (!block.data)
            continue;

        //copy block data
        in_memcpy(curChunk, block.data, block.sz);
        curChunk += block.sz;
        _safe_free_a(block.data);
        block.sz = 0;

        //check to make sure we don't read anything invalid
        if (curChunk > datEnd)
            break;
    }

    datStream.free();

    return res;
}

void Balloon::Free(balloon_result* res) {
    if (!res) return;
    _safe_free_a(res->data);
    ZeroMem(res, sizeof(balloon_result));
}

enum file_stream_type {
    file_stream_in,
    file_stream_out
};

/*template<file_stream_type _sty> class BinFileStream {
    std::fstream _f;
    bool iok = false;
public:
    BinFileStream() {
    }
    BinFileStream(std::string src) {
        if (src.length() <= 0)
            return;

        this->_f = std::fstream(src, (_sty == file_stream_in ? std::ios::in : std::ios::out) | std::ios::binary);

        this->iok = _f.good();
    }
    bool ok() {
        return this->iok;
    }
};

struct FileChunk {
    byte* dat;
    size_t sz;
};

class DualStream {
private:
    BinFileStream<file_stream_in> in;
    BinFileStream<file_stream_out> out;
    bool iok = false;
public:
    DualStream(std::string in_src, std::string out_src) {
        if (in_src.length() <= 0 || out_src.length() <= 0) return;

        this->in = BinFileStream<file_stream_in>(in_src);
        this->out = BinFileStream<file_stream_out>(out_src);

        this->iok = this->in.ok() && this->out.ok();
    }
    FileChunk reqChunk(size_t sz) {
        if (sz <= 0)
            return {};

    }
    bool ok() {
        return this->iok;
    }
    bool writeChunk(FileChunk chunk) {
        if (!chunk.dat || chunk.sz <= 0)
            return false;
    }
};

bool DeflateFileToFile(std::string in_src, std::string out_src, u32 compressionLevel = 2, const size_t winBits = 0xf) {
    if (in_src.length() <= 0 || out_src.length() <= 0) return false;

    DualStream f_stream = DualStream(in_src, out_src);

    if (!f_stream.ok())
        return false;
}*/

//line 861, lets see how off this comment gets
//line 1231, yeah that comment above is way off rn, but lets so how off this one gets :3
//line 1561, bruh how is the comment above off by 300 lines after 1 day ;-;
//line 1939, it hasn't even been a day since the last comment and this comment is now on line 1939!!! HOW???? 400-700 LINES IN 1 DAY :O
//line 2314, yep not even close to done and im already in the mid 2,000s, dang
//line 2390, finally done with most features and not too far off from last time, this line about to be much high do to remobing debug tho lol
//line 2400 something, yeah removed all the old stream stuff since it was slow ;-;