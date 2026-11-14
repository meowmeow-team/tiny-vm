#pragma once

#include <stack>

#include "../../core/source_manager.hpp"
#include "../../parser/include/ast.hpp"
#include "symbol_table.hpp"

class Analyzer {
   public:
    Analyzer(SourceManager& source_manager);

    void process(AST::Node* node);

   private:
    SourceManager& source_manager;
    SymbolTable    symstable;

    std::stack<AST::Node*> stack;

    bool is_func;
    bool is_assign;

   private:
    void process_func(AST::Node* cur);
    void process_param(AST::Node* cur);
    void process_assign(AST::Node* cur);
};
