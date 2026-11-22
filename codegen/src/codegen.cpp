#include "../include/codegen.hpp"

#include "../../core/exception.hpp"

CodeGen::CodeGen(SourceManager& source_manager)
    : source_manager(source_manager), cur_is_func(false), cur_meta{} {}

Chunk CodeGen::gen(AST::Node* node) {
    if (!node) throw vm::codegen_error("failed to gen: node is nullptr");

    cur_is_func = false;
    cur_meta = {};

    Chunk chunk{};
    gen_generic(node);

    chunk.byte_code.push_ins(Opcode::HALT);

    return chunk;
}

void CodeGen::gen_generic(AST::Node* node) {
    if (!node) throw vm::codegen_error("failed to gen: node is nullptr");

    switch (node->type) {
        case AST::UNIT_DECL:
            gen_unit(static_cast<AST::UnitDecl*>(node));
            break;
        case AST::FUNC_DECL:
            gen_func(static_cast<AST::FunctionDecl*>(node));
            break;
        case AST::PARAM_DECL:
            gen_param(static_cast<AST::ParamDecl*>(node));
            break;
        case AST::VARIABLE_DECL:
            gen_var_decl(static_cast<AST::VariableDecl*>(node));
            break;
        case AST::ASSIGN_STMT:
            gen_assign_decl(static_cast<AST::AssignStmt*>(node));
            break;
        default:
            throw vm::codegen_error("failed to gen: unknown type of node");
    }
}

void CodeGen::gen_unit(AST::UnitDecl* unit) {
    for (const auto& kid : unit->kids) {
        if (!kid) continue;
        gen_generic(kid.get());
    }
}

void CodeGen::gen_func(AST::FunctionDecl* func) {
    cur_is_func = true;
    cur_meta = {};

    for (const auto& kiddo : func->kids) {
        if (!kiddo) continue;

        gen_generic(kiddo.get());
    }

    cur_is_func = false;
}

void CodeGen::gen_param(AST::ParamDecl* param) {
    if (!cur_is_func) throw vm::codegen_error("how is it even possible");

    const auto& name = source_manager.get(param->ref_idx);
    const auto& type = source_manager.get(param->ref_type);

    push_scope_meta(name.get_lexeme(), type.get_lexeme());
}

void CodeGen::gen_var_decl(AST::VariableDecl* decl) {
    const auto& name = source_manager.get(decl->ref_idx);
    const auto& type = source_manager.get(decl->ref_type);

    if (!cur_is_func) {
        chunk.push_gl_var(name.get_lexeme(), convert_lexeme(type, nullptr));
    } else {
        push_scope_meta(name.get_lexeme(), type.get_lexeme());
    }
}

void CodeGen::gen_assign_decl(AST::AssignStmt* stmt) {
    const auto& lhs = stmt->kids[0];
    const auto& rhs = stmt->kids[1];

    VarMeta var_meta{};
    if (lhs->type == AST::VARIABLE_DECL) {
        gen_var_decl(static_cast<AST::VariableDecl*>(lhs.get()));
        var_meta = {VarMeta::LOCAL, VarMeta::INT,
                    static_cast<uint8_t>(cur_meta.next_reg - 1)};
    } else {
        auto        ref = static_cast<AST::RefVarDecl*>(lhs.get());
        const auto& name = source_manager.get(ref->ref_idx);
    }

    if (cur_is_func) {
    } else {
    }
}

uint8_t CodeGen::claim_reg() {
    if (cur_meta.next_reg >= 32) throw vm::codegen_error("too much registers");
    return cur_meta.next_reg++;
}

void CodeGen::push_scope_meta(const std::string& name,
                              const std::string& type) {
    uint8_t reg = claim_reg();

    cur_meta.src.emplace(name, reg);
    cur_meta.types.emplace(reg, VarMeta::what_type(type));
}

std::any CodeGen::convert_lexeme(const Token& type, Token* value) {
    const auto& t = type.get_lexeme();
    if (t == "int") {
        return value ? std::stoi(value->get_lexeme()) : 0;
    } else if (t == "char") {
        return value ? std::stoi(value->get_lexeme()) : 0;
    } else
        throw vm::codegen_error("failed to converty lexeme to type");
}
