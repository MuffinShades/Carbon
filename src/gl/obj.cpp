#include "obj.hpp"
#include "../filewrite.hpp"

Mesh ObjParse::LoadMeshFromObjFile(std::string src) {
    if (src.length() == 0)
        return {};

    file f_data = FileWrite::readFromBin(src);

    if (!f_data.dat || f_data.len == 0) {
        if (f_data.dat) _safe_free_a(f_data.dat);
        std::cout << "obj error: failed to open file!" << std::endl;
        return {};
    }

    char *scan = reinterpret_cast<char*>(f_data.dat), *scan_end = scan + f_data.len;
    char cur = 0;

    bool eof = false;

    //helper functions
    auto scan_next = [&]() {
        if (scan < scan_end) return *scan++;
        return (char) 0xff;
    };

    auto skip_line = [&]() {
        while (cur != '\n' && (unsigned) cur != 0xff) {
            cur = scan_next();
        }

        if ((unsigned)cur == 0xff)
            eof = true;
    };

    auto read_str = [&](std::string beg = "") {
        std::string str = beg;

        cur = scan_next();

        while (cur != ' ' && cur != 0 && (unsigned) cur != 0xff && cur != '\t' && cur != '\n') {
            str += cur;
            cur = scan_next();
        }

        return str;
    };

    auto read_str_cur = [&](std::string beg = "") {
        std::string str = beg;

        while (cur != ' ' && cur != 0 && (unsigned) cur != 0xff && cur != '\t' && cur != '\n') {
            str += cur;
            cur = scan_next();
        }

        return str;
    };

    auto read_float = [&]() {
        f32 f = 0.0f;

        while (cur >= '0' && cur <= '9') {
            f *= 10.0f;
            f += (f32)(cur - '0');
            cur = scan_next();
        }
        
        if (cur != '.') return f;
        cur = scan_next();

        f32 d_mod = 1.0f;

        while (cur >= '0' && cur <= '9') {
            d_mod *= 0.1f;
            f += ((f32)(cur - '0')) * d_mod;
        }

        return f;
    };

    std::vector<vec3> vs;
    std::vector<vec2> tv;

    for (;!eof;) {
        cur = scan_next();

        while (cur == 0 || cur == ' ' || cur == '\n' || cur == '\t')
            cur = scan_next();

        if ((unsigned) cur == 0xff)
                break;

        switch (cur) {
        case 'v': {
            char vMod = scan_next();
            switch (vMod) {
            //vertex
            case ' ': {
                cur = scan_next(); //skip the space

                vec3 v;

                //read in vertex data
                v.x = read_float();
                if (scan_next() != ' ') goto vertex_spacing_err;
                cur = scan_next();
                v.y = read_float();
                if (scan_next() != ' ') goto vertex_spacing_err;
                cur = scan_next();
                v.z = read_float();

                vs.push_back(v);

                break;
            }
            //normal
            case 'n': {

                break;
            }
            //texture
            case 't': {
                if ((cur = scan_next()) != ' ') {
                    std::cout << "obj error: unpexteced token: " << cur << std::endl;
                }

                vec2 v;

                //read in vertex data
                cur = scan_next(); //start at first float num
                v.x = read_float();
                if (scan_next() != ' ') goto vertex_spacing_err;
                cur = scan_next();
                v.y = read_float();

                tv.push_back(v);

                break;
            }
            }
            break;
        }
        //face
        case 'f': {

            break;
        }
        //comment
        case '#':
            skip_line();
            break;
        //unknown
        default:
            std::cout << "obj warning: unknown line begin \"" << read_str_cur() << "\"" << std::endl;
            skip_line();
            break;
        }

        if (false) {
        vertex_spacing_err:
            std::cout << "obj error: unexpected vertex deliminator, expected a space!" << std::endl;
        }
    }
}