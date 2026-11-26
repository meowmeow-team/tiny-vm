#include "../include/byte_code.hpp"

#include <iostream>

#include "../../core/exception.hpp"

ByteCode::ByteCode() {}

void ByteCode::push_func(const std::string& name) {
    funcs_defs.emplace(name, src.size());
}

void ByteCode::push_ins(Opcode opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
    std::cout << "PUSH: " << opcode::opcode_to_str(opcode) << " "
              << static_cast<int>(rd) << " " << static_cast<int>(rs1) << " "
              << static_cast<int>(rs2) << '\n';
    src.push_back({opcode, rd, rs1, rs2});
}

std::optional<Instruction> ByteCode::get_last_ins() const noexcept {
    if (src.empty()) return std::nullopt;
    return src.back();
}

Instruction ByteCode::get_ins(size_t idx) {
    if (idx >= src.size()) throw vm::byte_code_error("failed to get ins");
    return src.at(idx);
}

size_t ByteCode::get_size() const noexcept { return src.size(); }

std::ostream& operator<<(std::ostream& out, const ByteCode& bc) {
    const char* next = "\n";

    size_t line = 0;
    for (auto ins : bc.src) {
        out << line++ << "    " << opcode::opcode_to_str(ins.opcode) << ", "
            << static_cast<int>(ins.rd) << ", " << static_cast<int>(ins.rs1)
            << ", " << static_cast<int>(ins.rs2) << next;
    }

    return out;
}
