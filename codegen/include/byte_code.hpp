#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "instruction.hpp"

class ByteCode {
   public:
    ByteCode();

    void push_func(const std::string& name);
    void push_ins(Opcode opcode, uint8_t rd = 0, uint8_t rs1 = 0,
                  uint8_t rs2 = 0);

    std::optional<Instruction> get_last_ins() const noexcept;

    friend std::ostream& operator<<(std::ostream& out, const ByteCode& bc);

   private:
    std::vector<Instruction> src;  // all program instructions

    std::unordered_map<std::string, uint32_t>
        funcs_defs;  // first arg - name of func
                     // second arg - index in src vector
};
