#pragma once

#include <string>
#include <unordered_map>

class SymbolTable {
   public:
    enum Type { INT, CHAR, STRING, VOID };
    enum Group { VARIABLE, FUNCTION };

    struct SymbolMeta {
        Type  type;
        Group group;
    };

   public:
    SymbolTable();

    void              push(const std::string& name, const SymbolMeta& meta);
    const SymbolMeta& get(const std::string& name) const;

    bool contains(const std::string& name) const noexcept;

   private:
    std::unordered_map<std::string, SymbolMeta> src;
};
