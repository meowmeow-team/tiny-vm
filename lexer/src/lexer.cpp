#include "../include/lexer.hpp"

#include <fstream>

Lexer::Lexer() : holder(), files_lexed(0) {}

std::vector<Token> Lexer::load_file(const std::string& path) {
    if (path.empty()) return {};

    std::ifstream file(path);
    if (!file.is_open()) return {};

    std::vector<Token> res{};

    size_t   row = 0;
    uint32_t abs = 0;

    std::string line;
    while (std::getline(file, line)) {
        row++;
        size_t pos = 0;

        while (pos < line.size()) {
            if (std::isspace(line[pos])) {
                pos++;
                continue;
            }

            if (line[pos] == '/' && pos + 1 < line.size())
                if (line[pos + 1] == '/') break;

            if (holder.is_symbol(line[pos]) || line[pos] == '.') {
                res.push_back({holder.find(line[pos]),
                               {abs, static_cast<uint16_t>(row),
                                static_cast<uint32_t>(pos + 1)},
                               std::string(1, line[pos])});
                pos++;
                continue;
            }

            size_t start = pos;
            while (pos < line.size()) {
                if (line[pos] == '.') break;
                if (std::isspace(line[pos]) || holder.is_symbol(line[pos]))
                    break;

                pos++;
            }

            std::string word = line.substr(start, pos - start);
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
