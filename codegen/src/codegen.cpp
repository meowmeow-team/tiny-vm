#include "../include/codegen.hpp"

#include "../../core/exception.hpp"

CodeGen::CodeGen(SourceManager& source_manager)
    : source_manager(source_manager), cur_is_func(false) {}

Chunk CodeGen::gen(AST::Node* node) {
    if (!node) throw vm::codegen_error("failed to gen: node is nullptr");

    cur_is_func = false;

    Chunk chunk{};
    gen_generic(node, chunk);

    chunk.byte_code.push_ins(Opcode::HALT);

    return chunk;
}

void CodeGen::gen_generic(AST::Node* node, Chunk& chunk) {
    if (!node) throw vm::codegen_error("failed to gen: node is nullptr");

    switch (node->type) {
        case AST::UNIT_DECL:
            gen_unit(static_cast<AST::UnitDecl*>(node), chunk);
            break;
        case AST::FUNC_DECL:
            gen_func(static_cast<AST::FunctionDecl*>(node), chunk);
            break;
        case AST::VARIABLE_DECL:
            gen_var_decl(static_cast<AST::VariableDecl*>(node), chunk);
            break;
        case AST::ASSIGN_STMT:
            gen_assign_decl(static_cast<AST::AssignStmt*>(node), chunk);
            break;
        default:
            throw vm::codegen_error("failed to gen: unknown type of node");
    }
}

void CodeGen::gen_unit(AST::UnitDecl* unit, Chunk& chunk) {
    for (const auto& kid : unit->kids) {
        if (!kid) continue;
        gen_generic(kid.get(), chunk);
    }
}

void CodeGen::gen_func(AST::FunctionDecl* func, Chunk& chunk) {
    cur_is_func = true;
}

void CodeGen::gen_var_decl(AST::VariableDecl* decl, Chunk& chunk) {
    const auto& name = source_manager.get(decl->ref_idx);
    const auto& type = source_manager.get(decl->ref_type);

    if (!cur_is_func) {
        std::pair<std::string, std::any> p = {name.get_lexeme(),
                                              convert_lexeme(type)};
        chunk.pool.push_back(std::move(p));
    }
}

void CodeGen::gen_assign_decl(AST::AssignStmt* stmt, Chunk& chunk) {}

std::any CodeGen::convert_lexeme(const Token& token) {
    const auto& lex = token.get_lexeme();
    if (lex == "int") {
        return std::stoi(token.get_lexeme());
    } else if (lex == "char") {
        return token.get_lexeme()[0];
    } else
        throw vm::codegen_error("failed to converty lexeme to type");
}
