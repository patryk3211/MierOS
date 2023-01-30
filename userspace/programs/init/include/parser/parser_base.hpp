#pragma once

#include <iostream>
#include <optional>
#include <fstream>

namespace minit {
    class ParserBase {
        std::ifstream f_fileReader;
        bool f_permissiveParsing;

    public:
        ParserBase(const std::string& filename, bool permissive);
        ~ParserBase() = default;

        bool parse();

    protected:
        virtual bool parse_line(const std::string& line) = 0;
    };
}
