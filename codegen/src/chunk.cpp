#include "../include/chunk.hpp"

#include "../../core/exception.hpp"

Chunk::Chunk() : byte_code(), pool(), name_pool() {}

void Chunk::push_var(const std::string& name, std::any value) {
    size_t idx = pool.size();
    pool.push_back(value);

    name_pool.emplace(name, idx);
}

std::any Chunk::get_pool(size_t idx) const { return pool.at(idx); }

size_t Chunk::find_idx(const std::string& name) const {
    auto it = name_pool.find(name);
    if (it == name_pool.end())
        throw vm::codegen_error("failed to find idx by name in the chunk pool");

    return it->second;
}
