#include "content.hpp"
#include "msutil.hpp"
#include <fstream>

ContentSrc ContentSrc::FromFile(std::string path) {

}

ContentSrc ContentSrc::FromBinary(byte *dat, size_t sz, bool make_copy, bool free_src) {
    if (!dat || sz == 0)
        return {};

    ContentSrc src;

    //have computed these values so
    src.computed_fdat = true;
    src.computed_sz = true;

    //change format
    src.fmt = ContentSrc::SourceFormat::Binary;

    //copy over data or what not
    src.sz = sz;

    if (make_copy) {
        src.f_dat = new byte[sz];
        ZeroMem(src.f_dat, sz);
        in_memcpy(src.f_dat, dat, sz);

        if (free_src) {
            _safe_free_a(dat);
        }
    } else
        src.f_dat = dat;

    return src;
}

size_t ContentSrc::size() {

    //compute size if not computed already
    if (!this->computed_sz) {
        this->computed_sz = true;

        //compute size based on file
        if (this->fmt == SourceFormat::File) {
            std::ifstream r_stream;

            r_stream.open(this->f_src);

            if (!r_stream.good()) {
                goto sz_compute_fail;
            }

            r_stream.seekg(std::ios::end);
            this->sz = r_stream.tellg();
            r_stream.close();
        } 
        //failed to compute size
        else {
        sz_compute_fail:
            this->sz = 0;
            return 0;
        }
    }

    return this->sz;
}

#include "filewrite.hpp"

//TODO: slightly optimize by using possibly already computed size / length
byte *ContentSrc::data() {
    if (this->f_dat)
        return this->f_dat;

    //compute data if not computed already
    if (!this->computed_fdat) {
        this->computed_fdat = true;

        //compute size based on file
        if (this->fmt == SourceFormat::File) {
            file f = FileWrite::readFromBin(this->f_src);

            this->sz = f.len;
            this->f_dat = f.dat;

            if (!this->f_dat) goto dat_compute_fail;
            if (this->sz == 0) {
                if (this->f_dat)
                    _safe_free_a(this->f_dat);

                goto dat_compute_fail;
            }

            this->computed_sz = true;
        } 
        //failed to compute size
        else {
        dat_compute_fail:
            this->f_dat = nullptr;
            return nullptr;
        }
    }

    return this->f_dat;
}

std::string ContentSrc::src() {
    return this->f_src;
}

ContentSrc::SourceFormat ContentSrc::format() {
    return this->fmt;
}

void ContentSrc::free() {
    if (this->f_dat)
        _safe_free_a(this->f_dat);

    this->f_dat = nullptr;
    this->f_src = "";
    this->sz = 0;

    this->computed_fdat = false;
    this->computed_sz = false;
}