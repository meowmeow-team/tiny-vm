#pragma once

#include "opcode.hpp"

struct Instruction {
    Opcode opcode;

    uint8_t rd;   // destination register
    uint8_t rs1;  // src register 1
    uint8_t rs2;  // src register 2
};
