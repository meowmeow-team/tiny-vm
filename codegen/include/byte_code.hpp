#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "instruction.hpp"

class ByteCode {
   public:
    ByteCode();

   private:
    std::vector<Instruction> src;

    std::unordered_map<std::string, uint32_t>
        funcs_defs;  // first arg - name of func
                     // second arg - index in src vector
};
