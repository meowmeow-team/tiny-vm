#pragma once

#include "../../core/logger.hpp"
#include "../../core/source_manager.hpp"
#include "ast.hpp"

class Parser {
   public:
    Parser(SourceManager& source_manager, Logger& logger);

    AST::UnitDecl parse_unit(const std::string& file_name);

   private:
    SourceManager& source_manager;
    Logger&        logger;

    const uint8_t MIN_FUNC_DECL =
        6;  // minimum of tokens for function declaration
            // <return-type> <name>(<param-type> <param-name>...)

   private:
    std::unique_ptr<AST::FunctionDecl> parse_func(uint32_t& idx);
    void parse_func_def(uint32_t& idx, AST::FunctionDecl& func);
    std::vector<std::unique_ptr<AST::ParamDecl>> parse_func_params(
        uint32_t& idx);
    void parse_func_body(uint32_t& idx, AST::FunctionDecl& func);
    void parse_func_decl(uint32_t& idx, AST::CompoundStmt& cstmt);
    void parse_func_assign(uint32_t& idx, AST::CompoundStmt& cstmt);
    std::unique_ptr<AST::Node> parse_expression(uint32_t& idx);
};
