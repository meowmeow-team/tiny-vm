#pragma once

#include <cstdint>
#include <iostream>
#include <string>

class VarMeta {
   public:
    enum Loc { GLOBAL, LOCAL };

    enum Type { INT, CHAR };

   public:
    VarMeta();
    VarMeta(Loc loc, Type type, uint8_t idx);
    VarMeta(Loc loc, const std::string& name, uint8_t idx);

    Loc     get_loc() const noexcept;
    Type    get_type() const noexcept;
    uint8_t get_idx() const noexcept;

   public:
    static Type what_type(const std::string& lexeme);
    static Type what_type(const std::type_info& t);

   private:
    Loc     loc;
    Type    type;
    uint8_t idx;
};
