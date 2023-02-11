#include <parser/parser_base.hpp>

using namespace minit;

ParserBase::ParserBase(const std::string& filename, bool permissive)
    : f_fileReader(filename, std::ios::in) {
    f_permissiveParsing = permissive;
}

bool ParserBase::parse() {
    std::string line;
    while(std::getline(f_fileReader, line)) {
        // Remove comments
        auto commentIdx = line.find('#');
        if(commentIdx != std::string::npos)
            line = line.substr(0, commentIdx);

        if(!parse_line(line) && !f_permissiveParsing)
            return false;
    }

    return true;
}

