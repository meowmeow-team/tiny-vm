#pragma once

#include <stack>

#include "../../core/source_manager.hpp"
#include "../../parser/include/ast.hpp"
#include "symbol_table.hpp"

class Analyzer {
   public:
    Analyzer(SourceManager& source_manager);

    void process_unit(AST::UnitDecl* unit);

   private:
    SourceManager& source_manager;
    SymbolTable    global_st;

    std::string cur_var_name;
    bool        is_var_decl;

   private:
    void process_func(AST::FunctionDecl* func);
    void process_param(AST::ParamDecl* param, SymbolTable& table);
    void process_body(AST::CompoundStmt* stmt, SymbolTable& table);

    void process_var_decl(AST::VariableDecl* decl, SymbolTable& table);
    void process_ref_var_decl(AST::RefVarDecl* decl, SymbolTable& table);
    void process_assign_decl(AST::AssignStmt* stmt, SymbolTable& table);
    void process_expression(AST::Node* node, SymbolTable& table,
                            SymbolTable::Type type);

    void                     remb_funcs(AST::UnitDecl* unit);
    const SymbolTable::Meta& find_ref_meta(const Token& name,
                                           SymbolTable& table) const;
};
