// Randomized regression tests for tiny-vm as represented by output1.txt (v0.4.5).
// Catch2 v3.5.2; compatible with the project's current supported syntax.
// Reproducible runs: TINYVM_RANDOM_SEED=1234 TINYVM_RANDOM_CASES=200 ./run_random_tests.sh
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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

namespace random_tests {

uint64_t random_seed() {
    constexpr uint64_t default_seed = 0xC0FFEE123456789ULL;
    const char* text = std::getenv("TINYVM_RANDOM_SEED");
    if (!text || !*text) return default_seed;
    try {
        size_t consumed = 0;
        auto value = std::stoull(text, &consumed, 0);
        if (consumed != std::string(text).size())
            throw std::invalid_argument("invalid seed");
        return value;
    } catch (const std::exception&) {
        throw std::invalid_argument("TINYVM_RANDOM_SEED must be a non-negative integer");
    }
}

int random_cases() {
    const char* text = std::getenv("TINYVM_RANDOM_CASES");
    if (!text || !*text) return 120;
    try {
        size_t consumed = 0;
        const auto n = std::stoll(text, &consumed, 10);
        if (consumed != std::string(text).size() || n < 1 || n > 10000)
            throw std::invalid_argument("bad count");
        return static_cast<int>(n);
    } catch (const std::exception&) {
        throw std::invalid_argument("TINYVM_RANDOM_CASES must be an integer in [1, 10000]");
    }
}

std::mt19937_64 engine(uint64_t salt) {
    return std::mt19937_64(random_seed() ^ salt);
}

int rand_int(std::mt19937_64& rng, int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(rng);
}

std::string name(std::mt19937_64& rng, std::string_view prefix, int unique_index) {
    std::string result(prefix);
    for (int i = 0; i < 7; ++i)
        result += static_cast<char>('a' + rand_int(rng, 0, 25));
    result += std::to_string(unique_index);
    return result;
}

std::string whitespace(std::mt19937_64& rng) {
    return std::string(rand_int(rng, 1, 5), rand_int(rng, 0, 1) ? ' ' : '\t');
}

struct TempFile {
    std::filesystem::path path;

    explicit TempFile(std::string_view source) {
        static std::atomic<uint64_t> file_id{0};
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("tiny_vm_random_" + std::to_string(stamp) + "_" +
                std::to_string(++file_id) + ".txt");
        std::ofstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot create temporary source");
        file << source;
        if (!file) throw std::runtime_error("Cannot write temporary source");
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
};

void analyze_text(std::string_view source) {
    TempFile file(source);
    Lexer lexer;
    auto tokens = lexer.load_file(file.path.string());
    SourceManager sm(std::move(tokens));
    Logger logger;
    Parser parser(sm, logger);
    auto unit = parser.parse_unit("random_program");
    Analyzer analyzer(sm);
    analyzer.process_unit(&unit);
}

Token make_token(TokenType type, std::string_view str) {
    return Token(type, Position{0, 1, 1}, std::string(str));
}

void analyze_token_stream(std::vector<Token> tokens) {
    SourceManager sm(std::move(tokens));
    Logger logger;
    Parser parser(sm, logger);
    auto unit = parser.parse_unit("random_tokens");
    Analyzer analyzer(sm);
    analyzer.process_unit(&unit);
}

void append(std::vector<Token>& tokens, TokenType type, std::string_view value) {
    tokens.push_back(make_token(type, value));
}

} // namespace random_tests

using namespace random_tests;

TEST_CASE("Random: SymbolTable insertion, queries, and scope separation",
          "[randomized][semantic][symbol_table]") {
    auto rng = engine(0x1101);
    SymbolTable global(true);
    SymbolTable local(false);
    struct Entry { std::string id; SymbolTable::Meta meta; };
    std::vector<Entry> entries;
    const int n = random_cases();

    for (int i = 0; i < n; ++i) {
        const auto id = name(rng, "sym", i);
        const SymbolTable::Meta meta{
            static_cast<SymbolTable::Type>(rand_int(rng, 0, 2)),
            rand_int(rng, 0, 1) == 0 ? SymbolTable::VARIABLE : SymbolTable::FUNCTION};
        INFO("seed=" << random_seed() << " case=" << i << " symbol=" << id);
        REQUIRE_FALSE(global.contains(id));
        REQUIRE_NOTHROW(global.push(id, meta));
        REQUIRE(global.contains(id));
        CHECK(global.contains(id, meta));
        CHECK(global.contains(id, meta.type));
        CHECK(global.contains(id, meta.group));
        CHECK(global.get(id) == meta);
        CHECK_FALSE(local.contains(id));
        REQUIRE_THROWS_AS(global.push(id, meta), vm::symbol_table_error);
        entries.push_back({id, meta});
    }
    for (const auto& item : entries) {
        CHECK(global.contains(item.id, item.meta));
    }
}

TEST_CASE("Random: well-formed source programs pass Lexer Parser Analyzer",
          "[randomized][semantic][integration][positive]") {
    auto rng = engine(0x2202);
    for (int run = 0; run < random_cases(); ++run) {
        const int n = rand_int(rng, 1, 16);
        std::string source = "void main() {\n";
        std::vector<std::string> symbols;
        for (int i = 0; i < n; ++i) {
            auto current = name(rng, "var", i);
            if (i == 0) {
                source += "int" + whitespace(rng) + current + whitespace(rng) +
                          "=" + whitespace(rng) + std::to_string(rand_int(rng, 0, 10000)) + ";\n";
            } else {
                const auto& previous = symbols[rand_int(rng, 0, i - 1)];
                const char* op[] = {"+", "-", "*", "/"};
                const auto oper = op[rand_int(rng, 0, 3)];
                source += "int " + current + " = " + previous + " " + oper + " " +
                          std::to_string(rand_int(rng, 1, 30)) + ";\n";
                if (rand_int(rng, 0, 1))
                    source += current + " = " + current + " + 1;\n";
            }
            symbols.push_back(std::move(current));
        }
        source += "}\n";
        INFO("seed=" << random_seed() << " run=" << run << " source:\n" << source);
        REQUIRE_NOTHROW(analyze_text(source));
    }
}

TEST_CASE("Random: same local identifiers in independent functions are valid",
          "[randomized][semantic][scope][positive]") {
    auto rng = engine(0x3303);
    for (int run = 0; run < random_cases(); ++run) {
        const auto shared = name(rng, "local", 0);
        std::string source;
        for (int f = 0; f < rand_int(rng, 2, 6); ++f) {
            source += "void " + name(rng, "func", f) + "() { int " + shared +
                      " = " + std::to_string(rand_int(rng, 1, 999)) + "; }\n";
        }
        INFO("seed=" << random_seed() << " run=" << run << " source:\n" << source);
        REQUIRE_NOTHROW(analyze_text(source));
    }
}

TEST_CASE("Random: duplicate local variables are semantic errors",
          "[randomized][semantic][negative]") {
    auto rng = engine(0x4404);
    for (int run = 0; run < random_cases(); ++run) {
        const auto id = name(rng, "dup", run);
        const auto source = "void main() { int " + id + " = " +
            std::to_string(rand_int(rng, 0, 99)) + "; int " + id + " = " +
            std::to_string(rand_int(rng, 0, 99)) + "; }";
        INFO("seed=" << random_seed() << " run=" << run << " source=" << source);
        REQUIRE_THROWS_AS(analyze_text(source), vm::semantic_error);
    }
}

TEST_CASE("Random: duplicate function names are semantic errors",
          "[randomized][semantic][negative]") {
    auto rng = engine(0x5505);
    for (int run = 0; run < random_cases(); ++run) {
        const auto fun = name(rng, "dupfun", run);
        const auto source = "void " + fun + "() { int x = 1; } " +
                            "void " + fun + "() { int y = 2; }";
        INFO("seed=" << random_seed() << " run=" << run << " source=" << source);
        REQUIRE_THROWS_AS(analyze_text(source), vm::semantic_error);
    }
}

TEST_CASE("Random: unknown symbols on assignment RHS and LHS are rejected",
          "[randomized][semantic][negative]") {
    auto rng = engine(0x6606);
    for (int run = 0; run < random_cases(); ++run) {
        const auto known = name(rng, "known", run);
        const auto missing = name(rng, "missing", run);
        const bool rhs = rand_int(rng, 0, 1) != 0;
        const auto source = rhs
          ? ("void main() { int " + known + " = 1; int result = " + known +
             " + " + missing + "; }")
          : ("void main() { int " + known + " = 1; " + missing + " = 2; }");
        INFO("seed=" << random_seed() << " run=" << run << " source=" << source);
        REQUIRE_THROWS_AS(analyze_text(source), vm::semantic_error);
    }
}

TEST_CASE("Random: incompatible int and char expressions are rejected",
          "[randomized][semantic][negative][types]") {
    auto rng = engine(0x7707);
    for (int run = 0; run < random_cases(); ++run) {
        const auto param = name(rng, "letter", run);
        const auto variable = name(rng, "value", run);
        std::string source;
        switch (rand_int(rng, 0, 2)) {
            case 0:
                source = "void main(char " + param + ") { int " + variable +
                         " = " + param + "; }";
                break;
            case 1:
                source = "void main(char " + param + ") { int " + variable +
                         " = " + param + " + 1; }";
                break;
            default:
                source = "void main() { char " + variable + " = " +
                         std::to_string(rand_int(rng, 0, 127)) + "; }";
        }
        INFO("seed=" << random_seed() << " run=" << run << " source=" << source);
        REQUIRE_THROWS_AS(analyze_text(source), vm::semantic_error);
    }
}

TEST_CASE("Random: parser accepts multiple typed parameters from token streams",
          "[randomized][semantic][manual_tokens][positive]") {
    auto rng = engine(0x8808);
    for (int run = 0; run < random_cases(); ++run) {
        std::vector<Token> tokens;
        append(tokens, TokenType::KEYWORD_TYPE, "void");
        append(tokens, TokenType::IDENTIFIER, name(rng, "func", run));
        append(tokens, TokenType::SYMBOL, "(");
        const int count = rand_int(rng, 1, 7);
        std::vector<std::string> param_names;
        std::vector<std::string> param_types;
        for (int i = 0; i < count; ++i) {
            if (i > 0) append(tokens, TokenType::SYMBOL, ",");
            const std::string type = rand_int(rng, 0, 1) ? "int" : "char";
            auto id = name(rng, "arg", i);
            append(tokens, TokenType::KEYWORD_TYPE, type);
            append(tokens, TokenType::IDENTIFIER, id);
            param_types.push_back(type);
            param_names.push_back(std::move(id));
        }
        append(tokens, TokenType::SYMBOL, ")");
        append(tokens, TokenType::SYMBOL, "{");
        const int chosen = rand_int(rng, 0, count - 1);
        append(tokens, TokenType::KEYWORD_TYPE, param_types[chosen]);
        append(tokens, TokenType::IDENTIFIER, "output");
        append(tokens, TokenType::SYMBOL_ASSIGN, "=");
        append(tokens, TokenType::IDENTIFIER, param_names[chosen]);
        append(tokens, TokenType::SEMICOLON, ";");
        append(tokens, TokenType::SYMBOL, "}");
        INFO("seed=" << random_seed() << " run=" << run << " param_count=" << count);
        REQUIRE_NOTHROW(analyze_token_stream(std::move(tokens)));
    }
}

TEST_CASE("Random: repeated parameter identifiers cause semantic errors",
          "[randomized][semantic][manual_tokens][negative]") {
    auto rng = engine(0x9909);
    for (int run = 0; run < random_cases(); ++run) {
        const auto same = name(rng, "arg", run);
        std::vector<Token> tokens;
        append(tokens, TokenType::KEYWORD_TYPE, "void");
        append(tokens, TokenType::IDENTIFIER, "foo");
        append(tokens, TokenType::SYMBOL, "(");
        append(tokens, TokenType::KEYWORD_TYPE, "int");
        append(tokens, TokenType::IDENTIFIER, same);
        append(tokens, TokenType::SYMBOL, ",");
        append(tokens, TokenType::KEYWORD_TYPE, "char");
        append(tokens, TokenType::IDENTIFIER, same);
        append(tokens, TokenType::SYMBOL, ")");
        append(tokens, TokenType::SYMBOL, "{");
        append(tokens, TokenType::KEYWORD_TYPE, "int");
        append(tokens, TokenType::IDENTIFIER, "output");
        append(tokens, TokenType::SYMBOL_ASSIGN, "=");
        append(tokens, TokenType::NUMBER, "1");
        append(tokens, TokenType::SEMICOLON, ";");
        append(tokens, TokenType::SYMBOL, "}");
        INFO("seed=" << random_seed() << " run=" << run << " duplicate=" << same);
        REQUIRE_THROWS_AS(analyze_token_stream(std::move(tokens)), vm::semantic_error);
    }
}

TEST_CASE("Random: AST preserves arithmetic operator precedence",
          "[randomized][parser][ast][positive]") {
    auto rng = engine(0xA00A);
    for (int run = 0; run < random_cases(); ++run) {
        const auto a = std::to_string(rand_int(rng, 0, 200));
        const auto b = std::to_string(rand_int(rng, 0, 200));
        const auto c = std::to_string(rand_int(rng, 0, 200));
        const bool mul_first = rand_int(rng, 0, 1) != 0;
        const auto expression = mul_first ? (a + " * " + b + " + " + c)
                                          : (a + " + " + b + " * " + c);
        const auto source = "void main() { int result = " + expression + "; }";
        INFO("seed=" << random_seed() << " run=" << run << " source=" << source);
        TempFile file(source);
        Lexer lexer;
        auto tokens = lexer.load_file(file.path.string());
        SourceManager sm(std::move(tokens));
        Logger logger;
        Parser parser(sm, logger);
        auto unit = parser.parse_unit("random_ast");
        REQUIRE(unit.kids.size() == 1);
        const auto* func = dynamic_cast<AST::FunctionDecl*>(unit.kids[0].get());
        REQUIRE(func != nullptr);
        REQUIRE(func->kids.size() == 1);
        const auto* block = dynamic_cast<AST::CompoundStmt*>(func->kids[0].get());
        REQUIRE(block != nullptr);
        REQUIRE(block->kids.size() == 1);
        const auto* assignment = dynamic_cast<AST::AssignStmt*>(block->kids[0].get());
        REQUIRE(assignment != nullptr);
        REQUIRE(assignment->kids.size() == 2);
        const auto* plus = dynamic_cast<AST::BinaryOperation*>(assignment->kids[1].get());
        REQUIRE(plus != nullptr);
        REQUIRE(sm.get(plus->ref_idx).get_lexeme() == "+");
        const size_t nested = mul_first ? 0 : 1;
        REQUIRE(plus->kids.size() == 2);
        const auto* mul = dynamic_cast<AST::BinaryOperation*>(plus->kids[nested].get());
        REQUIRE(mul != nullptr);
        CHECK(sm.get(mul->ref_idx).get_lexeme() == "*");
        Analyzer analyzer(sm);
        REQUIRE_NOTHROW(analyzer.process_unit(&unit));
    }
}

TEST_CASE("Random: malformed statements and incomplete blocks are rejected",
          "[randomized][parser][negative]") {
    auto rng = engine(0xB00B);
    for (int run = 0; run < random_cases(); ++run) {
        const auto variable = name(rng, "var", run);
        const auto value = std::to_string(rand_int(rng, 0, 1000));
        std::string source;
        switch (run % 4) {
            case 0: source = "void main() { int " + variable + " = " + value + " }"; break;
            case 1: source = "void main() { int " + variable + " = " + value + ";"; break;
            case 2: source = "void main() { int " + variable + " = ; }"; break;
            default: source = "void main() { int " + variable + " " + value + "; }"; break;
        }
        INFO("seed=" << random_seed() << " run=" << run << " source=" << source);
        REQUIRE_THROWS_AS(analyze_text(source), vm::syntax_error);
    }
}

TEST_CASE("Random: source token references match AST variable declarations",
          "[randomized][parser][ast][integrity]") {
    auto rng = engine(0xC00C);
    for (int run = 0; run < random_cases(); ++run) {
        const int count = rand_int(rng, 1, 12);
        std::vector<std::string> ids;
        std::string source = "void main() {\n";
        for (int i = 0; i < count; ++i) {
            ids.push_back(name(rng, "n", i));
            source += "int " + ids.back() + " = " +
                      std::to_string(rand_int(rng, 1, 999)) + ";\n";
        }
        source += "}";
        INFO("seed=" << random_seed() << " run=" << run << " source:\n" << source);
        TempFile file(source);
        Lexer lexer;
        auto tokens = lexer.load_file(file.path.string());
        SourceManager sm(std::move(tokens));
        Logger logger;
        Parser parser(sm, logger);
        auto unit = parser.parse_unit("ref_check");
        REQUIRE(unit.kids.size() == 1);
        const auto* func = dynamic_cast<AST::FunctionDecl*>(unit.kids[0].get());
        REQUIRE(func != nullptr);
        REQUIRE(func->kids.size() == 1);
        const auto* block = dynamic_cast<AST::CompoundStmt*>(func->kids[0].get());
        REQUIRE(block != nullptr);
        REQUIRE(block->kids.size() == ids.size());
        for (size_t i = 0; i < ids.size(); ++i) {
            const auto* assign = dynamic_cast<AST::AssignStmt*>(block->kids[i].get());
            REQUIRE(assign != nullptr);
            REQUIRE(assign->kids.size() == 2);
            const auto* var = dynamic_cast<AST::VariableDecl*>(assign->kids[0].get());
            REQUIRE(var != nullptr);
            CHECK(sm.get(var->ref_idx).get_lexeme() == ids[i]);
            CHECK(sm.get(var->ref_type).get_lexeme() == "int");
            CHECK(assign->kids[1]->type == AST::INT_LITERAL_DECL);
        }
        Analyzer analyzer(sm);
        REQUIRE_NOTHROW(analyzer.process_unit(&unit));
    }
}
