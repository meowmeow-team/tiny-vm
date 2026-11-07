#include "../include/parser.hpp"

#include "../../core/exception.hpp"

Parser::Parser(SourceManager& source_manager, Logger& logger)
    : source_manager(source_manager), logger(logger) {}

AST::UnitDecl Parser::parse_unit(const std::string& file_name) {
    AST::UnitDecl unit{};
    unit.file_name = file_name;

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

        auto params = parse_func_params(idx);
        func->kids.insert(func->kids.end(), params.begin(), params.end());
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
        const auto& token = source_manager.get(idx);
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
std::vector<std::unique_ptr<AST::ParamDecl>> Parser::parse_func_params(
    uint32_t& idx) {
    std::vector<std::unique_ptr<AST::ParamDecl>> res{};

    bool     has_type = false;
    uint32_t temp_idx = 0;

    while (idx < source_manager.size()) {
        const auto& token = source_manager.get(idx);
        auto        type = token.get_type();

        if (has_type) {
            if (type != TokenType::IDENTIFIER)
                throw vm::syntax_error(
                    "func pars parsing: variable without name",
                    token.get_pos().row);

            std::unique_ptr<AST::ParamDecl> param =
                std::make_unique<AST::ParamDecl>();
            param->ref_idx = idx;
            param->ref_type = temp_idx;
            res.push_back(std::move(param));

            has_type = false;
        } else {
            if (type == TokenType::SYMBOL) {
                if (token.get_lexeme()[0] == ',') {
                    idx++;
                    continue;
                } else if (token.get_lexeme()[0] == ')') {
                    idx++;
                    return std::move(res);
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

    const auto& s = source_manager.get(idx);
    if (s.get_type() != TokenType::SYMBOL || s.get_lexeme() != "{")
        throw vm::syntax_error("func body parsing: there is no {",
                               s.get_pos().row);
    idx++;

    while (idx < source_manager.size()) {
        const auto& token = source_manager.get(idx);
        auto        type = token.get_type();

        switch (type) {
            case TokenType::SYMBOL:
                if (token.get_lexeme() == "}") {
                    func.kids.push_back(std::move(c_stmt));
                    idx++;
                    return;
                }
                break;
            case TokenType::IDENTIFIER:
            case TokenType::KEYWORD_TYPE:
                parse_func_decl(idx, *c_stmt);
                break;
            // TODO parsing if stmt, while/for loops
            default:
                throw vm::syntax_error("func body parsing failed",
                                       token.get_pos().row);
        }

        idx++;
    }

    throw vm::syntax_error("func body parsing failed", idx);
}

// example: int a = 10;
//          int a = func();
//          b = 20;
//          b = a;
//          func(); -> parse_func_call
void Parser::parse_func_decl(uint32_t& idx, AST::CompoundStmt& cstmt) {
    if (idx >= source_manager.size()) throw vm::syntax_error("end of file", 0);

    const auto& token = source_manager.get(idx);
    auto        type = token.get_type();

    if (token.get_type() == TokenType::KEYWORD_TYPE ||
        token.get_type() == TokenType::IDENTIFIER) {
        parse_func_assign(idx, cstmt);
        return;
    }
}

// example: int a = 10;
//          int a = func(); (on the last one we call parse_func_call yo)
//          b = 20;
//          b = a;
void Parser::parse_func_assign(uint32_t& idx, AST::CompoundStmt& cstmt) {
    std::unique_ptr<AST::AssignStmt> astmt =
        std::make_unique<AST::AssignStmt>();

    if (idx >= source_manager.size())
        throw vm::syntax_error("incorrect end of file", 0);

    // left side of assigning operation
    const auto& f = source_manager.get(idx);
    if (f.get_type() == TokenType::KEYWORD_TYPE) {
        // int a = ...
        const uint32_t idx_type = idx++;
        if (idx >= source_manager.size())
            throw vm::syntax_error("no variable name", f.get_pos().row);

        const auto& n = source_manager.get(idx);
        if (n.get_type() != TokenType::IDENTIFIER)
            throw vm::syntax_error("no variable name was found",
                                   f.get_pos().row);

        auto var = std::make_unique<AST::VariableDecl>();
        var->ref_type = idx_type;
        var->ref_idx = idx;
        astmt->kids.push_back(std::move(var));
        idx++;

    } else if (f.get_type() == TokenType::IDENTIFIER) {
        // a = ...
        auto ref_var = std::make_unique<AST::RefVarDecl>();
        ref_var->ref_idx = idx;
        astmt->kids.push_back(std::move(ref_var));
        idx++;
    } else {
        throw vm::syntax_error("expected var decl", f.get_pos().row);
    }

    if (idx >= source_manager.size())
        throw vm::syntax_error("expected assgin symbol", f.get_pos().row);

    const auto& at = source_manager.get(idx);
    auto        at_type = at.get_type();

    if (at_type != TokenType::SYMBOL_ASSIGN)
        throw vm::syntax_error("expected assign symbol bro", at.get_pos().row);
    idx++;
}

std::unique_ptr<AST::Node> Parser::parse_expression(uint32_t& idx) {}
