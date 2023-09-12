#include "Converter.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <input file> <output file>" << std::endl;
        return -1;
    }

    Converter converter(argv[1]);

    int res = converter.Convert(argv[2]);
    
    if(res != Converter::Success)
    {
        if(res == Converter::FileNotFound)
        {
            std::cout << "File not found." << std::endl;
        }
        else if(res == Converter::InvalidFormat)
        {
            std::cout << "Invalid format." << std::endl;
        }
        else if(res == Converter::PatchFailed)
        {
            std::cout << "Patch failed." << std::endl;
        }
        return -1;
    }

    std::cout << "Convert succeeded." << std::endl;

    return 0;
}