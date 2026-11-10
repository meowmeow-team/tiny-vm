#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#include "exception.hpp"
#include "lexer.hpp"
#include "type_holder.hpp"

struct LexerTempFile {
    std::string path;
    LexerTempFile(const std::string& name, const std::string& data)
        : path(name) {
        std::ofstream f(path);
        f << data;
    }
    ~LexerTempFile() {
        if (std::filesystem::exists(path)) {
            std::filesystem::remove(path);
        }
    }
};

static std::string generate_random_identifier(std::mt19937_64& rng,
                                              size_t           len) {
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<size_t> dist(0, sizeof(alphabet) - 2);
    std::string                           s;
    s.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(alphabet[dist(rng)]);
    }
    return s;
}

TEST_CASE("Lexer: TypeHolder randomized tests", "[lexer][type_holder]") {
    std::mt19937_64 rng(42);

    SECTION("Random pure digits are always classified as NUMBER") {
        for (int i = 0; i < 150; ++i) {
            uint64_t    val = rng();
            std::string num_str = std::to_string(val);
            CHECK(TypeHolder::find(num_str) == TokenType::NUMBER);
        }
    }

    SECTION("Random alphabetic words are classified as IDENTIFIER or KEYWORD") {
        for (int i = 0; i < 150; ++i) {
            std::string ident = generate_random_identifier(rng, (i % 15) + 2);
            TokenType   type = TypeHolder::find(ident);
            CHECK((type == TokenType::IDENTIFIER ||
                   type == TokenType::KEYWORD ||
                   type == TokenType::KEYWORD_TYPE));
        }
    }

    SECTION("Fuzzing all 256 byte characters on is_symbol and find") {
        for (int c = 0; c < 256; ++c) {
            char ch = static_cast<char>(c);
            REQUIRE_NOTHROW(TypeHolder::is_symbol(ch));
            REQUIRE_NOTHROW(TypeHolder::find(ch));
        }
    }
}

TEST_CASE("Lexer: File parsing with random combinations", "[lexer]") {
    Lexer lexer;

    SECTION("Randomized variable declarations parsing") {
        std::mt19937_64 rng(999);
        std::string     code;
        int             declarations_count = 50;

        for (int i = 0; i < declarations_count; ++i) {
            std::string var_name = "var" + generate_random_identifier(rng, 5);
            uint32_t    val = rng() % 10000;
            code += "int " + var_name + " = " + std::to_string(val) + ";\n";
        }

        LexerTempFile tf("random_vars.txt", code);
        auto          tokens = lexer.load_file(tf.path);

        REQUIRE(tokens.size() == declarations_count * 5);

        for (int i = 0; i < declarations_count; ++i) {
            size_t base = i * 5;
            CHECK(tokens[base + 0].get_type() == TokenType::KEYWORD_TYPE);
            CHECK(tokens[base + 1].get_type() == TokenType::IDENTIFIER);
            CHECK(tokens[base + 2].get_type() == TokenType::SYMBOL_ASSIGN);
            CHECK(tokens[base + 3].get_type() == TokenType::NUMBER);
            CHECK(tokens[base + 4].get_type() == TokenType::SEMICOLON);
        }
    }

    SECTION("Comments and operators / and * parsing") {
        std::string code =
            "int a = 1; // standard comment to end of line\n"
            "int b = 2 * 3; // expression with multiplication\n"
            "int c = 4 / 2;\n";

        LexerTempFile tf("comments.txt", code);
        auto          tokens = lexer.load_file(tf.path);

        REQUIRE(tokens.size() == 19);

        CHECK(tokens[4].get_lexeme() == ";");

        CHECK(tokens[5].get_lexeme() == "int");
        CHECK(tokens[8].get_lexeme() == "2");
        CHECK(tokens[9].get_type() == TokenType::OPERATOR);
        CHECK(tokens[9].get_lexeme() == "*");
        CHECK(tokens[11].get_lexeme() == ";");

        CHECK(tokens[12].get_lexeme() == "int");
        CHECK(tokens[15].get_lexeme() == "4");
        CHECK(tokens[16].get_type() == TokenType::OPERATOR);
        CHECK(tokens[16].get_lexeme() == "/");
        CHECK(tokens[18].get_lexeme() == ";");
    }

    SECTION("Random whitespace padding stability") {
        std::mt19937_64                    rng(777);
        std::uniform_int_distribution<int> spaces(1, 10);

        std::string code;
        for (int i = 0; i < 20; ++i) {
            code += std::string(spaces(rng), ' ') + "int" +
                    std::string(spaces(rng), '\t') + "x" + std::to_string(i) +
                    std::string(spaces(rng), ' ') + ";" + "\n";
        }

        LexerTempFile tf("spaces.txt", code);
        auto          tokens = lexer.load_file(tf.path);

        REQUIRE(tokens.size() == 60);
    }

    SECTION("Tokenizing error on invalid random symbols") {
        std::vector<std::string> invalid_symbols = {"@", "$", "#",
                                                    "`", "~", "§"};
        for (const auto& sym : invalid_symbols) {
            std::string   code = "int x = " + sym + ";";
            LexerTempFile tf("invalid.txt", code);
            REQUIRE_THROWS_AS(lexer.load_file(tf.path), vm::tokenizing_error);
        }
    }
}
