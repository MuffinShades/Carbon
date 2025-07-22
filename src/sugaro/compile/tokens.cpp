#include "tokens.hpp"
#include "util.hpp"
#include "lang_info.hpp"
#include "dbg_tools.hpp"
#include "ptr_itr.hpp"

void tok_flush(Token &tok, std::vector<Token> &tokens) {
    if (tok.rawValue.length() <= 0) return;

    //second interation check
    

    tok.ty = tok_literal;
    tokens.push_back(tok);
}

uint64_t bitShuffle64(uint64_t v, int pattern[64]) {
    uint64_t res = 0;

    for (int i = 0; i < 64; i++) {
        res <<= 1;
        res |= (v >> pattern[i]) & 1;
    }

    return res;
};

//TODO: add more shuffle functions
uint64_t shuffle1(uint64_t v) {
    const int ptrn[64] = {63,0,62,1,61,2,60,3,59,4,58,5,57,6,56,7,55,8,54,9,53,10,52,11,51,12,50,13,49,14,48,15,47,16,46,17,45,18,44,19,43,20,42,21,41,22,40,23,39,24,38,25,37,26,36,27,35,28,34,29,33,30,32,31};
    return bitShuffle64(v, (int*)ptrn);
};

//compute checksum used for faster comparrison of string literals
//TODO: make this a more random checksum yaknow for better security and lower collision chance
//TODO: also add a small 65kb hash table based on the checksum to for even faster token search speeds
void computeTokenCheckSum(Token& tok) {
    tok.len = tok.rawValue.length();

    const char *rv = tok.rawValue.c_str();

    if (tok.len == 0) {
        tok.raw_checksum = 0;
        return;
    }

    //compute
    const uint64_t pMod = 0x7fffffff; //largest prime number < 32 bit integerlimit
    const uint64_t rXor = 0xf4446b05;

    uint64_t sum = 0;

    size_t l = tok.len;
    char c;
    while (l--) {
        sum += (c=*rv++);
        sum = shuffle1((~sum * c) & pMod);
    }

    tok.raw_checksum = sum % 0xffffffff;
}

//generates a token
void gen_token(Token& tok, ptr_itr<const char>* p) {
    //std::cout << "sym: " << *cur << std::endl;
    auto _itr = [](
        const std::string itms[],
        const size_t itms_size, 
        size_t& c_len, 
        std::string& symCollect,
        size_t& lastBestMatch,
        int& match
    ) {
        size_t i = 0, len;
        std::string cmp;
        
        for (; i < itms_size && (len = (cmp = itms[i++]).length()) > 0;) {
            const size_t I = i - 1; //value of I that should be used for the current loop cycle
            if (len != c_len) continue;
            if (_strCompare(cmp, symCollect, c_len)) {
                lastBestMatch = c_len;
                match = I;
                return true;
            }
        }

        return false;
    };

    //get token mode
    int mode = 0;

    if (IS_NUMERICAL(*p->cur)) mode = 1;
    else if (IS_TOK_SYMBL(*p->cur)) mode = 2;
    else if (IS_ALPHA(*p->cur)) mode = 3;

    //parse based on mode
    switch (mode) {
        case 1: { //number
            uint64_t tnum = 0;

            while (IS_NUMERICAL(*p->cur) && !p->reachedEnd()) {
                tnum *= 10;
                tnum += p->inc() - '0';
            };
            p->dec();

            //add symbol
            tok.ty = tok_num_literal;
            tok.mode = 1; //mode 1 -> numbers
            tok.rawiValue = tnum;
            tok.rawValue = std::to_string(tnum);

            break;
        }
        case 2: { //symbol
            std::string symCollect = "";
            size_t c_len = 0;
            size_t lastBestMatch = 0;
            int match = -1;
            const size_t _returnPos = p->tell();
            while (IS_TOK_SYMBL(*p->cur) && !p->reachedEnd()) {
                symCollect += p->inc();
                c_len++;
                //check comparators and operators
                //TODO: add precalculations to optimize (like lengths and max lengths for fast checking)
                if(!_itr(operators, n_operators, c_len, symCollect, lastBestMatch, match))
                    break;
            };

            //std::cout << "\t SYM: " << symCollect << " | " << *cur << " | " << lastBestMatch << std::endl;

            //jump to last best match
            if (lastBestMatch > 0) {
                symCollect = symCollect.substr(0, lastBestMatch);
                p->jump(_returnPos + lastBestMatch - 1);
                tok.enum_id = match;
            } else
                p->dec();

            //construct token
            tok.ty = tok_operator;
            tok.rawValue = symCollect;

            break;
        }
        case 3: { //literal thingy
            std::string symCollect = "";
            size_t c_len = 0;
            size_t lastBestMatch = 0;
            int match = -1;
            const size_t _returnPos = p->tell();
            while (IS_ALPHA_NUMERICAL(*p->cur) && !p->reachedEnd()) {
                symCollect += p->inc();
                //check keywords
                //TODO: add precalculations to optimize (like lengths and max lengths for fast checking)
                //TODO: only do this outside of the loop
                _itr(keywords, n_keywords, ++c_len, symCollect, lastBestMatch, match);
            };

            tok.ty = tok_literal;

            //jump to last best match
            if (lastBestMatch > 0) {
                symCollect = symCollect.substr(0, lastBestMatch);
                p->jump(_returnPos + lastBestMatch - 1);
                tok.ty = tok_keyword;
                tok.enum_id = match;
            } else
                p->dec();

            //construct token
            tok.rawValue = symCollect;
            break;
        }
        default: {
            dbg.Error("Error Invalid Token: "+std::to_string(*p->cur));
            break;
        }
    }
}

std::vector<Token> TokenGenerator::genProgramTokens(const char* source, size_t srcLen) {
    int pgrm_line = 1;
    if (source == nullptr || srcLen <= 0) return std::vector<Token>(0);

    if (!glblLoad)
        _load_glbl_data();

    std::vector<Token> tokens;

    Token tok = {
        .ty = tok_notype
    };

    //create the pointer interator
    ptr_itr<const char> p = ptr_itr<const char>(source, srcLen);

    dbg.Log("Starting token generation!");

    do {
        switch (*p.cur) {
            case ';': {
                tok.ty = tok_semi;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case ':': {
                tok.ty = tok_colon;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case '{': {
                tok.ty = tok_lbrack;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case '}': {
                tok.ty = tok_rbrack;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case '(': {
                tok.ty = tok_lparen;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case ')': {
                tok.ty = tok_rparen;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case '[': {
                tok.ty = tok_lsqbrack;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case ']': {
                tok.ty = tok_rsqbrack;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case '@': {
                tok.ty = tok_at;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case ',': {
                //tok_flush(tok, tokens);
                tok.ty = tok_comma;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case '.': {
                tok.ty = tok_decimal;
                tok.rawValue = std::string(p.cur, 1);
                break;
            }
            case ' ': break;
            case '\n': {
                pgrm_line++;
                break;
            }
            default: {
                gen_token(tok, &p);
                break;
            }
        }

        if (tok.ty != tok_notype) {
            computeTokenCheckSum(tok);
            tokens.push_back(tok);
            tok.src.lineNumber = pgrm_line;
            dbg.Log("Generated Program Token ["+std::to_string(tokens.size())+"]: "+std::to_string((int)tok.ty)+" | "+tok.rawValue+" | Check Sum: "+std::to_string(tok.raw_checksum) + " | Line: "+std::to_string(tok.src.lineNumber));
            tok.ty = tok_notype;
            tok.rawValue = "";
        }
    } while(++p.cur <= p.p_end);

    tokens.push_back({
        .ty = tok_pgrm_end
    });

    return tokens;
};

bool Token::is(std::string v) {
    return _strCompare(v, this->rawValue, this->rawValue.length());
}

bool Token::isTypeOf(TokenType ty) {
    return this->ty == ty;
}