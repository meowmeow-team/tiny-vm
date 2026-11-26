#pragma once

#include "opcode.hpp"

struct Instruction {
    Opcode opcode;

    uint8_t rd = 0;   // destination register
    uint8_t rs1 = 0;  // src register 1
    uint8_t rs2 = 0;  // src register 2
};
