#pragma once

#include "../../core/exception.hpp"
#include "../../core/source_manager.hpp"
#include "../../parser/include/ast.hpp"
#include "chunk.hpp"

class CodeGen {
   public:
    struct ScopeMeta {
        std::unordered_map<std::string, uint8_t> src;
        uint8_t                                  next_reg = 0;
    };

    struct VarMeta {
        enum Loc {
            GLOBAL,
            LOCAL,
        };

        Loc     loc;
        uint8_t idx;
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
    void gen_func(AST::FunctionDecl* func);
    void gen_param(AST::ParamDecl* param);
    void gen_var_decl(AST::VariableDecl* decl);
    void gen_assign_decl(AST::AssignStmt* stmt);

    VarMeta find_var(const std::string& name) const;

   private:
    static std::any convert_lexeme(const Token& type, Token* value);
};
