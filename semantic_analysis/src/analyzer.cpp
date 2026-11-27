#include "../include/analyzer.hpp"

#include <istream>

#include "../../core/exception.hpp"

Analyzer::Analyzer(SourceManager& source_manager)
    : source_manager(source_manager),
      global_st(true),
      cur_var_name(),
      is_var_decl(false),
      cur_func_name() {}

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

        if (kid->type == AST::NodeType::RET_STMT) {
            throw vm::semantic_error("return operation outside the function");
        }
    }
}

void Analyzer::process_func(AST::FunctionDecl* func) {
    SymbolTable local(false);  // local symbol table for only that func

    cur_func_name = source_manager.get(func->ref_idx).get_lexeme();

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
            case AST::RET_STMT:
                process_ret_stmt(static_cast<AST::RetStmt*>(kid.get()), table);
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
    cur_var_name = name.get_lexeme();
    is_var_decl = true;
}

void Analyzer::process_ref_var_decl(AST::RefVarDecl* decl, SymbolTable& table) {
    const auto& name = source_manager.get(decl->ref_idx);

    if (table.contains(name.get_lexeme(), SymbolTable::Group::VARIABLE)) {
        cur_var_name = name.get_lexeme();
        return;
    }

    if (!table.is_global()) {
        if (global_st.contains(name.get_lexeme(),
                               SymbolTable::Group::VARIABLE)) {
            cur_var_name = name.get_lexeme();
            return;
        }
    }

    throw vm::semantic_error(
        "failed to process ref var decl: unknown reference");
}

void Analyzer::process_assign_decl(AST::AssignStmt* stmt, SymbolTable& table) {
    if (stmt->kids.size() != 2)
        throw vm::semantic_error(
            "failed to process assign: invalid size of kids");

    const auto&       lhs = stmt->kids[0];
    SymbolTable::Type lhs_type{};

    if (lhs->type == AST::VARIABLE_DECL) {
        AST::VariableDecl* decl = static_cast<AST::VariableDecl*>(lhs.get());
        process_var_decl(decl, table);

        const auto& type = source_manager.get(decl->ref_type);
        lhs_type = SymbolTable::make_type(type);
    } else if (lhs->type == AST::REF_VAR_DECL) {
        AST::RefVarDecl* decl = static_cast<AST::RefVarDecl*>(lhs.get());
        process_ref_var_decl(decl, table);

        const auto& name = source_manager.get(decl->ref_idx);

        auto meta = find_ref_meta(name, table);
        lhs_type = meta.type;

    } else
        throw vm::semantic_error("failed to process assign: invalid lhs");

    const auto& rhs = stmt->kids[1];
    process_expression(rhs.get(), table, lhs_type);

    is_var_decl = false;
}

void Analyzer::process_expression(AST::Node* node, SymbolTable& table,
                                  SymbolTable::Type type) {
    switch (node->type) {
        case AST::INT_LITERAL_DECL:
            if (type != SymbolTable::Type::INT)
                throw vm::semantic_error(
                    "failed to process expression: wrong type of expression");
            break;
        case AST::REF_VAR_DECL: {
            AST::RefVarDecl* ref = static_cast<AST::RefVarDecl*>(node);

            const auto& name = source_manager.get(ref->ref_idx);

            if (is_var_decl && name.get_lexeme() == cur_var_name)
                throw vm::semantic_error(
                    "failed to process expression: wrong type of expression");

            SymbolTable::Meta meta = find_ref_meta(name, table);
            if (meta.type == type && meta.group == SymbolTable::Group::VARIABLE)
                break;

            throw vm::semantic_error("failed to process expression: wrong ref");
            break;
        }
        case AST::BINARY_OPERATION:
            if (node->kids.size() != 2)
                throw vm::semantic_error(
                    "failed to process expression: invalid binary op");

            process_expression(node->kids[0].get(), table, type);  // lhs
            process_expression(node->kids[1].get(), table, type);  // rhs
            break;
        default:
            throw vm::semantic_error(
                "failed to process expression: unknown node type");
    }
}

void Analyzer::process_ret_stmt(AST::RetStmt* stmt, SymbolTable& table) {
    if (stmt->kids.size() != 1)
        throw vm::semantic_error(
            "failed to process return stmt: invalid size of kids");

    const auto& kid = stmt->kids[0];
    const auto& name = source_manager.get(kid->ref_idx);

    const auto& meta = find_func_meta(std::string(cur_func_name));

    if (kid->type == AST::INT_LITERAL_DECL) {
        if (meta.type != SymbolTable::Type::INT)
            throw vm::semantic_error(
                "failed to process return stmt: invalid return value type");
    } else if (kid->type == AST::REF_VAR_DECL) {
        const auto& var_meta =
            find_ref_meta(source_manager.get(kid->ref_idx), table);
        if (var_meta.type != meta.type)
            throw vm::semantic_error(
                "failed to proccess return stmt: invalid return value type");
    } else {
        throw vm::semantic_error(
            "failed to process return stmt: unknown type of return value");
    }
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

const SymbolTable::Meta& Analyzer::find_ref_meta(const Token& name,
                                                 SymbolTable& table) const {
    if (!table.contains(name.get_lexeme(), SymbolTable::Group::VARIABLE)) {
        if (!global_st.contains(name.get_lexeme(),
                                SymbolTable::Group::VARIABLE))
            throw vm::semantic_error(
                "failed to find ref var meta: there is no such var");

        return global_st.get(name.get_lexeme());
    }

    return table.get(name.get_lexeme());
}

const SymbolTable::Meta& Analyzer::find_func_meta(
    const std::string& name) const {
    if (!global_st.contains(name, SymbolTable::Group::FUNCTION))
        throw vm::semantic_error("failed to find func meta");

    return global_st.get(name);
}
