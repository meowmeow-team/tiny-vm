#include "../include/symbol_table.hpp"

#include <iostream>

SymbolTable::SymbolTable() : src() {}

// TODO make own exception for this

void SymbolTable::push(const std::string& name, const SymbolMeta& meta) {
    if (name.empty()) return;

    if (contains(name)) throw std::runtime_error("");  // TODO
    src.emplace(name, meta);
}

const SymbolTable::SymbolMeta& SymbolTable::get(const std::string& name) const {
    if (name.empty()) throw std::runtime_error("");  // TODO

    auto it = src.find(name);
    if (it == src.end()) throw std::runtime_error("");  // TODO

    return it->second;
}

bool SymbolTable::contains(const std::string& name) const noexcept {
    auto it = src.find(name);
    return it != src.end();
}
