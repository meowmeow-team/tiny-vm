#include "../include/codegen.hpp"

#include "../../core/exception.hpp"

CodeGen::CodeGen(SourceManager& source_manager)
    : source_manager(source_manager), cur_is_func(false), cur_meta{} {}

Chunk CodeGen::gen(AST::Node* node) {
    if (!node) throw vm::codegen_error("failed to gen: node is nullptr");

    cur_is_func = false;
    cur_meta = {};
    chunk = {};

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
        case AST::COMPOUND_STMT:
            gen_compound(static_cast<AST::CompoundStmt*>(node));
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
        case AST::RET_STMT:
            gen_ret_stmt(static_cast<AST::RetStmt*>(node));
            break;
        default:
            std::cout << "LOOOL: " << node->type << std::endl;
            throw vm::codegen_error("failed to gen: unknown type of node");
    }
}

void CodeGen::gen_unit(AST::UnitDecl* unit) {
    for (const auto& kid : unit->kids) {
        if (!kid) continue;
        gen_generic(kid.get());
    }
}

void CodeGen::gen_compound(AST::CompoundStmt* stmt) {
    for (const auto& kid : stmt->kids) {
        if (!kid) continue;
        gen_generic(kid.get());
    }
}

void CodeGen::gen_func(AST::FunctionDecl* func) {
    cur_is_func = true;
    cur_meta = {};

    const auto& name = source_manager.get(func->ref_idx);
    chunk.byte_code.push_func(name.get_lexeme());

    for (const auto& kiddo : func->kids) {
        if (!kiddo) continue;

        gen_generic(kiddo.get());
    }

    auto opt = chunk.byte_code.get_last_ins();
    if (opt && opt->opcode != Opcode::RET) {
        chunk.byte_code.push_ins(Opcode::RET);
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

    VarMeta meta;
    if (lhs->type == AST::VARIABLE_DECL) {
        auto decl = static_cast<AST::VariableDecl*>(lhs.get());
        gen_var_decl(decl);

        const auto& name = source_manager.get(decl->ref_idx);
        meta = find_var_meta(name.get_lexeme());
    } else {
        auto decl = static_cast<AST::RefVarDecl*>(lhs.get());

        const auto& name = source_manager.get(decl->ref_idx);
        meta = find_var_meta(name.get_lexeme());
    }

    gen_expression(rhs.get(), meta);
}

void CodeGen::gen_ret_stmt(AST::RetStmt* stmt) {
    const auto& kid = stmt->kids[0];

    const uint8_t reg = 0;
    if (kid->type == AST::INT_LITERAL_DECL) {
        const auto& val = source_manager.get(kid->ref_idx);

        chunk.byte_code.push_ins(
            Opcode::LOAD_CONST, reg,
            static_cast<uint8_t>(std::stoi(val.get_lexeme())));
        chunk.byte_code.push_ins(Opcode::RET, reg);
    } else if (kid->type == AST::REF_VAR_DECL) {
        const auto& name = source_manager.get(kid->ref_idx);
        VarMeta     meta = find_var_meta(name.get_lexeme());
        if (meta.get_loc() == VarMeta::GLOBAL) {
            chunk.byte_code.push_ins(Opcode::MOV, reg, meta.get_idx());
        } else {
            // local
            if (meta.get_idx() != reg)
                chunk.byte_code.push_ins(Opcode::MOV, reg, meta.get_idx());
        }

        chunk.byte_code.push_ins(Opcode::RET, meta.get_idx());
    }
}

void CodeGen::gen_expression(AST::Node* stmt, VarMeta& var_meta) {
    if (stmt->type == AST::INT_LITERAL_DECL) {
        auto lit = static_cast<AST::IntLiteralDecl*>(stmt);

        const auto& val = source_manager.get(lit->ref_idx);
        chunk.byte_code.push_ins(
            Opcode::LOAD_CONST, var_meta.get_idx(),
            static_cast<uint8_t>(std::stoi(val.get_lexeme())));
    } else if (stmt->type == AST::REF_VAR_DECL) {
        auto decl = static_cast<AST::RefVarDecl*>(stmt);

        const auto& name = source_manager.get(decl->ref_idx);
        if (var_meta.get_loc() == VarMeta::GLOBAL) {
        } else {
            // local
            // TODO
        }
    } else if (stmt->type == AST::BINARY_OPERATION) {
        auto op = static_cast<AST::BinaryOperation*>(stmt);
        // TODO
    }
}

uint8_t CodeGen::claim_reg() {
    if (cur_meta.next_reg >= 32) throw vm::codegen_error("too much registers");
    return cur_meta.next_reg++;
}

void CodeGen::push_scope_meta(const std::string& name,
                              const std::string& type) {
    uint8_t reg = claim_reg();

    std::cout << "fuck: " << name << std::endl;
    cur_meta.src.emplace(name, reg);
    cur_meta.types.emplace(reg, VarMeta::what_type(type));
}

VarMeta CodeGen::find_var_meta(const std::string& name) {
    auto it = cur_meta.src.find(name);
    if (it == cur_meta.src.end()) {
        auto gl = chunk.find_gl_idx(name);
        auto any = chunk.get_gl(gl);
        return {VarMeta::GLOBAL, VarMeta::what_type(any.type()),
                static_cast<uint8_t>(gl)};
    }

    auto type = cur_meta.types.find(it->second);

    return VarMeta(VarMeta::LOCAL, type->second, it->second);
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
