
#include "../include/constant_evaluator.hpp"

#include <charconv>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

ConstantEvaluator::ConstantEvaluator(const SourceManager& source)
    : source(source) {}

std::optional<int64_t> ConstantEvaluator::evaluate(
    const AST::Node* node) const {
    if (node == nullptr) {
        throw std::invalid_argument("AST node is null");
    }

    if (node->type == AST::INT_LITERAL_DECL) {
        const std::string& text = source.get(node->ref_idx).get_lexeme();

        int64_t value = 0;

        auto [ptr, ec] =
            std::from_chars(text.data(), text.data() + text.size(), value);

        if (ec == std::errc::result_out_of_range) {
            throw std::overflow_error("Integer literal is out of range: " +
                                      text);
        }

        if (ec != std::errc{} || ptr != text.data() + text.size()) {
            throw std::invalid_argument("Invalid integer literal: " + text);
        }

        return value;
    }

    if (node->type == AST::BINARY_OPERATION) {
        if (node->kids.size() != 2 || !node->kids[0] || !node->kids[1]) {
            throw std::invalid_argument(
                "Binary operation must have two operands");
        }

        auto left = evaluate(node->kids[0].get());
        auto right = evaluate(node->kids[1].get());

        if (!left || !right) {
            return std::nullopt;
        }

        const std::string& op = source.get(node->ref_idx).get_lexeme();

        return evaluate_operation(op, *left, *right);
    }

    return std::nullopt;
}

int64_t ConstantEvaluator::evaluate_operation(const std::string& op,
                                              int64_t            left,
                                              int64_t            right) const {
    constexpr int64_t MAX = std::numeric_limits<int64_t>::max();
    constexpr int64_t MIN = std::numeric_limits<int64_t>::min();

    if (op == "+") {
        if ((right > 0 && left > MAX - right) ||
            (right < 0 && left < MIN - right)) {
            throw std::overflow_error("Addition overflow");
        }

        return left + right;
    }

    if (op == "-") {
        if ((right < 0 && left > MAX + right) ||
            (right > 0 && left < MIN + right)) {
            throw std::overflow_error("Subtraction overflow");
        }

        return left - right;
    }

    if (op == "*") {
        if (left > 0) {
            if ((right > 0 && left > MAX / right) ||
                (right < 0 && right < MIN / left)) {
                throw std::overflow_error("Multiplication overflow");
            }
        } else if (left < 0) {
            if ((right > 0 && left < MIN / right) ||
                (right < 0 && left < MAX / right)) {
                throw std::overflow_error("Multiplication overflow");
            }
        }

        return left * right;
    }

    if (op == "/") {
        if (right == 0) {
            throw std::domain_error("Division by zero");
        }

        if (left == MIN && right == -1) {
            throw std::overflow_error("Division overflow");
        }

        return left / right;
    }

    throw std::invalid_argument("Unsupported binary operator: " + op);
}
