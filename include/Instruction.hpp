#ifndef __INSTRUCTION__
#define __INSTRUCTION__

#include "common.hpp"

class Instruction{
    public:
        //Members
        //std::set<std::string> instruction_llvm;
        std::map<std::string, uint32_t> instruction_llvm;

        //Functions
        void readInst(std::string file_name);
};
#endif
