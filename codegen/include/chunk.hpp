#pragma once

#include <any>
#include <utility>
#include <vector>

#include "byte_code.hpp"

struct Chunk {
    ByteCode byte_code;

    std::vector<std::pair<std::string, std::any>> pool;
};
