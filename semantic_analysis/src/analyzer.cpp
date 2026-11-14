#include "../include/analyzer.hpp"

#include <istream>

#include "../../core/exception.hpp"

Analyzer::Analyzer(SourceManager& source_manager)
    : source_manager(source_manager), global_st(true) {}

/*
 * example:
 * int lol = 67;
 *
 * int main(int a){
 *      int b = a + 10;
 * }
 * "
 */

void Analyzer::process_unit(AST::UnitDecl* unit) {
    if (!unit) throw vm::semantic_error("ast unit is nullptr");

    remb_funcs(unit);  // remembering function names for
                       // future possible calling, etc

    for (const auto& kid : unit->kids) {
        if (!kid) throw vm::semantic_error("processing unit: kid is nullptr");

        if (kid->type == AST::NodeType::FUNC_DECL) {
            process_func(static_cast<AST::FunctionDecl*>(kid.get()));
            continue;
        }

        if (kid->type == AST::NodeType::VARIABLE_DECL) {
            process_var_decl(static_cast<AST::VariableDecl*>(kid.get()),
                             global_st);
            continue;
        }

        if (kid->type == AST::NodeType::ASSIGN_STMT) {
            process_assign_decl(static_cast<AST::AssignStmt*>(kid.get()),
                                global_st);
            continue;
        }
    }
}

void Analyzer::process_func(AST::FunctionDecl* func) {
    SymbolTable local(false);  // local symbol table for only that func

    for (const auto& kid : func->kids) {
        if (!kid)
            throw vm::semantic_error("failed to process func: kid is nullptr");
        if (kid->type == AST::NodeType::PARAM_DECL) {
            process_param(static_cast<AST::ParamDecl*>(kid.get()), local);
            continue;
        }

        if (kid->type == AST::NodeType::COMPOUND_STMT) {
            process_body(static_cast<AST::CompoundStmt*>(kid.get()), local);
            break;
        }

        throw vm::semantic_error("failed to process func: unknown kid");
    }
}

void Analyzer::process_param(AST::ParamDecl* param, SymbolTable& table) {
    const auto& name = source_manager.get(param->ref_idx);
    const auto& type = source_manager.get(param->ref_type);

    auto meta = SymbolTable::make_meta(type, SymbolTable::Group::VARIABLE);

    if (table.contains(name.get_lexeme()))
        throw vm::semantic_error("local table already contains that name");

    table.push(name.get_lexeme(), meta);
}

void Analyzer::process_body(AST::CompoundStmt* stmt, SymbolTable& table) {
    for (const auto& kid : stmt->kids) {
        switch (kid->type) {
            case AST::VARIABLE_DECL:
                process_var_decl(static_cast<AST::VariableDecl*>(kid.get()),
                                 table);
                break;
            case AST::ASSIGN_STMT:
                process_assign_decl(static_cast<AST::AssignStmt*>(kid.get()),
                                    table);
                break;
            default:
                throw vm::semantic_error(
                    "failed to process body: unknown type");
        }
    }
}

void Analyzer::process_var_decl(AST::VariableDecl* decl, SymbolTable& table) {
    const auto& name = source_manager.get(decl->ref_idx);
    const auto& type = source_manager.get(decl->ref_type);

    if (table.contains(name.get_lexeme()))
        throw vm::semantic_error(
            "failed to process var decl: name already exists");

    if (!table.is_global()) {
        if (global_st.contains(name.get_lexeme(), SymbolTable::Group::VARIABLE))
            throw vm::semantic_error(
                "failed to process var decl: global var with that name already "
                "exists");
    }

    auto meta = SymbolTable::make_meta(type, SymbolTable::Group::VARIABLE);

    table.push(name.get_lexeme(), meta);
}

void Analyzer::process_ref_var_decl(AST::RefVarDecl* decl, SymbolTable& table) {
    const auto& name = source_manager.get(decl->ref_idx);

    if (table.contains(name.get_lexeme(), SymbolTable::Group::VARIABLE)) return;

    if (!table.is_global()) {
        if (global_st.contains(name.get_lexeme(), SymbolTable::Group::VARIABLE))
            return;
    }

    throw vm::semantic_error(
        "failed to process ref var decl: unknown reference");
}

void Analyzer::process_assign_decl(AST::AssignStmt* stmt, SymbolTable& table) {
    if (stmt->kids.size() != 2)
        throw vm::semantic_error(
            "failed to process assign: invalid size of kids");

    const auto& lhs = stmt->kids[0];
    if (lhs->type == AST::VARIABLE_DECL) {
        process_var_decl(static_cast<AST::VariableDecl*>(lhs.get()), table);
    } else if (lhs->type == AST::REF_VAR_DECL) {
        process_ref_var_decl(static_cast<AST::RefVarDecl*>(lhs.get()), table);
    } else
        throw vm::semantic_error("failed to process assign: invalid lhs");

    const auto& rhs = stmt->kids[1];
    // TODO analyze expresison
}

void Analyzer::remb_funcs(AST::UnitDecl* unit) {
    for (const auto& kid : unit->kids) {
        if (!kid) throw vm::semantic_error("register funcs: kid is nullptr");
        if (kid->type != AST::NodeType::FUNC_DECL) continue;

        AST::FunctionDecl* func = static_cast<AST::FunctionDecl*>(kid.get());

        const auto& name = source_manager.get(func->ref_idx);
        const auto& type = source_manager.get(func->ref_type);

        auto meta = SymbolTable::make_meta(type, SymbolTable::Group::FUNCTION);

        if (global_st.contains(name.get_lexeme()))
            throw vm::semantic_error("register funcs: name already exists");

        global_st.push(name.get_lexeme(), meta);
    }
}
