#include "../include/byte_code.hpp"

ByteCode::ByteCode() {}

void ByteCode::push_func(const std::string& name) {
    funcs_defs.emplace(name, src.size());
}

void ByteCode::push_ins(Opcode opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    src.push_back({opcode, rd, rs1, rs2});
}
