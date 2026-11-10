#include "source_manager.hpp"

#include <iostream>

SourceManager::SourceManager(std::vector<Token> tokens) : tokens() {
    if (tokens.size() == 0)
        throw std::invalid_argument("tokens vector is empty");
    this->tokens = std::move(tokens);
}

const std::vector<Token>& SourceManager::get_src() const noexcept {
    return tokens;
}

const Token& SourceManager::get(uint32_t idx) const { return tokens.at(idx); }

const uint32_t SourceManager::size() const noexcept { return tokens.size(); }
