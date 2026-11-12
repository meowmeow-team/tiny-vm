#pragma once

#include <unordered_map>

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
    static std::unordered_map<std::string, uint8_t> pratt_priorities;

   private:
    std::unique_ptr<AST::FunctionDecl> parse_func(uint32_t& idx);
    void parse_func_def(uint32_t& idx, AST::FunctionDecl& func);
    void parse_func_params(uint32_t&                                idx,
                           std::vector<std::unique_ptr<AST::Node>>& res);
    void parse_func_body(uint32_t& idx, AST::FunctionDecl& func);
    void parse_func_decl(uint32_t& idx, AST::CompoundStmt& cstmt);
    void parse_func_assign(uint32_t& idx, AST::CompoundStmt& cstmt);
    std::unique_ptr<AST::Node> pratt_parse(
        uint32_t& idx,
        uint8_t   priority);  // pratt parsing algortihgm
    void parse_if(uint32_t& idx, AST::CompoundStmt& cstmt);
    void parse_while(uint32_t& idx, AST::CompoundStmt& cstmt);
    void parse_for(uint32_t& idx, AST::CompoundStmt& cstmt);
    // wider, than parse_func_body, fucntion that parse blocks {...}
    std::unique_ptr<AST::CompoundStmt> parse_block(uint32_t& idx);    
};
