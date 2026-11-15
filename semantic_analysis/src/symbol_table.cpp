#include "../include/symbol_table.hpp"

#include <iostream>
#include <string_view>

#include "../../core/exception.hpp"

SymbolTable::SymbolTable(bool kore_wa_global_desu_ka)
    : src(), global_desu(kore_wa_global_desu_ka) {}

void SymbolTable::push(const std::string& name, const Meta& meta) {
    if (name.empty()) throw vm::symbol_table_error("name is empty");

    if (contains(name))
        throw vm::symbol_table_error(
            "failed to push: already contains that shit");

    src.emplace(name, meta);
}

bool SymbolTable::contains(const std::string& name) const noexcept {
    auto it = src.find(name);
    return it != src.end();
}

bool SymbolTable::contains(const std::string& name,
                           const Meta&        meta) const noexcept {
    auto it = src.find(name);
    if (it == src.end()) return false;

    return it->second == meta;
}

bool SymbolTable::contains(const std::string& name, Type type) const noexcept {
    auto it = src.find(name);
    if (it == src.end()) return false;

    return it->second.type == type;
}

bool SymbolTable::contains(const std::string& name,
                           Group              group) const noexcept {
    auto it = src.find(name);
    if (it == src.end()) return false;

    return it->second.group == group;
}

const SymbolTable::Meta& SymbolTable::get(const std::string& name) const {
    auto it = src.find(name);
    if (it == src.end()) throw vm::symbol_table_error("failed to get meta");

    return it->second;
}

bool SymbolTable::is_global() const noexcept { return global_desu; }

SymbolTable::Type SymbolTable::make_type(const Token& type) {
    static const std::unordered_map<std::string_view, Type> map = {
        {"void", VOID}, {"int", INT}, {"char", CHAR}};

    auto it = map.find(type.get_lexeme());
    if (it == map.end())
        throw vm::symbol_table_error("failed to make type: unknown lexeme");

    return it->second;
}

SymbolTable::Meta SymbolTable::make_meta(const Token& type, Group group) {
    if (type.get_type() != TokenType::KEYWORD &&
        type.get_type() != TokenType::KEYWORD_TYPE)
        throw vm::symbol_table_error("failed to make meta: wrong type");

    return {make_type(type), group};
}
