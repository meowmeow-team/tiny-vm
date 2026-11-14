#pragma once

#include <string>
#include <unordered_map>

#include "../../lexer/include/token.hpp"

class SymbolTable {
   public:
    enum Type { INT, CHAR, STRING, VOID };
    enum Group {
        VARIABLE,
        VARIABLE_GLOBAL,
        FUNCTION,
        FUNCTION_PARAM,
    };

    struct SymbolMeta {
        Type  type;
        Group group;

        bool operator==(const SymbolMeta& other) const noexcept {
            return type == other.type && group == other.group;
        }
    };

   public:
    SymbolTable();

    void push(const std::string& name, const SymbolMeta& meta);
    void erase(const std::string& name, const SymbolMeta& meta);

    const SymbolMeta& get(const std::string& name) const;

    bool contains(const std::string& name) const noexcept;
    bool contains(const std::string& name,
                  const SymbolMeta&  meta) const noexcept;

    static SymbolMeta make_meta(const Token& token, Group group);

   private:
    std::unordered_map<std::string, SymbolMeta> src;
};
