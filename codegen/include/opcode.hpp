#pragma once

#include <cstdint>
#include <istream>
#include <string>

enum class Opcode : uint8_t {
    // math
    ADD,
    SUB,
    MUL,
    DIV,
    // memory
    MOV,         // hueta 1 (copy from one reg to another)
    LOAD,        // hueta 2 (idk why, maybe for future heap objects)
    LOAD_CONST,  // hueta 3 (i guess for const variables like literals)
    // transitions
    JMP,
    // other
    CALL,
    RET,
    PRINT,
    HALT
};

namespace opcode {

std::string opcode_to_str(Opcode opcode);

}
