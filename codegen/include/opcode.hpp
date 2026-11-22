#pragma once

#include <cstdint>

enum class Opcode : uint8_t {
    // math
    ADD,
    SUB,
    MUL,
    DIV,
    // memory
    MOV,
    LOAD,        // hueta 1
    LOAD_CONST,  // hueta 2
    // transitions
    JMP,
    // other
    CALL,
    RET,
    PRINT,
    HALT
};
