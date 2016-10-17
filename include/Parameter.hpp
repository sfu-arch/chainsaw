#ifndef __PARAMETER__
#define __PARAMETER__

#include "common.hpp"

class Parameter{
    public:
        //Members
        std::map<std::string, uint32_t> param_set;

        //Functions
        void readParam(std::string file_name);
};
#endif
