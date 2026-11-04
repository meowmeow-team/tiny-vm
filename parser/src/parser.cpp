#include "../include/parser.hpp"

#include "../../core/exception.hpp"

Parser::Parser(SourceManager& source_manager, Logger& logger)
    : source_manager(source_manager), logger(logger) {}

AST::UnitDecl Parser::make_ast() {
    AST::UnitDecl unit{};

    uint32_t idx = 0;
    while (idx < source_manager.size()) {
        std::unique_ptr<AST::FunctionDecl> func = parse_func(idx);
        unit.kids.push_back(std::move(func));
    }

    return unit;
}

std::unique_ptr<AST::FunctionDecl> Parser::parse_func(uint32_t& idx) {
    if (idx > source_manager.size() ||
        source_manager.size() - idx < MIN_FUNC_DECL + 2)
        throw vm::syntax_error("func syntax parsing error",
                               source_manager.get(idx).get_pos().row);

    std::unique_ptr<AST::FunctionDecl> func =
        std::make_unique<AST::FunctionDecl>();

    // function parsing pipeline lol (kms)
    try {
        parse_func_def(idx, *func);
        parse_func_params(idx, *func);
    } catch (const vm::syntax_error& e) {
        logger.log(Logger::ERROR, "func parsing pipeline failed:\n", e.what());
        throw;
    }

    return func;
}

// example: int main(
void Parser::parse_func_def(uint32_t& idx, AST::FunctionDecl& func) {
    uint8_t words = 0;
    while (idx < source_manager.size()) {
        auto token = source_manager.get(idx);
        switch (words) {
            case 0:
                if (token.get_type() != TokenType::KEYWORD_TYPE)
                    throw vm::syntax_error(
                        "func parsing: first one is not a keyword",
                        token.get_pos().row);
                func.ref_type = idx;
                break;
            case 1:
                if (token.get_type() != TokenType::IDENTIFIER)
                    throw vm::syntax_error(
                        "func parsing: second one is not an identifier",
                        token.get_pos().row);

                func.ref_idx = idx;
                break;
            case 2:
                if (token.get_type() != TokenType::SYMBOL)
                    throw vm::syntax_error("func parsing: there is no ()",
                                           token.get_pos().row);
                if (token.get_lexeme()[0] != '(')
                    throw vm::syntax_error("func parsing: wrong symbol",
                                           token.get_pos().row);
                idx++;
                return;
            default:
                throw vm::syntax_error("func def parsing ub", idx);
        }

        words++;
        idx++;
    }

    throw vm::syntax_error("func def parsing failed", idx);
}

// example: int a, int b)
void Parser::parse_func_params(uint32_t& idx, AST::FunctionDecl& func) {
    bool     has_type = false;
    uint32_t temp_idx = 0;

    while (idx < source_manager.size()) {
        auto token = source_manager.get(idx);
        auto type = token.get_type();

        if (has_type) {
            if (type != TokenType::IDENTIFIER)
                throw vm::syntax_error(
                    "func pars parsing: variable without name",
                    token.get_pos().row);

            std::unique_ptr<AST::ParamDecl> param =
                std::make_unique<AST::ParamDecl>();
            param->ref_idx = idx;
            param->ref_type = temp_idx;
            func.kids.push_back(std::move(param));

            has_type = false;
        } else {
            if (type == TokenType::SYMBOL) {
                if (token.get_lexeme()[0] == ',') {
                    idx++;
                    continue;
                } else if (token.get_lexeme()[0] == ')') {
                    idx++;
                    return;
                }
                throw vm::syntax_error("func pars parsing: wrong symbol",
                                       token.get_pos().row);
            }

            if (type != TokenType::KEYWORD_TYPE)
                throw vm::syntax_error(
                    "func pars parsing: variable without type",
                    token.get_pos().row);

            temp_idx = idx;
            has_type = true;
        }

        idx++;
    }

    throw vm::syntax_error("func pars parsing failed", idx);
}

// example: {
//     int a = 10;
//     int b = 20;
//     int c = a + b;
//     return c;
// }
void Parser::parse_func_body(uint32_t& idx, AST::FunctionDecl& func) {
    std::unique_ptr<AST::CompoundStmt> c_stmt =
        std::make_unique<AST::CompoundStmt>();

    while (idx < source_manager.size()) {
        auto token = source_manager.get(idx);
        idx++;
    }

    throw vm::syntax_error("func body parsing failed", idx);
}
