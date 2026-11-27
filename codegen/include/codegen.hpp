#pragma once

#include "../../core/exception.hpp"
#include "../../core/source_manager.hpp"
#include "../../parser/include/ast.hpp"
#include "chunk.hpp"
#include "var_meta.hpp"

class CodeGen {
   public:
    struct ScopeMeta {
        std::unordered_map<std::string, uint8_t>   src;
        std::unordered_map<uint8_t, VarMeta::Type> types;
        uint8_t                                    next_reg = 0;
    };

   public:
    CodeGen(SourceManager& source_manager);

    Chunk gen(AST::Node* node);

   private:
    SourceManager& source_manager;

    bool      cur_is_func;
    ScopeMeta cur_meta;
    Chunk     chunk;

   private:
    void gen_generic(AST::Node* node);

    void gen_unit(AST::UnitDecl* unit);
    void gen_compound(AST::CompoundStmt* stmt);
    void gen_func(AST::FunctionDecl* func);
    void gen_param(AST::ParamDecl* param);
    void gen_var_decl(AST::VariableDecl* decl);
    void gen_assign_decl(AST::AssignStmt* stmt);
    void gen_ret_stmt(AST::RetStmt* stmt);

    void gen_expression(AST::Node* stmt, VarMeta& var_meta);

    uint8_t claim_reg();

    void push_scope_meta(const std::string& name, const std::string& type);

    VarMeta find_var_meta(const std::string& name);

   private:
    static std::any convert_lexeme(const Token& type, Token* value);
};
