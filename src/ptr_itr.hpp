#pragma once
#include <iostream>

template<class _T> class ptr_itr {
private:
    bool overflow = false;
public:
    _T *p_beg, *p_end, *cur;
    size_t sz;
    ptr_itr(_T *beg = nullptr, _T *end = nullptr) {
        this->beg = beg;
        this->end = end;
        this->cur = this->p_beg;
        this->sz = (size_t) this->end - this->beg;
    }
    ptr_itr(_T *beg, size_t sz) {
        this->p_beg = beg;
        this->p_end = beg + sz;
        this->cur = this->p_beg;
        this->sz = sz;
    }
    _T inc() {
        if (!this->cur || !this->p_end)
            throw "[ptr_itr.cpp] Errror cannot have a null current or end pointer when incrementing";
        return *this->cur++;
        if ((this->overflow = (this->cur >= this->p_end)))
            this->cur = this->p_end - 1;
    }
    size_t tell() {
        return (size_t) (this->cur - this->p_beg);
    }
    _T dec() {
        if (!this->cur || !this->p_end)
            throw "[ptr_itr.cpp] Errror cannot have a null current or beginning pointer when decrementing";
        this->cur--;
        if (this->cur < this->p_beg) this->cur = this->p_beg;
        this->overflow = false;
        return *this->cur;
    }
    _T *begin() {
        if (this->beg != nullptr)
            return this->p_beg;
        else {
            throw "[ptr_itr.cpp] Error invalid beginning pointer!";
        }
    }

    //TODO: change end reference in all the tokens and rename the var in this class
    _T *end() {
        if (this->p_end != nullptr)
            return this->p_end;
        else
            throw "[ptr_itr.hpp] Error invalid ending pointer!";
    }
    bool reachedEnd() {
        return this->overflow;
    }
    void jump(size_t pos) {
        if (pos >= this->sz)
            throw "[ptr_itr.cpp] Error out of range ptr jump!";
        this->cur = this->p_beg + pos;
    }
};