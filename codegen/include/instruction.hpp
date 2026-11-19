#pragma once

#include "opcode.hpp"

struct Instruction {
    Opcode opcode;

    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
};
