// tiny-vm 0.4.5: reproducible property-based tests for control flow.
// Default seed is stable. To reproduce another generation, set
// TINY_VM_TEST_SEED=<unsigned decimal integer> before running Catch2.
// These tests exercise currently implemented features only.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>

#include <array>
#include <cstddef>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "exception.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "source_manager.hpp"
#include "type_holder.hpp"

namespace {

constexpr uint64_t DEFAULT_SEED = 20261115ULL;

uint64_t test_seed() {
    const char* text = std::getenv("TINY_VM_TEST_SEED");
    if (text == nullptr || *text == '\0') return DEFAULT_SEED;
    const std::string value(text);
    size_t used = 0;
    const auto parsed = std::stoull(value, &used, 10);
    if (used != value.size()) {
        throw std::invalid_argument("TINY_VM_TEST_SEED must be an unsigned integer");
    }
    return static_cast<uint64_t>(parsed);
}

std::mt19937_64 generator(uint64_t salt) {
    return std::mt19937_64(test_seed() ^ salt);
}

int random_int(std::mt19937_64& rng, int lo, int hi) {
    return std::uniform_int_distribution<int>(lo, hi)(rng);
}

template <typename T, size_t N>
const T& pick(std::mt19937_64& rng, const std::array<T, N>& options) {
    return options[static_cast<size_t>(random_int(rng, 0, static_cast<int>(N - 1)))];
}

const std::array<std::string, 4> comparisons = {"<", ">", "==", "!="};
const std::array<std::string, 2> control_types = {"if", "while"};

std::string identifier(std::mt19937_64& rng) {
    return "v" + std::to_string(random_int(rng, 0, 1000000000));
}

struct Parsed {
    std::vector<Token> tokens;
    AST::UnitDecl ast;
};

Parsed parse_tokens_randomized(std::vector<Token> tokens) {
    Logger logger;
    SourceManager source(tokens);
    Parser parser(source, logger);
    auto ast = parser.parse_unit("randomized_test");
    return {std::move(tokens), std::move(ast)};
}

// Deliberately split at whitespace: this tests Parser separately from Lexer.
Parsed parse_words_randomized(const std::string& text) {
    std::istringstream source(text);
    std::string word;
    uint32_t offset = 0;
    std::vector<Token> tokens;
    while (source >> word) {
        const TokenType type = TypeHolder::find(word);
        if (type == TokenType::UNKNOWN) {
            throw std::invalid_argument("invalid test token: " + word);
        }
        tokens.emplace_back(type, Position{offset, 1, offset + 1}, word);
        offset += static_cast<uint32_t>(word.size() + 1);
    }
    return parse_tokens_randomized(std::move(tokens));
}

const AST::Node& body_of(const Parsed& parsed) {
    REQUIRE(parsed.ast.kids.size() == 1);
    const auto& func = *parsed.ast.kids.front();
    REQUIRE(func.type == AST::FUNC_DECL);
    REQUIRE(func.kids.size() == 1);
    REQUIRE(func.kids[0]->type == AST::COMPOUND_STMT);
    return *func.kids[0];
}

std::string lexeme(const Parsed& parsed, const AST::Node& node) {
    return parsed.tokens.at(node.ref_idx).get_lexeme();
}

void expect_comparison(const Parsed& parsed, const AST::Node& node,
                       const std::string& name, const std::string& op,
                       const std::string& literal) {
    REQUIRE(node.type == AST::BINARY_OPERATION);
    REQUIRE(node.kids.size() == 2);
    CHECK(lexeme(parsed, node) == op);
    REQUIRE(node.kids[0]->type == AST::REF_VAR_DECL);
    CHECK(lexeme(parsed, *node.kids[0]) == name);
    REQUIRE(node.kids[1]->type == AST::INT_LITERAL_DECL);
    CHECK(lexeme(parsed, *node.kids[1]) == literal);
}

void expect_literal_assignment(const Parsed& parsed, const AST::Node& node,
                               const std::string& name,
                               const std::string& literal) {
    REQUIRE(node.type == AST::ASSIGN_STMT);
    REQUIRE(node.kids.size() == 2);
    REQUIRE((node.kids[0]->type == AST::REF_VAR_DECL ||
             node.kids[0]->type == AST::VARIABLE_DECL));
    CHECK(lexeme(parsed, *node.kids[0]) == name);
    REQUIRE(node.kids[1]->type == AST::INT_LITERAL_DECL);
    CHECK(lexeme(parsed, *node.kids[1]) == literal);
}

struct TempInput {
    std::filesystem::path path;

    TempInput(size_t index, const std::string& text)
        : path(std::filesystem::temp_directory_path() /
               ("tiny_vm_randomized_" + std::to_string(index) + "_" +
                std::to_string(
                    std::chrono::steady_clock::now().time_since_epoch().count()) +
                ".txt")) {
        std::ofstream out(path);
        if (!out) throw std::runtime_error("cannot create test source file");
        out << text;
    }

    ~TempInput() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

}  // namespace

TEST_CASE("Parser randomized: if AST and the following statement",
          "[parser][randomized][if]") {
    constexpr int iterations = 300;
    const auto seed = test_seed();
    auto rng = generator(0x11);

    for (int i = 0; i < iterations; ++i) {
        const auto name = identifier(rng);
        const auto op = pick(rng, comparisons);
        const auto lhs_init = std::to_string(random_int(rng, 0, 100000));
        const auto bound = std::to_string(random_int(rng, 0, 100000));
        const auto inside = std::to_string(random_int(rng, 0, 100000));
        const auto after = std::to_string(random_int(rng, 0, 100000));
        const auto source = "void f ( ) { int " + name + " = " + lhs_init +
            " ; if ( " + name + " " + op + " " + bound + " ) { " +
            name + " = " + inside + " ; } " + name + " = " + after + " ; }";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);

        const auto result = parse_words_randomized(source);
        const auto& root = body_of(result);
        REQUIRE(root.kids.size() == 3);
        REQUIRE(root.kids[1]->type == AST::IF_STMT);
        const auto& branch = *root.kids[1];
        REQUIRE(branch.kids.size() == 2);
        expect_comparison(result, *branch.kids[0], name, op, bound);
        REQUIRE(branch.kids[1]->type == AST::COMPOUND_STMT);
        REQUIRE(branch.kids[1]->kids.size() == 1);
        expect_literal_assignment(result, *branch.kids[1]->kids[0], name, inside);
        expect_literal_assignment(result, *root.kids[2], name, after);
    }
}

TEST_CASE("Parser randomized: while AST and assignment in loop body",
          "[parser][randomized][while]") {
    constexpr int iterations = 300;
    const auto seed = test_seed();
    auto rng = generator(0x22);

    for (int i = 0; i < iterations; ++i) {
        const auto name = identifier(rng);
        const auto op = pick(rng, comparisons);
        const auto start = std::to_string(random_int(rng, 0, 100000));
        const auto bound = std::to_string(random_int(rng, 0, 100000));
        const auto replacement = std::to_string(random_int(rng, 0, 100000));
        const auto following = std::to_string(random_int(rng, 0, 100000));
        const auto source = "void f ( ) { int " + name + " = " + start +
            " ; while ( " + name + " " + op + " " + bound + " ) { " +
            name + " = " + replacement + " ; } " + name + " = " +
            following + " ; }";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);

        const auto result = parse_words_randomized(source);
        const auto& root = body_of(result);
        REQUIRE(root.kids.size() == 3);
        REQUIRE(root.kids[1]->type == AST::WHILE_STMT);
        const auto& loop = *root.kids[1];
        REQUIRE(loop.kids.size() == 2);
        expect_comparison(result, *loop.kids[0], name, op, bound);
        REQUIRE(loop.kids[1]->type == AST::COMPOUND_STMT);
        REQUIRE(loop.kids[1]->kids.size() == 1);
        expect_literal_assignment(result, *loop.kids[1]->kids[0], name, replacement);
        expect_literal_assignment(result, *root.kids[2], name, following);
    }
}

TEST_CASE("Parser randomized: nested if and while do not swallow siblings",
          "[parser][randomized][nested]") {
    constexpr int iterations = 200;
    const auto seed = test_seed();
    auto rng = generator(0x33);

    for (int i = 0; i < iterations; ++i) {
        const auto name = identifier(rng);
        const int depth = random_int(rng, 1, 6);
        std::vector<AST::NodeType> expected_types;
        std::vector<std::string> condition_literals;
        std::vector<std::string> following_literals;
        std::string source = "void f ( ) { int " + name + " = 10 ; ";

        for (int level = 0; level < depth; ++level) {
            const auto keyword = pick(rng, control_types);
            expected_types.push_back(keyword == "if" ? AST::IF_STMT : AST::WHILE_STMT);
            const auto limit = std::to_string(random_int(rng, 0, 10000));
            condition_literals.push_back(limit);
            source += keyword + " ( " + name + " > " + limit + " ) { ";
        }
        const auto deepest = std::to_string(random_int(rng, 0, 10000));
        source += name + " = " + deepest + " ; ";
        for (int level = depth - 1; level >= 0; --level) {
            const auto after = std::to_string(random_int(rng, 0, 10000));
            following_literals.push_back(after);
            source += "} " + name + " = " + after + " ; ";
        }
        source += "}";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);

        const auto parsed = parse_words_randomized(source);
        const auto& top = body_of(parsed);
        REQUIRE(top.kids.size() == 3);  // initial declaration, outer block, trailing assign
        const AST::Node* node = top.kids[1].get();
        for (int level = 0; level < depth; ++level) {
            REQUIRE(node->type == expected_types[static_cast<size_t>(level)]);
            REQUIRE(node->kids.size() == 2);
            expect_comparison(parsed, *node->kids[0], name, ">",
                              condition_literals[static_cast<size_t>(level)]);
            REQUIRE(node->kids[1]->type == AST::COMPOUND_STMT);
            const auto& block = *node->kids[1];
            REQUIRE(block.kids.size() == (level == depth - 1 ? 1u : 2u));
            node = block.kids[0].get();
        }
        expect_literal_assignment(parsed, *node, name, deepest);
        // Verify each assignment after a nested block stays in the parent block.
        const AST::Node* parent = top.kids[1].get();
        for (int level = 0; level < depth - 1; ++level) {
            const auto& block = *parent->kids[1];
            REQUIRE(block.kids.size() == 2);
            expect_literal_assignment(
                parsed, *block.kids[1], name,
                following_literals[static_cast<size_t>(depth - 2 - level)]);
            parent = block.kids[0].get();
        }
        // Assignment after the entire outermost block belongs to the function.
        expect_literal_assignment(parsed, *top.kids[2], name,
                                  following_literals.back());
    }
}

TEST_CASE("Parser randomized: comparison AND/OR precedence",
          "[parser][randomized][precedence]") {
    constexpr int iterations = 200;
    const auto seed = test_seed();
    auto rng = generator(0x44);

    for (int i = 0; i < iterations; ++i) {
        const auto first = identifier(rng);
        const auto second = identifier(rng);
        const auto third = identifier(rng);
        const auto a = pick(rng, comparisons);
        const auto b = pick(rng, comparisons);
        const auto c = pick(rng, comparisons);
        const auto v1 = std::to_string(random_int(rng, 0, 10000));
        const auto v2 = std::to_string(random_int(rng, 0, 10000));
        const auto v3 = std::to_string(random_int(rng, 0, 10000));
        const auto source = "void f ( ) { if ( " + first + " " + a + " " + v1 +
            " || " + second + " " + b + " " + v2 + " && " + third +
            " " + c + " " + v3 + " ) { " + first + " = 1 ; } }";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);

        const auto parsed = parse_words_randomized(source);
        const auto& top = body_of(parsed);
        REQUIRE(top.kids.size() == 1);
        REQUIRE(top.kids[0]->type == AST::IF_STMT);
        const auto& condition = *top.kids[0]->kids[0];
        REQUIRE(condition.type == AST::BINARY_OPERATION);
        REQUIRE(condition.kids.size() == 2);
        CHECK(lexeme(parsed, condition) == "||");
        expect_comparison(parsed, *condition.kids[0], first, a, v1);
        const auto& rhs = *condition.kids[1];
        REQUIRE(rhs.type == AST::BINARY_OPERATION);
        REQUIRE(rhs.kids.size() == 2);
        CHECK(lexeme(parsed, rhs) == "&&");
        expect_comparison(parsed, *rhs.kids[0], second, b, v2);
        expect_comparison(parsed, *rhs.kids[1], third, c, v3);
    }
}

TEST_CASE("Parser randomized: malformed if/while syntax is rejected",
          "[parser][randomized][errors]") {
    constexpr int iterations = 300;
    const auto seed = test_seed();
    auto rng = generator(0x55);

    for (int i = 0; i < iterations; ++i) {
        const auto control = pick(rng, control_types);
        const auto name = identifier(rng);
        const auto value = std::to_string(random_int(rng, 0, 10000));
        const auto case_id = random_int(rng, 0, 3);
        std::string invalid;
        switch (case_id) {
            case 0: // missing '('
                invalid = control + " " + name + " > " + value + " ) { " +
                          name + " = 1 ; }";
                break;
            case 1: // missing ')'
                invalid = control + " ( " + name + " > " + value + " { " +
                          name + " = 1 ; }";
                break;
            case 2: // missing '{' after condition
                invalid = control + " ( " + name + " > " + value + " ) " +
                          name + " = 1 ;";
                break;
            default: // empty condition
                invalid = control + " ( ) { " + name + " = 1 ; }";
                break;
        }
        const auto source = "void f ( ) { int " + name + " = 1 ; " + invalid + " }";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);
        REQUIRE_THROWS_AS(parse_words_randomized(source), vm::syntax_error);
    }
}

TEST_CASE("Parser randomized: unimplemented for reports a stable error",
          "[parser][randomized][for][current-limitations]") {
    constexpr int iterations = 80;
    const auto seed = test_seed();
    auto rng = generator(0x66);

    for (int i = 0; i < iterations; ++i) {
        const auto name = identifier(rng);
        const auto start = std::to_string(random_int(rng, 0, 10000));
        const auto bound = std::to_string(random_int(rng, 0, 10000));
        const auto step = std::to_string(random_int(rng, 1, 30));
        const auto source = "void f ( ) { for ( " + name + " = " + start +
            " ; " + name + " < " + bound + " ; " + name + " = " +
            name + " + " + step + " ) { " + name + " = 0 ; } }";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);

        try {
            (void)parse_words_randomized(source);
            FAIL("for must report its current implementation status");
        } catch (const vm::syntax_error& error) {
            CHECK(std::string(error.what()).find("for is not implemented yet") !=
                  std::string::npos);
        }
    }
}

TEST_CASE("Parser randomized: Lexer to AST integration with valid spacing",
          "[parser][randomized][integration]") {
    constexpr int iterations = 60;
    const auto seed = test_seed();
    auto rng = generator(0x77);
    Lexer lexer;

    for (int i = 0; i < iterations; ++i) {
        const auto name = identifier(rng);
        const auto op = pick(rng, comparisons);
        const auto init = std::to_string(random_int(rng, 0, 10000));
        const auto bound = std::to_string(random_int(rng, 0, 10000));
        const auto source = "void f(){\n  int " + name + " = " + init + ";\n" +
            "  if(" + name + " " + op + " " + bound + "){" + name +
            " = 3;}\n" + "  while(" + name + " " + op + " " + bound +
            "){" + name + " = 4;}\n}";
        INFO("seed=" << seed << " iteration=" << i << " code=" << source);

        TempInput file(static_cast<size_t>(i), source);
        const auto parsed = parse_tokens_randomized(lexer.load_file(file.path.string()));
        const auto& top = body_of(parsed);
        REQUIRE(top.kids.size() == 3);
        REQUIRE(top.kids[0]->type == AST::ASSIGN_STMT);
        REQUIRE(top.kids[1]->type == AST::IF_STMT);
        REQUIRE(top.kids[2]->type == AST::WHILE_STMT);
        expect_comparison(parsed, *top.kids[1]->kids[0], name, op, bound);
        expect_comparison(parsed, *top.kids[2]->kids[0], name, op, bound);
    }
}
