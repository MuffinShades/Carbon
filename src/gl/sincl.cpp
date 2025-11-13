/*

Sincl

Shader include utilities that expand upon GLSL and add include functionality and
other useful things to make writing shader code easier

Copyright Lambdana Software 2025
Programmed by muffinshades

--Usage--

#include "name" -> includes all functions from another shader
#include <lib>
#include_fn "name"@fn -> include specific function
#include_fn <lib>@fn

versions using quack brackets point to a library directory which includes a 
binary that is stored on either the side or in the executable itself which
has all the binaries included

--Ideas to Implement--

Create shader templates that can be included or used that can allow different
types of vertex data to be used

Include templates too

Shader Package File that can package a bunch of shader libs together into a 
half pre-compiled binary that devs can use to easily include new shaders 
people develop

*/

#include "sincl.hpp"
#include  "../lang/tokens.hpp"
#include "../msutil.hpp"
#include "filewrite.hpp"
#include "linked_map.hpp"

//WARNING: if you change the type of this you have to change how the variable below is calculated!!
constexpr std::string valid_glsl_version_profiles[] = {
    "core",
    "es"
};

constexpr size_t n_valid_glsl_version_profiles = sizeof(valid_glsl_version_profiles) / sizeof(std::string);

struct pgrm_fn {
    std::string name;
    std::string body;
};

std::string incl_keywords[] = {
    "include",
    "include_fn"
};

//so quack brackets can be detected
std::string incl_operators[] = {
    "<",
    ">"
};

const lang_info lang_inf_GLSL_incl = {
    incl_operators,
    sizeof(incl_operators) / sizeof(std::string),
    incl_keywords,
    sizeof(incl_keywords) / sizeof(std::string)
};

std::vector<pgrm_fn> get_glslShaderFunctions(std::vector<Token> pgrm_tokens) {
    i32 i, i_return;

    /*
    
    last_fn_tk_check explained

    void cool_function(){}
                      ^---

    ^ is where is checked first
    then in the smallest possible function:
        ){} must follow in order to be valid
    
    */
    const size_t nTokens = pgrm_tokens.size(), last_fn_tk_check = nTokens - 3;
    Token tk;

    for (i = 0; i < last_fn_tk_check; i++) {
        tk = pgrm_tokens[i];
        i_return = i;

        i32 j = i+1, alt_check = 0;

        if (!tk.isTypeOf(TokenType::tok_lparen))
            continue;

        //***check the parameters*** (right side)
        //first check to make sure there even are params
        tk = pgrm_tokens[++i];

        if (tk.isTypeOf(TokenType::tok_rparen))
            goto fn_body_check;

        while (j < nTokens) {
            if (alt_check & 1) {

            } else {

            }

            alt_check++;
        }

        if (false) {
        fn_check_fail:
            continue;
        }

        //check function body
        fn_body_check:



        //check left side of function (name and return type)

    } 
}

//flag that enables or disables the __ihistory param in the addshader include
//this is only toggled by _sincl_src_finder::getSourceData() functions so previously
//included files aren't included multiple times
bool Sincl::ihistory_enable = false;

//

enum class include_src_region {
    unknown,
    dir, //directory includes like #include "file.glsl"
    lib  //library includes like #include <library.glsl> or #include <library>
};

//wild is an arbutrary counter that is reserved for usage in a whole decay storage 
//when storing already processed files in memory
//ignore it till I add the whole static decay storage thing a ma bob
struct finder_inst {
    u16 wild = 0;
};

/*

in specific mode the finder will only search and extract certain functions
addSpecific --> adds a specific function the finder should search for when calling "getSrcData()"

*/

class _sincl_src_finder {
private:
    include_src_region rgn = include_src_region::unknown;
    std::string src_path;
    bool valid = false, spec = false;

    struct incl_history_node {
        incl_history_node **includes = nullptr;
        size_t n_includes = 0, n_alloc_includes = 0;
    };

    void free_ihistory_node_no_del(incl_history_node *n) {
        if (!n) return;

        i32 i;

        for (i = 0; i < n->n_includes; i++)
            free_ihistory_node(n->includes[i]);

        _safe_free_a(n->includes);
    }

    void free_ihistory_node(incl_history_node *n) {
        if (!n) return;

        i32 i;

        for (i = 0; i < n->n_includes; i++)
            free_ihistory_node(n->includes[i]);

        _safe_free_a(n->includes);
        _safe_free_b(n);
    }

    void alloc_ihistory_node(incl_history_node *n) {
        if (!n || n->n_alloc_includes == 0) return;

        if (n->includes) {
            free_ihistory_node_no_del(n);
        }

        n->includes = new incl_history_node*[n->n_alloc_includes];
        ZeroMem(n->includes, n->n_alloc_includes);
        n->n_includes = 0;
    }

    std::string getSpecificSourceData() {

    }

    std::string getSourceData(void *ihistory_ref = nullptr) {
        if (src_path.length() == 0)
            return "";
        
        file raw = FileWrite::readFromBin(src_path);

        if (!raw.dat || raw.len == 0) {
            if (raw.dat)
                _safe_free_a(raw.dat);

            return "";
        }

        //process the raw file
        incl_history_node *h = new incl_history_node;
        
        constexpr size_t assumed_inital_max_include = 256;

        //allocate the first node
        h->n_alloc_includes = assumed_inital_max_include;
        alloc_ihistory_node(h);

        //start to process includes and other ihistory nodes
        bool dis_ihist = false;
        if (!Sincl::ihistory_enable) {
            Sincl::enable_ihistory();
            dis_ihist = true;
        }

        //do all the processing and stuff here


        //disable the ihistory param
        if (dis_ihist)
            Sincl::disable_ihistory();
    }
public:
    _sincl_src_finder(include_src_region rgn, std::string src_path, bool specific = false) {
        this->rgn = rgn;

        if (src_path.length() == 0)
            return;

        this->valid = true;
        this->spec = specific;
    }
    std::string getSrcData(void *ihistory = nullptr) {
        //TODO: make sure you check ihistory flag

        //parse the whole path

        //read from the file

        //return or some shit
    }
    void addSpecific(std::string s) {
        if (s.length() == 0)
            return;

        //TODO: this function
    }
    void clearSepcifics() {
        //TODO: this function
    }
};

_sincl_exp AddShaderInclude(const char *src, size_t src_len, Sincl::Options settings, void *__ihistory) {
    if (!src || src_len == 0)
        return {};

    //this stuff is used a lil later in the function
    enum class sector_src_type {
        unknown,
        intra, //source code itself
        inter  //within other sources / libraries
    };

    enum class include_mode {
        na,
        base,
        fn
    };

    struct output_sector {
        sector_src_type ty = sector_src_type::unknown;
        size_t sz;
        size_t offset;
    };

    //
    const char* c_src = TokenGenerator::cleanCode(src, src_len);
    const size_t c_src_len = strlen(c_src);

    std::vector<Token> pgrm_tokens = TokenGenerator::genProgramTokens(c_src, c_src_len, lang_inf_GLSL_incl);

    _sincl_exp res = {
        .new_src = (char*) c_src,
        .src_len = c_src_len
    };

    i32 i, j;
    size_t n_tok = pgrm_tokens.size(),
           last_proc_tok = n_tok + 3; //when processing and include statement, there must be at least 3 tokens left for the forms "asdf" and <asdf>
    const size_t last_incl_proc_tok = n_tok + 3;
    size_t prev_chunk_end = 0;
    Token tk;
    bool barrier = false;

    //have to change this since versions can be specified as #version [number] or #version [number] [profile]
    if (settings.remove_version)
        last_proc_tok = n_tok + 2;

    //skeleton of the output file
    std::vector<output_sector> output_skeleton;
    size_t output_sz = 0;

    for (i = 0; i < last_proc_tok; i++) {
        tk = pgrm_tokens[i];
        if (tk.isTypeOf(TokenType::tok_tag)) {
            barrier = true;
            continue;
        } else if (!barrier) {
            i++;
            continue;
        }

        if (i < 1) continue; //simple barrier to prevent possibly subtracting 1 from an i val of 0 below when computing intra chunk size

        std::string f_target;

        include_mode mode = include_mode::na;
        include_src_region region = include_src_region::unknown;

        if (tk.is("include"))
            mode = include_mode::base;
        else if (tk.is("include_fn"))
            mode = include_mode::fn;
        else if (settings.remove_version && tk.is("version")) {
            //remove the version specifier
            tk = pgrm_tokens[++i];

            if (!tk.isTypeOf(TokenType::tok_num_literal)) {
                std::cout << "Invalid glsl version number type! Version number is NaN!!" << std::endl;
                delete[] c_src;
                return {};
            }

            if (i >= last_proc_tok)
                continue;

            //look for the profile or something; idk why I have to do this but alas I must
            size_t last_tok_line = tk.src.lineNumber;
            tk = pgrm_tokens[i+1];

            if (tk.src.lineNumber != last_tok_line) continue; //new line so no profile
            if (!tk.isTypeOf(TokenType::tok_literal)) { //profile must be defined as a literal; nothing else
                std::cout << "Invalid glsl version profile type [1]!" << std::endl;
                delete[] c_src;
                return {};
            }
            
            //check the valid glsl profiles
            bool valid_profile = false;
            for (j = 0; j < n_valid_glsl_version_profiles; j++) {
                if (tk.is(valid_glsl_version_profiles[i])) {
                    valid_profile = true;
                    break;
                }
            }

            if (!valid_profile) {
                std::cout << "Invalid glsl version profile type [2]!" << std::endl;
                delete[] c_src;
                return {};
            }

            i++; //inc i since if we got here there must be a valid profile thingy

            continue;
        }
        else {
            barrier = false;
            continue;
        }

        //can't process an include if there aren't enough tokens
        //TODO: throw an error or something instead of simply just skipping
        if (i >= last_incl_proc_tok)
            continue;

        //create a new output sector

        //intra chunk
        const size_t intra_chunk_sz = (i - 1) - prev_chunk_end;
        output_sector intra_chunk = {
            .ty = sector_src_type::intra,
            .sz = intra_chunk_sz,
            .offset = prev_chunk_end
        };
        output_skeleton.push_back(intra_chunk);

        //inter chunk

        //extract the target
        tk = pgrm_tokens[++i];

        switch (tk.ty) {
        //"
        case TokenType::tok_double_quote: {
            tk = pgrm_tokens[++i];

            if (!tk.isTypeOf(TokenType::tok_literal)) {
                std::cout << "sincl error! invalid include format" << std::endl;
                delete[] c_src;
                return {};
            }

            //extract the stuff
            f_target = tk.rawValue;

            //check to make sure next token is a double quote then dip
            tk = pgrm_tokens[++i];
            if (!tk.isTypeOf(TokenType::tok_double_quote)) {
                std::cout << "sincl error! invalid include format" << std::endl;
                delete[] c_src;
                return {};
            }

            region = include_src_region::dir;
            break;
        }
        //<
        case TokenType::tok_operator: {
            if (!tk.is("<")) {
                std::cout << "sincl error! invalid include format" << std::endl;
                delete[] c_src;
                return {};
            }

            //
            tk = pgrm_tokens[++i];

            if (!tk.isTypeOf(TokenType::tok_literal)) {
                std::cout << "sincl error! invalid include format" << std::endl;
                delete[] c_src;
                return {};
            }

            //extract the stuff
            f_target = tk.rawValue;

            //check to make sure next token is a double quote then dip
            tk = pgrm_tokens[++i];
            if (!tk.isTypeOf(TokenType::tok_operator) && !tk.is(">")) {
                std::cout << "sincl error! invalid include format" << std::endl;
                delete[] c_src;
                return {};
            }

            region = include_src_region::lib;
            break;
        }
        default: {
            std::cout << "sincl error! invalid include format" << std::endl;
            delete[] c_src;
            return {};
        }
        };

        //create the whole source parser thingy
        _sincl_src_finder src_read = _sincl_src_finder(region, f_target, mode == include_mode::fn);

        //insert the target's data and do the whole fancy
        //TODO: add the whole access from something or actually do something with the include_mode::base case
        switch (mode) {
        case include_mode::base:
            //idk
            break;
        case include_mode::fn: {
            //parse the additional @ to get the required function name
            if ((i32)last_proc_tok - i < 2) {
                std::cout << "sincl error! invalid function include format!" << std::endl;
                delete[] c_src;
                return {};
            }

            tk = pgrm_tokens[++i];

            if (!tk.isTypeOf(TokenType::tok_at)) {
                std::cout << "sincl error! invalid function include format!" << std::endl;
                delete[] c_src;
                return {};
            }

            tk = pgrm_tokens[++i];

            if (!tk.isTypeOf(TokenType::tok_literal)) {
                std::cout << "sincl error! invalid function include format!" << std::endl;
                delete[] c_src;
                return {};
            }

            std::string fn_target = tk.rawValue;
            src_read.addSpecific(tk.rawValue);
            break;
        }
        default:
            std::cout << "wtf memory corruption or something" << std::endl;
            break;
        }

        std::string inter_data = src_read.getSrcData();

        //compute new prev_chunk_end


        barrier = false;
    }

    //convert the skeleton into source code
    //add 1 for the nul terminator
    const size_t output_wterm_sz = output_sz + 1;
    char *o_src = new char[output_wterm_sz];
    ZeroMem(o_src, output_wterm_sz);

    size_t w_pos = 0; //write position in the output

    for (const auto sec : output_skeleton) {

    }

    //pack output source into a sincl_exp and export it

}