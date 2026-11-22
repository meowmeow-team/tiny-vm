// Additional regression tests for tiny-vm 0.4.5.
// The input below is a STREAM OF TOKENS, not raw source text.
// Whitespace between *all* lexemes lets us isolate Parser from Lexer.

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ast.hpp"
#include "exception.hpp"
#include "logger.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "source_manager.hpp"
#include "type_holder.hpp"

namespace {

struct ParseResult {
    std::vector<Token> tokens;
    AST::UnitDecl unit;
};

// Example: "void main ( ) { if ( x > 0 ) { x = 1 ; } }"
// Each whitespace-separated word becomes one Token.
// This helper intentionally does not run Lexer::load_file().
ParseResult parse_tokens(std::vector<Token> tokens) {
    Logger logger;
    SourceManager source(tokens);
    Parser parser(source, logger);
    auto unit = parser.parse_unit("control_flow_test");
    return {std::move(tokens), std::move(unit)};
}

ParseResult parse_words(const std::string& words) {
    std::istringstream input(words);
    std::vector<Token> tokens;
    std::string word;
    uint32_t offset = 0;

    while (input >> word) {
        const auto type = TypeHolder::find(word);
        if (type == TokenType::UNKNOWN) {
            throw std::invalid_argument("unknown test lexeme: " + word);
        }
        tokens.emplace_back(type, Position{offset, 1, offset + 1}, word);
        offset += static_cast<uint32_t>(word.size() + 1);
    }

    return parse_tokens(std::move(tokens));
}

struct TemporarySource {
    std::filesystem::path path;

    TemporarySource(const std::string& name, const std::string& content)
        : path(std::filesystem::temp_directory_path() /
               (name + "_" + std::to_string(
                   std::chrono::steady_clock::now().time_since_epoch().count()))) {
        std::ofstream output(path);
        if (!output) throw std::runtime_error("failed to create test input");
        output << content;
    }

    ~TemporarySource() {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
    }
};

const AST::Node& function_body(const ParseResult& result) {
    REQUIRE(result.unit.kids.size() == 1);
    const auto& func = *result.unit.kids[0];
    REQUIRE(func.type == AST::FUNC_DECL);
    REQUIRE(func.kids.size() == 1); // The examples use zero parameters.
    REQUIRE(func.kids[0]->type == AST::COMPOUND_STMT);
    return *func.kids[0];
}

const std::string& node_lexeme(const ParseResult& result, const AST::Node& node) {
    return result.tokens.at(node.ref_idx).get_lexeme();
}

} // namespace

TEST_CASE("Parser: if produces a condition and body", "[parser][if][ast]") {
    const auto result = parse_words(
        "void main ( ) { int a = 10 ; if ( a > 5 ) { a = 1 ; } }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 2);
    REQUIRE(body.kids[0]->type == AST::ASSIGN_STMT);

    const auto& branch = *body.kids[1];
    REQUIRE(branch.type == AST::IF_STMT);
    REQUIRE(branch.kids.size() == 2);

    const auto& condition = *branch.kids[0];
    REQUIRE(condition.type == AST::BINARY_OPERATION);
    REQUIRE(node_lexeme(result, condition) == ">");
    REQUIRE(condition.kids.size() == 2);
    CHECK(condition.kids[0]->type == AST::REF_VAR_DECL);
    CHECK(node_lexeme(result, *condition.kids[0]) == "a");
    CHECK(condition.kids[1]->type == AST::INT_LITERAL_DECL);
    CHECK(node_lexeme(result, *condition.kids[1]) == "5");

    const auto& if_body = *branch.kids[1];
    REQUIRE(if_body.type == AST::COMPOUND_STMT);
    REQUIRE(if_body.kids.size() == 1);
    REQUIRE(if_body.kids[0]->type == AST::ASSIGN_STMT);
}

TEST_CASE("Parser: if accepts an identifier condition and empty body",
          "[parser][if][ast]") {
    const auto result = parse_words("void main ( ) { if ( ready ) { } }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 1);
    const auto& branch = *body.kids[0];
    REQUIRE(branch.type == AST::IF_STMT);
    REQUIRE(branch.kids.size() == 2);
    REQUIRE(branch.kids[0]->type == AST::REF_VAR_DECL);
    CHECK(node_lexeme(result, *branch.kids[0]) == "ready");
    REQUIRE(branch.kids[1]->type == AST::COMPOUND_STMT);
    CHECK(branch.kids[1]->kids.empty());
}

TEST_CASE("Parser: while produces condition, body and assignment expression",
          "[parser][while][ast]") {
    const auto result = parse_words(
        "void main ( ) { int count = 0 ; while ( count < 3 ) { count = count + 1 ; } }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 2);
    const auto& loop = *body.kids[1];
    REQUIRE(loop.type == AST::WHILE_STMT);
    REQUIRE(loop.kids.size() == 2);
    REQUIRE(loop.kids[0]->type == AST::BINARY_OPERATION);
    CHECK(node_lexeme(result, *loop.kids[0]) == "<");

    const auto& loop_body = *loop.kids[1];
    REQUIRE(loop_body.type == AST::COMPOUND_STMT);
    REQUIRE(loop_body.kids.size() == 1);
    const auto& assign = *loop_body.kids[0];
    REQUIRE(assign.type == AST::ASSIGN_STMT);
    REQUIRE(assign.kids.size() == 2);
    REQUIRE(assign.kids[0]->type == AST::REF_VAR_DECL);
    REQUIRE(assign.kids[1]->type == AST::BINARY_OPERATION);
    CHECK(node_lexeme(result, *assign.kids[1]) == "+");
}

TEST_CASE("Parser: while accepts a simple identifier condition",
          "[parser][while][ast]") {
    const auto result = parse_words("void main ( ) { while ( ready ) { ready = 0 ; } }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 1);
    const auto& loop = *body.kids[0];
    REQUIRE(loop.type == AST::WHILE_STMT);
    REQUIRE(loop.kids.size() == 2);
    CHECK(loop.kids[0]->type == AST::REF_VAR_DECL);
    CHECK(node_lexeme(result, *loop.kids[0]) == "ready");
    CHECK(loop.kids[1]->type == AST::COMPOUND_STMT);
}

TEST_CASE("Parser: nested while and if retain their own blocks",
          "[parser][if][while][nested]") {
    const auto result = parse_words(
        "void main ( ) { while ( x > 0 ) { if ( y == 1 ) { y = 2 ; } x = x - 1 ; } }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 1);
    const auto& outer_while = *body.kids[0];
    REQUIRE(outer_while.type == AST::WHILE_STMT);
    REQUIRE(outer_while.kids.size() == 2);
    const auto& while_body = *outer_while.kids[1];
    REQUIRE(while_body.type == AST::COMPOUND_STMT);
    REQUIRE(while_body.kids.size() == 2);

    const auto& inner_if = *while_body.kids[0];
    REQUIRE(inner_if.type == AST::IF_STMT);
    REQUIRE(inner_if.kids.size() == 2);
    REQUIRE(inner_if.kids[1]->type == AST::COMPOUND_STMT);
    REQUIRE(inner_if.kids[1]->kids.size() == 1);
    CHECK(inner_if.kids[1]->kids[0]->type == AST::ASSIGN_STMT);
    CHECK(while_body.kids[1]->type == AST::ASSIGN_STMT);
}

TEST_CASE("Parser: if with nested while does not consume the next statement",
          "[parser][if][while][nested]") {
    const auto result = parse_words(
        "void main ( ) { if ( a == 1 ) { while ( b < 3 ) { b = b + 1 ; } } a = 3 ; }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 2);
    const auto& branch = *body.kids[0];
    REQUIRE(branch.type == AST::IF_STMT);
    REQUIRE(branch.kids.size() == 2);
    const auto& if_body = *branch.kids[1];
    REQUIRE(if_body.type == AST::COMPOUND_STMT);
    REQUIRE(if_body.kids.size() == 1);
    REQUIRE(if_body.kids[0]->type == AST::WHILE_STMT);
    CHECK(body.kids[1]->type == AST::ASSIGN_STMT);
}

TEST_CASE("Parser: logical operators in if conditions preserve precedence",
          "[parser][if][precedence]") {
    const auto result = parse_words(
        "void main ( ) { if ( a > 0 && b < 5 ) { a = 1 ; } }");
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 1);
    const auto& condition = *body.kids[0]->kids[0];
    REQUIRE(condition.type == AST::BINARY_OPERATION);
    REQUIRE(node_lexeme(result, condition) == "&&");
    REQUIRE(condition.kids.size() == 2);
    REQUIRE(condition.kids[0]->type == AST::BINARY_OPERATION);
    REQUIRE(condition.kids[1]->type == AST::BINARY_OPERATION);
    CHECK(node_lexeme(result, *condition.kids[0]) == ">");
    CHECK(node_lexeme(result, *condition.kids[1]) == "<");
}

TEST_CASE("Parser: Lexer -> Parser integration for if and while",
          "[parser][integration][if][while]") {
    TemporarySource file(
        "tiny_vm_parser_control_flow_045.test",
        "void main() { int a = 10; if (a > 5) { a = 1; } "
        "while (a > 0) { a = 0; } }");
    Lexer lexer;
    auto result = parse_tokens(lexer.load_file(file.path.string()));
    const auto& body = function_body(result);
    REQUIRE(body.kids.size() == 3);
    CHECK(body.kids[0]->type == AST::ASSIGN_STMT);
    REQUIRE(body.kids[1]->type == AST::IF_STMT);
    REQUIRE(body.kids[2]->type == AST::WHILE_STMT);
    CHECK(node_lexeme(result, *body.kids[1]->kids[0]) == ">");
    CHECK(node_lexeme(result, *body.kids[2]->kids[0]) == ">");
}

TEST_CASE("Parser: rejects malformed if syntax", "[parser][if][errors]") {
    SECTION("Missing opening parenthesis") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { if a > 0 ) { a = 1 ; } }"), vm::syntax_error);
    }
    SECTION("Missing closing parenthesis") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { if ( a > 0 { a = 1 ; } }"), vm::syntax_error);
    }
    SECTION("Missing opening body brace") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { if ( a > 0 ) a = 1 ; }"), vm::syntax_error);
    }
    SECTION("Empty condition") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { if ( ) { a = 1 ; } }"), vm::syntax_error);
    }
    SECTION("Unclosed body") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { if ( a > 0 ) { a = 1 ; }"), vm::syntax_error);
    }
}

TEST_CASE("Parser: rejects malformed while syntax", "[parser][while][errors]") {
    SECTION("Missing opening parenthesis") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { while a > 0 ) { a = 1 ; } }"), vm::syntax_error);
    }
    SECTION("Missing closing parenthesis") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { while ( a > 0 { a = 1 ; } }"), vm::syntax_error);
    }
    SECTION("Missing opening body brace") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { while ( a > 0 ) a = 1 ; }"), vm::syntax_error);
    }
    SECTION("Empty condition") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { while ( ) { a = 1 ; } }"), vm::syntax_error);
    }
    SECTION("Unclosed body") {
        REQUIRE_THROWS_AS(parse_words(
            "void main ( ) { while ( a > 0 ) { a = 1 ; }"), vm::syntax_error);
    }
}

TEST_CASE("Parser: for explicitly reports not implemented",
          "[parser][for][current-limitations]") {
    REQUIRE_THROWS_AS(parse_words(
        "void main ( ) { for ( i = 0 ; i < 3 ; i = i + 1 ) { i = 1 ; } }"),
        vm::syntax_error);

    try {
        (void)parse_words(
            "void main ( ) { for ( i = 0 ; i < 3 ; i = i + 1 ) { i = 1 ; } }");
        FAIL("The current version must reject an unimplemented for loop");
    } catch (const vm::syntax_error& error) {
        CHECK(std::string(error.what()).find("for is not implemented yet") !=
              std::string::npos);
    }
}

TEST_CASE("Parser: else is not implemented in this version",
          "[parser][if][current-limitations]") {
    REQUIRE_THROWS_AS(parse_words(
        "void main ( ) { if ( x > 0 ) { x = 1 ; } else { x = 2 ; } }"),
        vm::syntax_error);
}
