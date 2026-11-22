#include "../include/var_meta.hpp"

#include <unordered_map>

#include "../../core/exception.hpp"

VarMeta::VarMeta() : loc(), type(), idx() {}

VarMeta::VarMeta(VarMeta::Loc loc, VarMeta::Type type, uint8_t idx)
    : loc(loc), type(type), idx(idx) {}

VarMeta::Loc VarMeta::get_loc() const noexcept { return loc; }

VarMeta::Type VarMeta::get_type() const noexcept { return type; }

uint8_t VarMeta::get_idx() const noexcept { return idx; }

VarMeta::Type VarMeta::what_type(const std::string& lexeme) {
    static std::unordered_map<std::string, Type> map = {{"int", INT},
                                                        {"char", CHAR}};

    auto it = map.find(lexeme);
    if (it == map.end())
        throw vm::codegen_error("failed to find var meta type");

    return it->second;
}
