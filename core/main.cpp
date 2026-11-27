#include <iostream>

#include "../codegen/include/codegen.hpp"
#include "../lexer/include/lexer.hpp"
#include "../parser/include/parser.hpp"
#include "../semantic_analysis/include/analyzer.hpp"
#include "../vm/include/vm.hpp"
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

    Analyzer analyzer(src);
    analyzer.process_unit(&ast);

    CodeGen codegen(src);
    auto    chunk = codegen.gen(&ast);
    std::cout << chunk << std::endl;

    VM vm;
    vm.load(std::move(chunk));
    vm.enable_logging(&std::cout);

    vm.run();

    return 0;
}
