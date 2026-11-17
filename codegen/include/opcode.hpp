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
    LOAD,
    STORE,
    STORE_GLOBAL,
    LOAD_CONST,
    // transitions
    JMP,
    // other
    CALL,
    RET,
    PRINT,
    HALT
};
