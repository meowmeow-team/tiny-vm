#pragma once

#include <string>
#include <unordered_map>

#include "../../lexer/include/token.hpp"

class SymbolTable {
   public:
    enum Type { INT, CHAR, VOID };
    enum Group { VARIABLE, FUNCTION };

    struct Meta {
        Type  type;
        Group group;

        bool operator==(const Meta& other) const noexcept {
            return type == other.type && group == other.group;
        }
    };

   public:
    SymbolTable(bool kore_wa_global_desu_ka);

    void push(const std::string& name, const Meta& meta);

    bool contains(const std::string& name) const noexcept;
    bool contains(const std::string& name, const Meta& meta) const noexcept;
    bool contains(const std::string& name, Type type) const noexcept;
    bool contains(const std::string& name, Group group) const noexcept;

    const Meta& get(const std::string& name) const;

    bool is_global() const noexcept;

    static Type make_type(const Token& type);
    static Meta make_meta(const Token& type, Group group);

   private:
    bool global_desu;

    std::unordered_map<std::string, Meta> src;
};
