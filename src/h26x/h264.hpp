#include <iostream>
#include "../bitstream.hpp"
#include "../msutil.hpp"

struct nal_unit {
    struct nal_header {
        u8 ty, ref_idc;
    } header;
    byte *rbsp = nullptr;
    size_t sz;
};

struct sps {
    u8 profile, level;
    u16 id, max_frame_num;
    u8 pic_order_count_ty;
    flag constraint_flags[6];
     
};

struct pps {
    struct pps_header {

    } header;
};

struct slice {

};

struct h264Context {

};

class Nal {
public:
    static nal_unit decode_slice();
};

class h264 {
public:

};