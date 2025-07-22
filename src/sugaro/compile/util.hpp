#pragma once
#include <iostream>
#include <vector>
#include <string>

#define null {}

static bool _strCompare(std::string s1, std::string s2, size_t len) {
    const char *s1c = s1.c_str(), *s2c = s2.c_str();
    size_t pos = 0;

    if (len <= 0) return false;

    do {
        if (*(s1c + pos) != *(s2c + pos)) return false;
    } while (++pos < len);

    return true;
}

//char pointer to byte pointer
#define CPTBP(ptr) reinterpret_cast<unsigned char*>(const_cast<char*>((ptr)));

static double convertToDecimal(uint64_t dec) {
    double lg = log10(dec);
    return (double) dec / powf(10.0f, ceil(lg));
}

#define max(a,b) ((a) > (b) ? (a) : (b))