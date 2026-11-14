#include "../include/symbol_table.hpp"

#include <iostream>

#include "../../core/exception.hpp"

SymbolTable::SymbolTable() : src() {}

void SymbolTable::push(const std::string& name, const SymbolMeta& meta) {
    if (name.empty())
        throw vm::symbol_table_error("failed to push: name is empty");

    if (contains(name))
        throw vm::symbol_table_error("failed to push: already has that name");
    src.emplace(name, meta);
}

void SymbolTable::erase(const std::string& name, const SymbolMeta& meta) {
    if (name.empty())
        throw vm::symbol_table_error("failed to erase: name is empty");

    auto it = src.find(name);
    if (it == src.end())
        throw vm::symbol_table_error(
            "failed to erase: there is no such a name");

    src.erase(it);
}

const SymbolTable::SymbolMeta& SymbolTable::get(const std::string& name) const {
    if (name.empty())
        throw vm::symbol_table_error("failed to get: name is empty");

    auto it = src.find(name);
    if (it == src.end())
        throw vm::symbol_table_error("failed to get: there is no such name");
    return it->second;
}

bool SymbolTable::contains(const std::string& name) const noexcept {
    auto it = src.find(name);
    return it != src.end();
}

bool SymbolTable::contains(const std::string& name,
                           const SymbolMeta&  meta) const noexcept {
    auto it = src.find(name);
    if (it == src.end()) return false;

    return it->second == meta;
}

SymbolTable::SymbolMeta SymbolTable::make_meta(const Token& token,
                                               Group        group) {
    if (token.get_type() != TokenType::KEYWORD ||
        token.get_type() != TokenType::KEYWORD_TYPE)
        throw vm::symbol_table_error("wrong token type");

    static const std::unordered_map<std::string, Type> map = {
        {"void", VOID}, {"int", INT}, {"char", CHAR}};

    auto it = map.find(token.get_lexeme());
    if (it == map.end())
        throw vm::symbol_table_error("unknown lexeme for meta");

    return {it->second, group};
}
