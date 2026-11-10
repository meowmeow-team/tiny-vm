#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <random>
#include <vector>

#include "ast.hpp"
#include "exception.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "source_manager.hpp"

inline AST::UnitDecl parse_tokens(std::vector<Token> tokens) {
    Logger logger;
    SourceManager sm(std::move(tokens));
    Parser parser(sm, logger);
    return parser.parse_unit("unit_test");
}

TEST_CASE("Parser: Parameterized functions declaration", "[parser]") {
    SECTION("Functions with various count of parameters") {
        // Тестируем функции от 1 до 5 аргументов
        for (int param_count = 1; param_count <= 5; ++param_count) {
            std::vector<Token> tokens = {
                {TokenType::KEYWORD_TYPE, {0, 1, 1}, "int"},
                {TokenType::IDENTIFIER,   {4, 1, 5}, "calculate"},
                {TokenType::SYMBOL,       {13, 1, 14}, "("}
            };

            for (int p = 0; p < param_count; ++p) {
                if (p > 0) {
                    tokens.emplace_back(TokenType::SYMBOL, Position{0, 1, 1}, ",");
                }
                tokens.emplace_back(TokenType::KEYWORD_TYPE, Position{0, 1, 1}, "int");
                tokens.emplace_back(TokenType::IDENTIFIER, Position{0, 1, 1}, "arg" + std::to_string(p));
            }

            // Закрываем сигнатуру и добавляем тело с присваиванием (>= 8 токенов)
            tokens.emplace_back(TokenType::SYMBOL, Position{0, 1, 1}, ")");
            tokens.emplace_back(TokenType::SYMBOL, Position{0, 1, 1}, "{");
            tokens.emplace_back(TokenType::KEYWORD_TYPE, Position{0, 1, 1}, "int");
            tokens.emplace_back(TokenType::IDENTIFIER, Position{0, 1, 1}, "res");
            tokens.emplace_back(TokenType::SYMBOL_ASSIGN, Position{0, 1, 1}, "=");
            tokens.emplace_back(TokenType::NUMBER, Position{0, 1, 1}, "0");
            tokens.emplace_back(TokenType::SEMICOLON, Position{0, 1, 1}, ";");
            tokens.emplace_back(TokenType::SYMBOL, Position{0, 1, 1}, "}");

            auto unit = parse_tokens(tokens);
            REQUIRE(unit.kids.size() == 1);

            auto* func = dynamic_cast<AST::FunctionDecl*>(unit.kids[0].get());
            REQUIRE(func != nullptr);

            // В func->kids идут param_count параметров и один CompoundStmt в конце
            REQUIRE(func->kids.size() == static_cast<size_t>(param_count + 1));
            for (int p = 0; p < param_count; ++p) {
                CHECK(func->kids[p]->type == AST::PARAM_DECL);
            }
            CHECK(func->kids[param_count]->type == AST::COMPOUND_STMT);
        }
    }
}

TEST_CASE("Parser: Pratt parsing randomized arithmetic chains", "[parser]") {
    std::mt19937_64 rng(31415);
    const std::vector<std::string> ops = {"+", "-", "*", "/"};

    SECTION("Random binary operations tree depth and precedence") {
        for (int test_run = 0; test_run < 30; ++test_run) {
            std::vector<Token> tokens = {
                {TokenType::KEYWORD_TYPE,  {0, 1, 1}, "void"},
                {TokenType::IDENTIFIER,    {5, 1, 6}, "exprFunc"},
                {TokenType::SYMBOL,        {13, 1, 14}, "("},
                {TokenType::SYMBOL,        {14, 1, 15}, ")"},
                {TokenType::SYMBOL,        {16, 1, 17}, "{"},
                {TokenType::IDENTIFIER,    {18, 1, 19}, "val"},
                {TokenType::SYMBOL_ASSIGN, {22, 1, 23}, "="},
                {TokenType::NUMBER,        {24, 1, 25}, "10"}
            };

            // Добавляем цепочку из 3 случайных операций
            for (int op_i = 0; op_i < 3; ++op_i) {
                std::string chosen_op = ops[rng() % ops.size()];
                tokens.emplace_back(TokenType::OPERATOR, Position{0, 1, 1}, chosen_op);
                tokens.emplace_back(TokenType::NUMBER, Position{0, 1, 1}, std::to_string(op_i + 1));
            }

            tokens.emplace_back(TokenType::SEMICOLON, Position{0, 1, 1}, ";");
            tokens.emplace_back(TokenType::SYMBOL, Position{0, 1, 1}, "}");

            auto unit = parse_tokens(tokens);
            REQUIRE(unit.kids.size() == 1);

            auto* func = dynamic_cast<AST::FunctionDecl*>(unit.kids[0].get());
            REQUIRE(func != nullptr);

            auto* body = dynamic_cast<AST::CompoundStmt*>(func->kids[0].get());
            REQUIRE(body != nullptr);
            REQUIRE(body->kids.size() == 1);

            auto* assign = dynamic_cast<AST::AssignStmt*>(body->kids[0].get());
            REQUIRE(assign != nullptr);

            // Правая часть должна успешно свернуться в BinaryOperation
            auto* right_expr = dynamic_cast<AST::BinaryOperation*>(assign->kids[1].get());
            REQUIRE(right_expr != nullptr);
        }
    }
}

TEST_CASE("Parser: Syntax errors and invalid structure handling", "[parser]") {
    Logger logger;

    SECTION("Fails on invalid return type") {
        std::vector<Token> tokens = {
            {TokenType::NUMBER,     {0, 1, 1}, "123"}, // Число вместо типа возврата
            {TokenType::IDENTIFIER, {4, 1, 5}, "foo"},
            {TokenType::SYMBOL,     {7, 1, 8}, "("},
            {TokenType::SYMBOL,     {8, 1, 9}, ")"},
            {TokenType::SYMBOL,     {10, 1, 11}, "{"},
            {TokenType::KEYWORD_TYPE,{12, 1, 13}, "int"},
            {TokenType::IDENTIFIER, {16, 1, 17}, "a"},
            {TokenType::SYMBOL_ASSIGN,{18, 1, 19}, "="},
            {TokenType::NUMBER,     {20, 1, 21}, "1"},
            {TokenType::SEMICOLON,  {21, 1, 22}, ";"},
            {TokenType::SYMBOL,     {23, 1, 24}, "}"}
        };
        SourceManager sm(tokens);
        Parser parser(sm, logger);
        REQUIRE_THROWS_AS(parser.parse_unit("test"), vm::syntax_error);
    }

    SECTION("Fails on missing function name") {
        std::vector<Token> tokens = {
            {TokenType::KEYWORD_TYPE, {0, 1, 1}, "void"},
            {TokenType::SYMBOL,       {5, 1, 6}, "("}, // Пропущено имя функции
            {TokenType::SYMBOL,       {6, 1, 7}, ")"},
            {TokenType::SYMBOL,       {8, 1, 9}, "{"},
            {TokenType::KEYWORD_TYPE, {10, 1, 11}, "int"},
            {TokenType::IDENTIFIER,   {14, 1, 15}, "a"},
            {TokenType::SYMBOL_ASSIGN,{16, 1, 17}, "="},
            {TokenType::NUMBER,       {18, 1, 19}, "1"},
            {TokenType::SEMICOLON,    {19, 1, 20}, ";"},
            {TokenType::SYMBOL,       {21, 1, 22}, "}"}
        };
        SourceManager sm(tokens);
        Parser parser(sm, logger);
        REQUIRE_THROWS_AS(parser.parse_unit("test"), vm::syntax_error);
    }

    SECTION("Fails on unclosed parenthesis in signature") {
        std::vector<Token> tokens = {
            {TokenType::KEYWORD_TYPE, {0, 1, 1}, "int"},
            {TokenType::IDENTIFIER,   {4, 1, 5}, "bar"},
            {TokenType::SYMBOL,       {7, 1, 8}, "("},
            {TokenType::KEYWORD_TYPE, {8, 1, 9}, "int"},
            {TokenType::IDENTIFIER,   {12, 1, 13}, "x"},
            // Пропущена закрывающая ')'
            {TokenType::SYMBOL,       {14, 1, 15}, "{"},
            {TokenType::SYMBOL,       {15, 1, 16}, "}"}
        };
        SourceManager sm(tokens);
        Parser parser(sm, logger);
        REQUIRE_THROWS_AS(parser.parse_unit("test"), vm::syntax_error);
    }
}