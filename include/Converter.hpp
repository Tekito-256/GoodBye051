#pragma once

#include "header.hpp"

#include <string>
#include <fstream>
#include <vector>

using PatternMask = std::vector<std::pair<u32, u32>>;

class Converter {
public:
    enum ConvertResult {
        Success,
        FileNotFound,
        InvalidFormat,
        PatchFailed,
    };

    Converter(const std::string &inPath);
    ~Converter();

    ConvertResult Convert(const std::string &outPath);

private:
    std::ifstream _in;
    char *_buffer;
    u32 _bufferSize;

    u32 plgLdrDataOffset = 0;
    u32 plgLdrFetchEventOffset = 0;
    u32 plgLdrReplyOffset = 0;
    u32 plgLdrInitOffset = 0;
    u32 initExOffset = 0;
    u32 convertHeaderOffset = 0;

    void ConvertHeader(_3gx_Header &newHeader, const _3gx_Header_Old &oldHeader);
    void Write(std::fstream &out, _3gx_Header &header, const _3gx_Header_Old &oldHeader);

    int Search(const PatternMask &pattern);
    bool Patch(const _3gx_Header &header);
};