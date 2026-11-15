// Semantic analysis tests for tiny-vm output1.txt (2026-11-15).
// Catch2 v3.5.2. Does not modify production code.
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "analyzer.hpp"
#include "ast.hpp"
#include "exception.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "source_manager.hpp"
#include "symbol_table.hpp"

namespace {

struct TempSource {
    std::filesystem::path path;

    explicit TempSource(std::string_view source) {
        static uint64_t counter = 0;
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("tiny_vm_semantic_" + std::to_string(stamp) + "_" +
                std::to_string(++counter) + ".txt");
        std::ofstream output(path, std::ios::binary);
        if (!output) throw std::runtime_error("Cannot create temporary source file");
        output << source;
        if (!output) throw std::runtime_error("Cannot write temporary source file");
    }

    ~TempSource() {
        std::error_code error;
        std::filesystem::remove(path, error);
    }

    TempSource(const TempSource&) = delete;
    TempSource& operator=(const TempSource&) = delete;
};

// End-to-end pipeline for the syntax actually handled by output1.txt:
// input text -> Lexer -> SourceManager -> Parser -> AST -> Analyzer.
void analyze_source(std::string_view source) {
    TempSource file(source);
    Lexer lexer;
    auto tokens = lexer.load_file(file.path.string());
    SourceManager manager(std::move(tokens));
    Logger logger;
    Parser parser(manager, logger);
    auto unit = parser.parse_unit("semantic_test");
    Analyzer analyzer(manager);
    analyzer.process_unit(&unit);
}

// Useful when testing parameter lists: output1.txt's lexer does not recognize
// commas, even though the parser supports comma tokens.
Token token(TokenType type, const char* text) {
    return Token(type, Position{0, 1, 1}, text);
}

void analyze_tokens(std::vector<Token> tokens) {
    SourceManager manager(std::move(tokens));
    Logger logger;
    Parser parser(manager, logger);
    auto unit = parser.parse_unit("manual_tokens");
    Analyzer analyzer(manager);
    analyzer.process_unit(&unit);
}

} // namespace

TEST_CASE("SymbolTable: stores and searches typed symbols", "[semantic][symbol_table]") {
    SymbolTable global(true);
    SymbolTable local(false);
    const SymbolTable::Meta var{SymbolTable::INT, SymbolTable::VARIABLE};
    const SymbolTable::Meta func{SymbolTable::VOID, SymbolTable::FUNCTION};

    REQUIRE(global.is_global());
    REQUIRE_FALSE(local.is_global());
    REQUIRE_FALSE(global.contains("counter"));

    global.push("counter", var);
    global.push("start", func);

    CHECK(global.contains("counter"));
    CHECK(global.contains("counter", SymbolTable::INT));
    CHECK(global.contains("counter", SymbolTable::VARIABLE));
    CHECK(global.contains("counter", var));
    CHECK_FALSE(global.contains("counter", SymbolTable::CHAR));
    CHECK_FALSE(global.contains("counter", SymbolTable::FUNCTION));
    CHECK(global.get("counter") == var);
    CHECK(global.get("start") == func);
    CHECK_FALSE(local.contains("counter"));
}

TEST_CASE("SymbolTable: rejects invalid insertions and missing symbols", "[semantic][symbol_table]") {
    SymbolTable table(false);
    const SymbolTable::Meta var{SymbolTable::INT, SymbolTable::VARIABLE};

    REQUIRE_THROWS_AS(table.push("", var), vm::symbol_table_error);
    REQUIRE_THROWS_AS(table.get("missing"), vm::symbol_table_error);

    table.push("x", var);
    REQUIRE_THROWS_AS(table.push("x", var), vm::symbol_table_error);
}

TEST_CASE("SymbolTable: token-to-type conversion matches current implementation", "[semantic][symbol_table]") {
    CHECK(SymbolTable::make_type(token(TokenType::KEYWORD_TYPE, "int")) == SymbolTable::INT);
    CHECK(SymbolTable::make_type(token(TokenType::KEYWORD_TYPE, "char")) == SymbolTable::CHAR);
    CHECK(SymbolTable::make_type(token(TokenType::KEYWORD_TYPE, "void")) == SymbolTable::VOID);

    // At this version float and double are lexer keywords, but not semantic types.
    REQUIRE_THROWS_AS(SymbolTable::make_type(token(TokenType::KEYWORD_TYPE, "float")),
                      vm::symbol_table_error);
    REQUIRE_THROWS_AS(SymbolTable::make_type(token(TokenType::KEYWORD_TYPE, "double")),
                      vm::symbol_table_error);
    REQUIRE_THROWS_AS(SymbolTable::make_meta(token(TokenType::IDENTIFIER, "int"),
                                             SymbolTable::VARIABLE),
                      vm::symbol_table_error);

    const auto meta = SymbolTable::make_meta(
        token(TokenType::KEYWORD_TYPE, "int"), SymbolTable::FUNCTION);
    CHECK(meta.type == SymbolTable::INT);
    CHECK(meta.group == SymbolTable::FUNCTION);
}

TEST_CASE("Analyzer: handles declarations, reassignment and arithmetic", "[semantic][integration][positive]") {
    REQUIRE_NOTHROW(analyze_source(R"(
        void main() {
            int first = 10;
            int second = first + 3;
            second = second * 2;
            first = second - 1;
        }
    )"));
}

TEST_CASE("Analyzer: resolves function parameters", "[semantic][integration][positive]") {
    REQUIRE_NOTHROW(analyze_source(R"(
        int calculate(int input) {
            int result = input + 2;
            result = result / 2;
        }
    )"));
}

TEST_CASE("Analyzer: keeps local names separate between functions", "[semantic][integration][positive]") {
    REQUIRE_NOTHROW(analyze_source(R"(
        void alpha() { int value = 1; }
        void beta()  { int value = 2; }
    )"));
}

TEST_CASE("Analyzer: rejects duplicate functions", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void same() { int x = 1; }
        void same() { int y = 2; }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects duplicate variables in one function", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void main() {
            int x = 1;
            int x = 2;
        }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects local variable conflicting with a parameter", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void main(int x) {
            int x = 2;
        }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects duplicate parameter names", "[semantic][manual_tokens][negative]") {
    std::vector<Token> tokens{
        token(TokenType::KEYWORD_TYPE, "int"),
        token(TokenType::IDENTIFIER, "calculate"),
        token(TokenType::SYMBOL, "("),
        token(TokenType::KEYWORD_TYPE, "int"),
        token(TokenType::IDENTIFIER, "a"),
        token(TokenType::SYMBOL, ","),
        token(TokenType::KEYWORD_TYPE, "int"),
        token(TokenType::IDENTIFIER, "a"),
        token(TokenType::SYMBOL, ")"),
        token(TokenType::SYMBOL, "{"),
        token(TokenType::KEYWORD_TYPE, "int"),
        token(TokenType::IDENTIFIER, "x"),
        token(TokenType::SYMBOL_ASSIGN, "="),
        token(TokenType::NUMBER, "1"),
        token(TokenType::SEMICOLON, ";"),
        token(TokenType::SYMBOL, "}")
    };
    REQUIRE_THROWS_AS(analyze_tokens(std::move(tokens)), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects undeclared variable on assignment left side", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void main() { unknown = 3; }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects undeclared variable in expression", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void main() {
            int x = 1;
            int y = x + unknown;
        }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects incompatible variable types", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void main(char ch) { int x = ch; }
    )"), vm::semantic_error);

    REQUIRE_THROWS_AS(analyze_source(R"(
        void main(char ch) { int x = ch + 1; }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects integer literal assigned to char", "[semantic][integration][negative]") {
    REQUIRE_THROWS_AS(analyze_source(R"(
        void main() { char ch = 65; }
    )"), vm::semantic_error);
}

TEST_CASE("Analyzer: rejects null root AST", "[semantic][ast][negative]") {
    std::vector<Token> tokens{token(TokenType::NUMBER, "0")};
    SourceManager manager(std::move(tokens));
    Analyzer analyzer(manager);
    REQUIRE_THROWS_AS(analyzer.process_unit(nullptr), vm::semantic_error);
}

TEST_CASE("Analyzer: can reference a global symbol in a function from a manually built AST", "[semantic][ast][positive]") {
    // The current parser cannot produce a top-level variable declaration;
    // this constructs one using the AST shapes the analyzer supports.
    std::vector<Token> tokens{
        token(TokenType::KEYWORD_TYPE, "int"),       // index 0
        token(TokenType::IDENTIFIER, "shared"),      // index 1
        token(TokenType::NUMBER, "5"),               // index 2
        token(TokenType::KEYWORD_TYPE, "void"),      // index 3
        token(TokenType::IDENTIFIER, "main"),        // index 4
        token(TokenType::IDENTIFIER, "shared"),      // index 5
        token(TokenType::NUMBER, "7")                // index 6
    };
    SourceManager manager(std::move(tokens));
    Analyzer analyzer(manager);
    AST::UnitDecl unit("global_test");

    auto global_assign = std::make_unique<AST::AssignStmt>();
    auto global_decl = std::make_unique<AST::VariableDecl>();
    global_decl->ref_type = 0;
    global_decl->ref_idx = 1;
    auto initial_value = std::make_unique<AST::IntLiteralDecl>();
    initial_value->ref_idx = 2;
    global_assign->kids.push_back(std::move(global_decl));
    global_assign->kids.push_back(std::move(initial_value));
    unit.kids.push_back(std::move(global_assign));

    auto function = std::make_unique<AST::FunctionDecl>();
    function->ref_type = 3;
    function->ref_idx = 4;
    auto body = std::make_unique<AST::CompoundStmt>();
    auto assign = std::make_unique<AST::AssignStmt>();
    auto reference = std::make_unique<AST::RefVarDecl>();
    reference->ref_idx = 5;
    auto value = std::make_unique<AST::IntLiteralDecl>();
    value->ref_idx = 6;
    assign->kids.push_back(std::move(reference));
    assign->kids.push_back(std::move(value));
    body->kids.push_back(std::move(assign));
    function->kids.push_back(std::move(body));
    unit.kids.push_back(std::move(function));

    REQUIRE_NOTHROW(analyzer.process_unit(&unit));
}

TEST_CASE("Analyzer: rejects an assignment AST with incorrect child count", "[semantic][ast][negative]") {
    std::vector<Token> tokens{
        token(TokenType::KEYWORD_TYPE, "void"),
        token(TokenType::IDENTIFIER, "main")
    };
    SourceManager manager(std::move(tokens));
    Analyzer analyzer(manager);
    AST::UnitDecl unit;
    auto function = std::make_unique<AST::FunctionDecl>();
    function->ref_type = 0;
    function->ref_idx = 1;
    auto body = std::make_unique<AST::CompoundStmt>();
    body->kids.push_back(std::make_unique<AST::AssignStmt>());
    function->kids.push_back(std::move(body));
    unit.kids.push_back(std::move(function));

    REQUIRE_THROWS_AS(analyzer.process_unit(&unit), vm::semantic_error);
}

TEST_CASE("Analyzer: deterministic randomized valid programs", "[semantic][integration][randomized][positive]") {
    std::mt19937 rng(20261115);
    for (int i = 0; i < 100; ++i) {
        const auto first = std::to_string(rng() % 100000);
        const auto second = std::to_string(rng() % 100000);
        const auto name = "var" + std::to_string(i);
        const std::string source =
            "void main() { int " + name + " = " + first + "; " +
            "int result = " + name + " + " + second + "; " +
            "result = result * 2; }";
        REQUIRE_NOTHROW(analyze_source(source));
    }
}

TEST_CASE("Analyzer: deterministic randomized undeclared references", "[semantic][integration][randomized][negative]") {
    std::mt19937 rng(20261116);
    for (int i = 0; i < 100; ++i) {
        const auto value = std::to_string(rng() % 100000);
        const auto missing_name = "missing" + std::to_string(i);
        const std::string source =
            "void main() { int x = " + value + "; int y = x + " + missing_name + "; }";
        REQUIRE_THROWS_AS(analyze_source(source), vm::semantic_error);
    }
}
