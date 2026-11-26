
#pragma once

#include <cstdint>
#include <optional>

#include "../../core/source_manager.hpp"
#include "../../parser/include/ast.hpp"

class ConstantEvaluator {
public:
    explicit ConstantEvaluator(const SourceManager& source);

    std::optional<int64_t> evaluate(const AST::Node* node) const;

private:
    const SourceManager& source;

    int64_t evaluate_operation(
        const std::string& op,
        int64_t left,
        int64_t right
    ) const;
};
