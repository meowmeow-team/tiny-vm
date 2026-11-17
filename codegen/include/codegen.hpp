#pragma once

#include "../../core/exception.hpp"
#include "../../core/source_manager.hpp"
#include "../../parser/include/ast.hpp"
#include "chunk.hpp"

class CodeGen {
   public:
    struct VarMeta {
        std::unordered_map<std::string, uint8_t> src;
        uint8_t                                  next_reg = 0;
    };

   public:
    CodeGen(SourceManager& source_manager);

    Chunk gen(AST::Node* node);

   private:
    SourceManager& source_manager;

    bool cur_is_func;

   private:
    void gen_generic(AST::Node* node, Chunk& chunk);

    void gen_unit(AST::UnitDecl* unit, Chunk& chunk);
    void gen_func(AST::FunctionDecl* func, Chunk& chunk);
    void gen_var_decl(AST::VariableDecl* decl, Chunk& chunk);
    void gen_assign_decl(AST::AssignStmt* stmt, Chunk& chunk);

   private:
    static std::any convert_lexeme(const Token& token);
};
