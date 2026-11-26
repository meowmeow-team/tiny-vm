#include <catch2/catch_get_random_seed.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "constant_evaluator.hpp"
#include "parser.hpp"

namespace {

using Integer = std::int64_t;
using Node = std::unique_ptr<AST::Node>;
using Random = std::mt19937_64;

constexpr Integer min_int = std::numeric_limits<Integer>::min();
constexpr Integer max_int = std::numeric_limits<Integer>::max();
constexpr int trials = 256;

Integer draw(Random& rng, Integer low, Integer high) {
    return std::uniform_int_distribution<Integer>{low, high}(rng);
}

struct Expression {
    std::vector<Token> tokens;

    void append(TokenType type, const std::string& text) {
        tokens.emplace_back(type, Position{0, 1, 1}, text);
    }

    Node literal(const std::string& text) {
        auto node = std::make_unique<AST::IntLiteralDecl>();
        node->ref_idx = static_cast<std::uint32_t>(tokens.size());
        append(TokenType::NUMBER, text);
        return node;
    }

    Node literal(Integer value) {
        return literal(std::to_string(value));
    }

    Node variable(const std::string& name) {
        auto node = std::make_unique<AST::RefVarDecl>();
        node->ref_idx = static_cast<std::uint32_t>(tokens.size());
        append(TokenType::IDENTIFIER, name);
        return node;
    }

    Node binary(char op, Node left, Node right) {
        auto node = std::make_unique<AST::BinaryOperation>();
        node->ref_idx = static_cast<std::uint32_t>(tokens.size());
        append(TokenType::OPERATOR, std::string(1, op));
        node->kids.push_back(std::move(left));
        node->kids.push_back(std::move(right));
        return node;
    }

    std::optional<Integer> evaluate(const AST::Node* node) const {
        const SourceManager source(tokens);
        return ConstantEvaluator(source).evaluate(node);
    }
};

std::optional<Integer> evaluate_binary(char op, Integer left, Integer right) {
    Expression expression;
    auto lhs = expression.literal(left);
    auto rhs = expression.literal(right);
    auto root = expression.binary(op, std::move(lhs), std::move(rhs));
    return expression.evaluate(root.get());
}

struct GeneratedTree {
    Node node;
    Integer value;
    std::string text;
};

GeneratedTree generate_tree(Expression& expression, Random& rng, int depth) {
    if (depth == 0 || draw(rng, 0, 3) == 0) {
        const auto value = draw(rng, -32, 32);
        return {expression.literal(value), value, std::to_string(value)};
    }

    constexpr std::array ops{'+', '-', '*', '/'};
    const char op = ops[draw(rng, 0, 3)];
    auto left = generate_tree(expression, rng, depth - 1);
    auto right = generate_tree(expression, rng, depth - 1);
    if (op == '/' && right.value == 0) {
        right = {expression.literal(1), 1, "1"};
    }

    // At depth 3, at most eight leaves of magnitude 32 contribute.
    Integer value = 0;
    switch (op) {
        case '+': value = left.value + right.value; break;
        case '-': value = left.value - right.value; break;
        case '*': value = left.value * right.value; break;
        case '/': value = left.value / right.value; break;
    }

    auto text = "(" + left.text + " " + op + " " + right.text + ")";
    return {expression.binary(op, std::move(left.node), std::move(right.node)),
            value, std::move(text)};
}

}  // namespace

TEST_CASE("Optimizer: randomized signed integer literals", "[optimizer][literal]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);

    for (int trial = 0; trial < trials; ++trial) {
        const auto value = draw(rng, min_int, max_int);
        const auto delta = draw(rng, 1, 10000);
        const auto zeros = std::string(draw(rng, 1, 8), '0');
        CAPTURE(seed, trial, value, delta);

        for (const auto expected : {value, max_int, min_int, Integer{0}}) {
            auto text = std::to_string(expected);
            text.insert(expected < 0 ? 1 : 0, zeros);
            CAPTURE(text, expected);
            Expression expression;
            auto root = expression.literal(text);
            CHECK(expression.evaluate(root.get()) == expected);
        }

        const auto too_large = static_cast<std::uint64_t>(max_int) + delta;
        constexpr auto min_magnitude = static_cast<std::uint64_t>(max_int) + 1;
        for (const auto& text : {std::to_string(too_large),
                                 "-" + std::to_string(min_magnitude + delta)}) {
            CAPTURE(text);
            Expression expression;
            auto root = expression.literal(text);
            CHECK_THROWS_AS(expression.evaluate(root.get()), std::overflow_error);
        }

        const auto digits = std::to_string(delta);
        for (const auto& text : {std::string{}, "+" + digits, " " + digits,
                                 digits + " ", "-" + digits + "x", digits + ".5"}) {
            CAPTURE(text);
            Expression expression;
            auto root = expression.literal(text);
            CHECK_THROWS_AS(expression.evaluate(root.get()), std::invalid_argument);
        }
    }
}

TEST_CASE("Optimizer: randomized nested constant expressions", "[optimizer][arithmetic]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);

    for (int trial = 0; trial < trials; ++trial) {
        Expression expression;
        auto tree = generate_tree(expression, rng, 3);
        CAPTURE(seed, trial, tree.text, tree.value);
        CHECK(expression.evaluate(tree.node.get()) == tree.value);
    }
}

TEST_CASE("Optimizer: randomized addition and subtraction at int64 limits",
          "[optimizer][overflow]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);

    for (int trial = 0; trial < trials; ++trial) {
        const auto delta = draw(rng, 1, 1000000);
        const auto high = max_int - delta;
        const auto low = min_int + delta;
        CAPTURE(seed, trial, delta, low, high);

        CHECK(evaluate_binary('+', high, delta) == max_int);
        CHECK(evaluate_binary('+', low, -delta) == min_int);
        CHECK_THROWS_AS(evaluate_binary('+', high, delta + 1), std::overflow_error);
        CHECK_THROWS_AS(evaluate_binary('+', low, -delta - 1), std::overflow_error);

        CHECK(evaluate_binary('-', high, -delta) == max_int);
        CHECK(evaluate_binary('-', low, delta) == min_int);
        CHECK_THROWS_AS(evaluate_binary('-', high, -delta - 1), std::overflow_error);
        CHECK_THROWS_AS(evaluate_binary('-', low, delta + 1), std::overflow_error);
    }
}

TEST_CASE("Optimizer: randomized multiplication at int64 limits",
          "[optimizer][overflow]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);

    for (int trial = 0; trial < trials; ++trial) {
        const auto power = draw(rng, 1, 62);
        const Integer factor = Integer{1} << power;
        // The negative range has one extra magnitude: 2^63.
        const Integer negative_limit = Integer{1} << (63 - power);
        const Integer positive_limit = negative_limit - 1;
        const auto value = draw(rng, min_int, max_int);
        CAPTURE(seed, trial, power, factor, positive_limit, negative_limit, value);

        CHECK(evaluate_binary('*', factor, positive_limit) == max_int - (factor - 1));
        CHECK(evaluate_binary('*', -factor, -positive_limit) == max_int - (factor - 1));
        CHECK(evaluate_binary('*', factor, -negative_limit) == min_int);
        CHECK(evaluate_binary('*', -factor, negative_limit) == min_int);
        CHECK(evaluate_binary('*', value, 0) == 0);
        CHECK(evaluate_binary('*', 0, value) == 0);

        CHECK_THROWS_AS(evaluate_binary('*', factor, negative_limit), std::overflow_error);
        CHECK_THROWS_AS(evaluate_binary('*', -factor, -negative_limit), std::overflow_error);
        CHECK_THROWS_AS(evaluate_binary('*', factor, -negative_limit - 1), std::overflow_error);
        CHECK_THROWS_AS(evaluate_binary('*', -factor, negative_limit + 1), std::overflow_error);
    }

    CHECK_THROWS_AS(evaluate_binary('*', min_int, -1), std::overflow_error);
    CHECK_THROWS_AS(evaluate_binary('*', -1, min_int), std::overflow_error);
    CHECK(evaluate_binary('*', min_int, 1) == min_int);
    CHECK(evaluate_binary('*', 1, min_int) == min_int);
}

TEST_CASE("Optimizer: randomized signed division", "[optimizer][division]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);

    for (int trial = 0; trial < trials; ++trial) {
        const auto denominator = draw(rng, 2, 1000000);
        const auto quotient = trial % 4 == 0 ? 0 : draw(rng, 1, 1000000);
        const auto remainder = draw(rng, 1, denominator - 1);
        const auto numerator = denominator * quotient + remainder;
        CAPTURE(seed, trial, numerator, denominator, quotient, remainder);

        CHECK(evaluate_binary('/', numerator, denominator) == quotient);
        CHECK(evaluate_binary('/', -numerator, denominator) == -quotient);
        CHECK(evaluate_binary('/', numerator, -denominator) == -quotient);
        CHECK(evaluate_binary('/', -numerator, -denominator) == quotient);
        CHECK_THROWS_AS(evaluate_binary('/', numerator, 0), std::domain_error);
    }

    CHECK_THROWS_AS(evaluate_binary('/', min_int, -1), std::overflow_error);
    CHECK(evaluate_binary('/', min_int, 1) == min_int);
    CHECK_THROWS_AS(evaluate_binary('/', 0, 0), std::domain_error);
}

TEST_CASE("Optimizer: randomized nonconstant expressions and invalid nodes",
          "[optimizer][ast]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);
    constexpr std::array ops{'+', '-', '*', '/'};
    constexpr std::array unsupported_ops{'%', '<', '>', '^', '&', '|'};

    for (int trial = 0; trial < trials; ++trial) {
        const auto value = draw(rng, 1, 1000);
        const auto op = ops[draw(rng, 0, 3)];
        const auto unsupported = unsupported_ops[draw(rng, 0, unsupported_ops.size() - 1)];
        const auto extra_children = draw(rng, 1, 5);
        CAPTURE(seed, trial, value, op, unsupported, extra_children);

        Expression expression;
        auto unknown_left = expression.binary(
            op, expression.variable("x"), expression.literal(value));
        CHECK(expression.evaluate(unknown_left.get()) == std::nullopt);

        auto unknown_right = expression.binary(
            op, expression.literal(value), expression.variable("y"));
        CHECK(expression.evaluate(unknown_right.get()) == std::nullopt);

        auto nested = expression.binary('+', std::move(unknown_left),
                                        std::move(unknown_right));
        CHECK(expression.evaluate(nested.get()) == std::nullopt);

        CHECK_THROWS_AS(evaluate_binary(unsupported, value, 1), std::invalid_argument);

        auto missing_left = expression.binary(op, nullptr, expression.literal(value));
        auto missing_right = expression.binary(op, expression.literal(value), nullptr);
        CHECK_THROWS_AS(expression.evaluate(missing_left.get()), std::invalid_argument);
        CHECK_THROWS_AS(expression.evaluate(missing_right.get()), std::invalid_argument);

        auto wrong_arity = expression.binary(op, expression.literal(value),
                                            expression.literal(1));
        for (Integer i = 0; i < extra_children; ++i) {
            wrong_arity->kids.push_back(expression.literal(i));
        }
        CHECK_THROWS_AS(expression.evaluate(wrong_arity.get()), std::invalid_argument);
        wrong_arity->kids.resize(1);
        CHECK_THROWS_AS(expression.evaluate(wrong_arity.get()), std::invalid_argument);
        wrong_arity->kids.clear();
        CHECK_THROWS_AS(expression.evaluate(wrong_arity.get()), std::invalid_argument);
    }

    const SourceManager source({Token{TokenType::NUMBER, {0, 1, 1}, "0"}});
    CHECK_THROWS_AS(ConstantEvaluator(source).evaluate(nullptr), std::invalid_argument);
}

TEST_CASE("Optimizer: evaluates randomized expressions parsed from tokens",
          "[optimizer][integration]") {
    const auto seed = Catch::getSeed();
    Random rng(seed);

    for (int trial = 0; trial < trials; ++trial) {
        const auto a = draw(rng, 1, 100);
        const auto b = draw(rng, 1, 100);
        const auto c = draw(rng, 1, 100);
        const bool multiply = draw(rng, 0, 1) == 1;
        const bool add = draw(rng, 0, 1) == 1;
        const bool high_first = trial % 2 == 0;
        const std::string low_op = add ? "+" : "-";
        const std::string high_op = multiply ? "*" : "/";
        const auto& first_op = high_first ? high_op : low_op;
        const auto& second_op = high_first ? low_op : high_op;
        const auto first = std::to_string(a);
        const auto second = std::to_string(b);
        const auto third = std::to_string(c);
        const auto text = first + " " + first_op + " " + second + " " + second_op
                          + " " + third;
        const auto lhs = high_first ? (multiply ? a * b : a / b) : a;
        const auto rhs = high_first ? c : (multiply ? b * c : b / c);
        const auto expected = add ? lhs + rhs : lhs - rhs;
        CAPTURE(seed, trial, text, expected);

        SourceManager source({
            {TokenType::KEYWORD_TYPE, {0, 1, 1}, "void"},
            {TokenType::IDENTIFIER, {0, 1, 1}, "f"},
            {TokenType::SYMBOL, {0, 1, 1}, "("},
            {TokenType::SYMBOL, {0, 1, 1}, ")"},
            {TokenType::SYMBOL, {0, 1, 1}, "{"},
            {TokenType::KEYWORD_TYPE, {0, 1, 1}, "int"},
            {TokenType::IDENTIFIER, {0, 1, 1}, "result"},
            {TokenType::SYMBOL_ASSIGN, {0, 1, 1}, "="},
            {TokenType::NUMBER, {0, 1, 1}, first},
            {TokenType::OPERATOR, {0, 1, 1}, first_op},
            {TokenType::NUMBER, {0, 1, 1}, second},
            {TokenType::OPERATOR, {0, 1, 1}, second_op},
            {TokenType::NUMBER, {0, 1, 1}, third},
            {TokenType::SEMICOLON, {0, 1, 1}, ";"},
            {TokenType::SYMBOL, {0, 1, 1}, "}"},
        });
        Logger logger;
        auto unit = Parser(source, logger).parse_unit("optimizer_test");
        REQUIRE(unit.kids.size() == 1);
        const auto& function = unit.kids.front();
        REQUIRE(function->kids.size() == 1);
        const auto& body = function->kids.front();
        REQUIRE(body->kids.size() == 1);
        const auto& assignment = body->kids.front();
        REQUIRE(assignment->kids.size() == 2);
        CHECK(ConstantEvaluator(source).evaluate(assignment->kids[1].get()) == expected);
    }
}
