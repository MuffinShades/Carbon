#pragma once
#include "msutil.hpp"
#include <iostream>
#include <vector>
#include <string>

//just allows me to easily switch what kind of file path is being used
class Path {
public:
    static std::string project_dir;
    static std::string GetOSPath(std::string path) {
        if (path.length() <= 0) return "";
        std::vector<std::string> pathItems = SplitString(path, '/');
        std::string out = "";
        #ifdef WIN32
            const char pDelim = '\\';
        #else
            const char pDelim = '/';
        #endif
        size_t _ = 0;
        bool __ = (char) path[path.length()-1] == '/';
        for (const std::string i : pathItems) {
            if (i.length() <= 0) {
                if (_++ > 0)
                    return "Invalid Path!";
                else
                    continue;
            }

            out += i + pDelim;
        }

        if (!__)
            return project_dir+out.substr(0,out.length()-1);
        return project_dir+out;
    };
    static void SetProjectDir(std::string dir) {
        project_dir = "";
        project_dir = Path::GetOSPath(dir);
    };
};