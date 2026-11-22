#include "../include/chunk.hpp"

#include "../../core/exception.hpp"

Chunk::Chunk() : byte_code(), gl_pool(), gl_name_pool() {}

void Chunk::push_gl_var(const std::string& name, std::any value) {
    size_t idx = gl_pool.size();
    gl_pool.push_back(value);

    gl_name_pool.emplace(name, idx);
}

std::any Chunk::get_gl(size_t idx) const { return gl_pool.at(idx); }

size_t Chunk::find_gl_idx(const std::string& name) const {
    auto it = gl_name_pool.find(name);
    if (it == gl_name_pool.end())
        throw vm::codegen_error("failed to find idx by name in the chunk pool");

    return it->second;
}
