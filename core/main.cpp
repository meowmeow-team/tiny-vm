#include <iostream>

#include "../lexer/include/lexer.hpp"
#include "../parser/include/parser.hpp"
#include "logger.hpp"
#include "source_manager.hpp"

int main(void) {
    const char* path = getenv("HOME");
    std::string fr = std::string(path) + "/tiny-vm/code_example";

    Logger log;
    log.enable_log_shell();

    Lexer lexer;
    auto  vec = lexer.load_file(fr);
    std::cout << "tokens size: " << vec.size() << std::endl;
    std::cout << "tokens: " << std::endl;
    for (const auto& t : vec) {
        std::cout << t << std::endl;
    }

    SourceManager src(vec);

    Parser parser(src, log);
    auto   ast = parser.parse_unit("code_example");
    ast.print(&ast);

    return 0;
}
