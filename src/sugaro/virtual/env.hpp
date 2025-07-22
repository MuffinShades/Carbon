#include <iostream>
#include "../../msutil.hpp"

constexpr size_t cacheSize = 0x1FFF;
constexpr size_t varStartAddr = cacheSize + 1;
constexpr size_t stackSize = 0x1FFF;

enum class CactiInstruction {
    NOP = 0x00,
    RET = 0x10,
    EXT = 0x11
};

class Env {
public:
    void *vMem = nullptr;
    const size_t memBlockSize = 0xFFFF;
};