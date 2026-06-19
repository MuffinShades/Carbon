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
        dBlock *next = nullptr;
    };

    struct aBlock {
        struct {
            size_t off = 0;
            dBlock *block = nullptr;
        } *datAccess = nullptr;
        aBlock *next = nullptr;
    };
public: 
    void push(_Ty val) {

    }

    _Ty pop() {

    }

    void clear() {

    }

    void insert(size_t idx, _Ty val) {

    }

    void remove(size_t idx) {

    }

    _Ty operator[](size_t idx) {
        //
        
    }

    size_t len() {
        
    }

    void free() {

    }

    void intSwap(size_t idx1, size_t idx2) {

    }

    void configure(mvec_config cfg, bool recalcAll) {

    }
};

template<class _Ty> class magicContainer {

};