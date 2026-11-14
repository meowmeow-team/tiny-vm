#include "../include/analyzer.hpp"

#include <stack>

#include "../../core/exception.hpp"

Analyzer::Analyzer(SourceManager& source_manager)
    : source_manager(source_manager), is_func(false), is_assign(false) {}

/*
 * example:
 * int lol = 67;
 *
 * int main(int a){
 *      int b = a + 10;
 * }
 * "
 */
void Analyzer::process(AST::Node* node) {
    if (!node) throw vm::semantic_error("node is nullptr");

    // DFS
    stack.push(node);
    while (!stack.empty()) {
        AST::Node* cur = stack.top();
        stack.pop();

        for (int i = cur->kids.size() - 1; i >= 0; i--) {
            stack.push(cur->kids[i].get());
        }

        if (is_func) {
            process_func(cur);
        } else {
            if (cur->type == AST::NodeType::FUNC_DECL) {
                is_func = true;

                AST::FunctionDecl* func = static_cast<AST::FunctionDecl*>(cur);

                const auto& name = source_manager.get(func->ref_idx);
                const auto& type = source_manager.get(func->ref_type);

                auto meta =
                    symstable.make_meta(type, SymbolTable::Group::FUNCTION);
                if (symstable.contains(name.get_lexeme(), meta))
                    throw vm::semantic_error(
                        "already contains identical func name");
                symstable.push(name.get_lexeme(), std::move(meta));
                continue;
            }
        }
    }
}

void Analyzer::process_func(AST::Node* cur) {
    switch (cur->type) {
        case AST::NodeType::PARAM_DECL:
            process_param(cur);
            break;
        case AST::NodeType::COMPOUND_STMT:
            break;
        case AST::NodeType::ASSIGN_STMT:
            break;
        default:
            throw vm::semantic_error("failed to define node type");
    }
}

void Analyzer::process_param(AST::Node* cur) {
    AST::ParamDecl* param = static_cast<AST::ParamDecl*>(cur);

    const auto& name = source_manager.get(param->ref_idx);
    const auto& type = source_manager.get(param->ref_type);

    auto meta =
        SymbolTable::make_meta(type, SymbolTable::Group::FUNCTION_PARAM);

    if (symstable.contains(name.get_lexeme(), meta))
        throw vm::semantic_error("already contains identical param");

    symstable.push(name.get_lexeme(), std::move(meta));
}

void Analyzer::process_assign(AST::Node* cur) {
    AST::AssignStmt* agn = static_cast<AST::AssignStmt*>(cur);
}
