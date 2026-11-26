#pragma once

#include <any>
#include <vector>

#include "byte_code.hpp"

class Chunk {
   public:
    Chunk();

    void push_gl_var(const std::string& name, std::any value);

    std::any get_gl(size_t idx) const;
    size_t   find_gl_idx(const std::string& name) const;

    ByteCode byte_code;

    friend std::ostream& operator<<(std::ostream& out, const Chunk& chunk);

   private:
    std::vector<std::any>                   gl_pool;
    std::unordered_map<std::string, size_t> gl_name_pool;
};
