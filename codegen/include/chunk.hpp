#pragma once

#include <any>
#include <utility>
#include <vector>

#include "byte_code.hpp"

class Chunk {
   public:
    Chunk();

    void push_var(const std::string& name, std::any value);

    std::any get_pool(size_t idx) const;
    size_t   find_idx(const std::string& name) const;

    ByteCode byte_code;

   private:
    std::vector<std::any>                   pool;
    std::unordered_map<std::string, size_t> name_pool;
};
