#include "../include/opcode.hpp"

#include <unordered_map>

#include "../../core/exception.hpp"

static std::string opcode_to_str(Opcode opcode) {
    static std::unordered_map<Opcode, std::string> map = {
        {Opcode::ADD, "ADD"},
        {Opcode::SUB, "SUB"},
        {Opcode::MUL, "MUL"},
        {Opcode::DIV, "DIV"},
        {Opcode::MOV, "MOV"},
        {Opcode::LOAD, "LOAD"},
        {Opcode::LOAD_CONST, "LOAD_CONST"},
        {Opcode::JMP, "JMP"},
        {Opcode::CALL, "CALL"},
        {Opcode::RET, "RET"},
        {Opcode::PRINT, "PRINT"},
        {Opcode::HALT, "HALT"}};

    auto it = map.find(opcode);
    if (it == map.end()) throw vm::codegen_error("failed to find opcode str");

    return it->second;
}
