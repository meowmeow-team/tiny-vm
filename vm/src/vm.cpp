#include "../include/vm.hpp"

#include <iostream>

#include "../../core/exception.hpp"

VM::VM() : regs(), cur_ins(0), out(nullptr), chunk() {}

VM::VM(Chunk chunk)
    : regs(), cur_ins(0), out(nullptr), chunk(std::move(chunk)) {}

void VM::load(Chunk chunk) { this->chunk = std::move(chunk); }

void VM::enable_logging(std::ostream* out) {
    if (!out) throw vm::vms_error("failed to enable logging: out is nullptr");
    this->out = out;
}

void VM::disable_logging() noexcept {
    if (out) out = nullptr;
}

void VM::run() {
    auto& bc = chunk.byte_code;
    while (cur_ins < bc.get_size()) {
        const auto ins = bc.get_ins(cur_ins);

        switch (ins.opcode) {
            case Opcode::LOAD_CONST:
                regs[ins.rd] = ins.rs1;
                log("executing LOAD_CONST: ", static_cast<int>(ins.rd), " <- ",
                    static_cast<int>(ins.rs1));
                break;
            case Opcode::ADD:
                regs[ins.rd] = regs[ins.rs1] + regs[ins.rs2];
                log("executing ADD: ", static_cast<int>(ins.rd), " <- ",
                    static_cast<int>(ins.rs1), " + ",
                    static_cast<int>(ins.rs2));
                break;
            case Opcode::SUB:
                regs[ins.rd] = regs[ins.rs1] - regs[ins.rs2];
                log("executing SUB: ", static_cast<int>(ins.rd), " <- ",
                    static_cast<int>(ins.rs1), " - ",
                    static_cast<int>(ins.rs2));
                break;
            case Opcode::MUL:
                regs[ins.rd] = regs[ins.rs1] * regs[ins.rs2];
                log("executing MUL: ", static_cast<int>(ins.rd), " <- ",
                    static_cast<int>(ins.rs1), " * ",
                    static_cast<int>(ins.rs2));
                break;
            case Opcode::DIV:
                if (regs[ins.rs2] == 0) throw vm::vms_error("division by zero");
                regs[ins.rd] = regs[ins.rs1] / regs[ins.rs2];
                log("executing DIV: ", static_cast<int>(ins.rd), " <- ",
                    static_cast<int>(ins.rs1), " / ",
                    static_cast<int>(ins.rs2));
                break;
            case Opcode::HALT:
                return;
        }

        cur_ins++;
    }
}
