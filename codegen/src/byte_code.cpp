#include "../include/byte_code.hpp"

#include <iostream>

#include "../../core/exception.hpp"

ByteCode::ByteCode() {}

void ByteCode::push_func(const std::string& name) {
    auto     opt = get_last_ins();
    uint32_t idx = src.size();
    if (opt) {
        if (opt->opcode == Opcode::RET) idx++;
    }
    funcs_defs.emplace(name, idx);
    funcs_idx.emplace(idx, name);
}

void ByteCode::push_ins(Opcode opcode, uint8_t rd, uint8_t rs1, uint8_t rs2) {
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
        auto it = bc.funcs_idx.find(line);
        if (it != bc.funcs_idx.end()) {
            out << line++ << " * " << " func: " << it->second << next;
        }

        out << line++ << "    " << opcode::opcode_to_str(ins.opcode) << ", "
            << static_cast<int>(ins.rd) << ", " << static_cast<int>(ins.rs1)
            << ", " << static_cast<int>(ins.rs2) << next;
    }

    return out;
}
