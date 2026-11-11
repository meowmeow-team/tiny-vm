#include "../include/lexer.hpp"

#include <fstream>

#include "../../core/exception.hpp"

Lexer::Lexer() : holder(), files_lexed(0) {}

std::vector<Token> Lexer::load_file(const std::string& path) {
    if (path.empty()) throw vm::io_error("file path is empty");

    std::ifstream file(path);
    if (!file.is_open()) throw vm::io_error("unable to open file: " + path);

    std::vector<Token> res{};

    size_t   row = 0;
    uint32_t abs = 0;

    bool        is_str = false;
    std::string str = "";

    std::string line;
    while (std::getline(file, line)) {
        row++;
        size_t pos = 0;

        while (pos < line.size()) {
            unsigned char c = line[pos];
            if (std::isspace(c)) {
                pos++;
                abs++;
                continue;
            }

            if (c == '/' && pos + 1 < line.size() && line[pos + 1] == '/') {
                break;
            }

            if (holder.is_symbol(c)) {
                if (c == '"') is_str = !is_str;
                res.push_back({holder.find(c),
                               {abs, static_cast<uint16_t>(row),
                                static_cast<uint32_t>(pos + 1)},
                               std::string(1, line[pos])});
                pos++;
                abs++;
                continue;
            }

            size_t start = pos;
            while (pos < line.size()) {
                if (!is_str) {
                    if (line[pos] == '.') break;
                    if (std::isspace(line[pos]) || holder.is_symbol(line[pos]))
                        break;
                } else {
                    if (line[pos] == '"') {
                        str = line.substr(start, pos - start);
                        break;
                    }
                }

                pos++;
            }

            if (!str.empty()) {
                res.push_back({TokenType::STRING_LITERAL,
                               {abs, static_cast<uint16_t>(row),
                                static_cast<uint32_t>(start + 1)},
                               str});
                str.clear();
                abs += pos - start;
                continue;
            }

            std::string word = line.substr(start, pos - start);
            auto        type = holder.find(word);
            if (type == TokenType::UNKNOWN)
                throw vm::tokenizing_error("unknown lexeme: ", word.c_str());

            res.push_back({holder.find(word),
                           {abs, static_cast<uint16_t>(row),
                            static_cast<uint32_t>(start + 1)},
                           word});

            abs += pos - start;
        }

        abs++;
    }

    files_lexed++;

    return std::move(res);
}

size_t Lexer::get_files_lexed() const noexcept { return files_lexed; }
