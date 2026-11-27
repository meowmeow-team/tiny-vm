#include "../include/vm.hpp"

#include <iostream>

#include "../../core/exception.hpp"

VM::VM()
    : regs(),
      cur_ins(0),
      out(nullptr),
      chunk(),
      cur_func(Func::NONE),
      func_nesting(0),
      saved_ins{},
      ret_value() {}

VM::VM(Chunk chunk)
    : regs(),
      cur_ins(0),
      out(nullptr),
      chunk(std::move(chunk)),
      cur_func(Func::NONE),
      func_nesting(0),
      saved_ins{},
      ret_value() {}

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
    bool  has_func = false;

    crawling();
    execute();
    out_exit_code();
}

void VM::crawling() {
    auto& bc = chunk.byte_code;
    while (cur_ins < bc.get_size()) {
        auto opt = bc.find_func(cur_ins);
        if (opt) {
            if (*opt == ENTRY_POINT) {
                cur_func = Func::MAIN;
                func_nesting = 1;
                return;
            }
        }

        cur_ins++;
    }
}

void VM::execute() {
    auto& bc = chunk.byte_code;
    while (cur_ins < bc.get_size()) {
        const auto ins = bc.get_ins(cur_ins);
        switch (ins.opcode) {
            case Opcode::MOV:
                regs[ins.rd] = regs[ins.rs1];
                log("executing MOV: ", static_cast<int>(ins.rd), " <- ",
                    static_cast<int>(ins.rs1));
                break;
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
            case Opcode::RET:
                if (cur_func == Func::MAIN) {
                    ret_value = regs[ins.rd];
                    return;
                }
                func_nesting--;
                break;
            case Opcode::HALT:
                return;
        }

        cur_ins++;
    }
}

void VM::out_exit_code() {
    if (!ret_value.has_value()) throw vm::vms_error("failed to get exit code");

    std::string val;
    if (const auto* s = std::any_cast<int>(&ret_value)) {
        val = std::to_string(*s);
    } else if (const auto* s = std::any_cast<char>(&ret_value)) {
        val = std::to_string(*s);
    }

    log("program ended with exit code: ", val);
}
