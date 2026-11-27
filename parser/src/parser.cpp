#include "../include/parser.hpp"

#include "../../core/exception.hpp"

std::unordered_map<std::string, uint8_t> Parser::pratt_priorities = {
    {"=", 1}, {"||", 2}, {"&&", 3}, {"==", 4}, {"!=", 4}, {"<", 5},
    {">", 5}, {"+", 6},  {"-", 6},  {"*", 7},  {"/", 7}};

Parser::Parser(SourceManager& source_manager, Logger& logger)
    : source_manager(source_manager), logger(logger) {}

AST::UnitDecl Parser::parse_unit(const std::string& file_name) {
    AST::UnitDecl unit{};
    unit.file_name = file_name;

    uint32_t idx = 0;
    while (idx < source_manager.size()) {
        std::unique_ptr<AST::FunctionDecl> func = parse_func(idx);
        if (func) {
            unit.kids.push_back(std::move(func));
        }
    }

    return unit;
}

std::unique_ptr<AST::FunctionDecl> Parser::parse_func(uint32_t& idx) {
    if (idx >= source_manager.size())
        throw vm::syntax_error("unexpected end of file", 0, 0);

    if (source_manager.size() - idx < MIN_FUNC_DECL + 2)
        throw vm::syntax_error("func syntax parsing error",
                               source_manager.get(idx).get_pos().row,
                               source_manager.get(idx).get_pos().column);

    std::unique_ptr<AST::FunctionDecl> func =
        std::make_unique<AST::FunctionDecl>();

    // function parsing pipeline lol (kms)
    try {
        parse_func_def(idx, *func);
        parse_func_params(idx, func->kids);
        parse_func_body(idx, *func);
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
                        token.get_pos().row, token.get_pos().column);
                func.ref_type = idx;
                break;
            case 1:
                if (token.get_type() != TokenType::IDENTIFIER)
                    throw vm::syntax_error(
                        "func parsing: second one is not an identifier",
                        token.get_pos().row, token.get_pos().column);

                func.ref_idx = idx;
                break;
            case 2:
                if (token.get_type() != TokenType::SYMBOL)
                    throw vm::syntax_error("func parsing: there is no ()",
                                           token.get_pos().row,
                                           token.get_pos().column);
                if (token.get_lexeme()[0] != '(')
                    throw vm::syntax_error("func parsing: wrong symbol",
                                           token.get_pos().row,
                                           token.get_pos().column);
                idx++;
                return;
            default:
                throw vm::syntax_error("func def parsing ub", 0, 0);
        }

        words++;
        idx++;
    }

    throw vm::syntax_error("func def parsing failed", 0, 0);
}

// example: int a, int b)
void Parser::parse_func_params(uint32_t&                                idx,
                               std::vector<std::unique_ptr<AST::Node>>& res) {
    bool     has_type = false;
    uint32_t temp_idx = 0;

    while (idx < source_manager.size()) {
        const auto& token = source_manager.get(idx);
        auto        type = token.get_type();

        if (has_type) {
            if (type != TokenType::IDENTIFIER)
                throw vm::syntax_error(
                    "func pars parsing: variable without name",
                    token.get_pos().row, token.get_pos().column);

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
                    return;
                }
                throw vm::syntax_error("func pars parsing: wrong symbol",
                                       token.get_pos().row,
                                       token.get_pos().column);
            }

            if (type != TokenType::KEYWORD_TYPE)
                throw vm::syntax_error(
                    "func pars parsing: variable without type",
                    token.get_pos().row, token.get_pos().column);

            temp_idx = idx;
            has_type = true;
        }

        idx++;
    }

    throw vm::syntax_error("func pars parsing failed", 0, 0);
}

// example: {
//     int a = 10;
//     int b = 20;
//     int c = a + b;
//     return c;
// }
void Parser::parse_func_body(uint32_t& idx, AST::FunctionDecl& func) {
    auto body = parse_block(idx);
    func.kids.push_back(std::move(body));
}

// example: int a = 10;
//          int a = func();
//          b = 20;
//          b = a;
//          TODO func(); -> parse_func_call
//          TODO for(...)
//          TODO while(...)
void Parser::parse_func_decl(uint32_t& idx, AST::CompoundStmt& cstmt) {
    if (idx >= source_manager.size())
        throw vm::syntax_error("end of file", 0, 0);

    const auto& token = source_manager.get(idx);
    auto        type = token.get_type();

    if (type == TokenType::KEYWORD) {
        if (token.get_lexeme() == "if") {
            parse_if(idx, cstmt);
            return;
        }
        if (token.get_lexeme() == "while") {
            parse_while(idx, cstmt);
            return;
        }
        if (token.get_lexeme() == "for") {
            parse_for(idx, cstmt);
            return;
        }

        if (token.get_lexeme() == "return") {
            parse_ret(idx, cstmt);
            return;
        }

        throw vm::syntax_error("unknown keyword in block", token.get_pos().row,
                               token.get_pos().column);
    }

    if (type == TokenType::KEYWORD_TYPE || type == TokenType::IDENTIFIER) {
        parse_func_assign(idx, cstmt);
        return;
    }

    if (type == TokenType::KEYWORD_RET) {
        if (token.get_lexeme() == "return") {
            parse_ret(idx, cstmt);
            return;
        }

        throw vm::syntax_error("unknown keyword_ret in block",
                               token.get_pos().row, token.get_pos().column);
    }

    throw vm::syntax_error("unexpected token in block", token.get_pos().row,
                           token.get_pos().column);
}

// example: int a = 10;
//          int a = func(); (on the last one we call parse_func_call yo)
//          b = 20;
//          b = a;
void Parser::parse_func_assign(uint32_t& idx, AST::CompoundStmt& cstmt) {
    std::unique_ptr<AST::AssignStmt> astmt =
        std::make_unique<AST::AssignStmt>();

    if (idx >= source_manager.size())
        throw vm::syntax_error("incorrect end of file", 0, 0);

    // left side of assigning operation
    const auto& f = source_manager.get(idx);
    if (f.get_type() == TokenType::KEYWORD_TYPE) {
        // int a = ...
        const uint32_t idx_type = idx++;
        if (idx >= source_manager.size())
            throw vm::syntax_error("no variable name", f.get_pos().row,
                                   f.get_pos().column);

        const auto& n = source_manager.get(idx);
        if (n.get_type() != TokenType::IDENTIFIER)
            throw vm::syntax_error("no variable name was found",
                                   f.get_pos().row, f.get_pos().column);

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
        throw vm::syntax_error("expected var decl", f.get_pos().row,
                               f.get_pos().column);
    }

    if (idx >= source_manager.size())
        throw vm::syntax_error("expected assgin symbol", f.get_pos().row,
                               f.get_pos().column);

    const auto& at = source_manager.get(idx);
    auto        at_type = at.get_type();

    if (at_type != TokenType::SYMBOL_ASSIGN)
        throw vm::syntax_error("expected assign symbol bro", at.get_pos().row,
                               at.get_pos().column);
    idx++;

    auto expression = pratt_parse(idx, 0);
    astmt->kids.push_back(std::move(expression));

    if (idx >= source_manager.size())
        throw vm::syntax_error("unexpected end of file", 0, 0);
    if (source_manager.get(idx).get_type() != TokenType::SEMICOLON)
        throw vm::syntax_error("expected semiclon after the expression",
                               source_manager.get(idx).get_pos().row,
                               source_manager.get(idx).get_pos().column);

    idx++;
    cstmt.kids.push_back(std::move(astmt));
}

// TODO make it possible to call some functions
std::unique_ptr<AST::Node> Parser::pratt_parse(uint32_t& idx,
                                               uint8_t   priority) {
    std::unique_ptr<AST::Node> fuck = nullptr;

    const auto& token = source_manager.get(idx);
    auto        type = token.get_type();
    if (type == TokenType::IDENTIFIER) {
        fuck = std::make_unique<AST::RefVarDecl>();
        fuck->ref_idx = idx++;
    } else if (type == TokenType::NUMBER) {
        fuck = std::make_unique<AST::IntLiteralDecl>();
        fuck->ref_idx = idx++;
    } else
        throw vm::syntax_error("pratt expected variable or identifier",
                               token.get_pos().row, token.get_pos().column);

    while (idx < source_manager.size()) {
        const auto& t = source_manager.get(idx);
        if (t.get_lexeme() == ";") break;

        auto it = pratt_priorities.find(t.get_lexeme());
        if (it == pratt_priorities.end() || it->second <= priority) break;

        uint32_t op_idx = idx++;

        auto rhs = pratt_parse(idx, it->second);

        std::unique_ptr<AST::BinaryOperation> op =
            std::make_unique<AST::BinaryOperation>();
        op->ref_idx = op_idx;
        op->kids.push_back(std::move(fuck));
        op->kids.push_back(std::move(rhs));

        fuck = std::move(op);
    }

    return fuck;
}

void Parser::parse_if(uint32_t& idx, AST::CompoundStmt& cstmt) {
    idx++;

    check_eof(idx);

    if (source_manager.get(idx).get_lexeme() != "(")
        throw vm::syntax_error("expected '(' after if", 0, 0);

    idx++;

    auto condition = pratt_parse(idx, 0);

    check_eof(idx);

    if (source_manager.get(idx).get_lexeme() != ")")
        throw vm::syntax_error("expected ')' after if condition", 0, 0);

    idx++;

    auto body = parse_block(idx);

    auto if_node = std::make_unique<AST::IfStmt>();
    if_node->kids.push_back(std::move(condition));
    if_node->kids.push_back(std::move(body));

    cstmt.kids.push_back(std::move(if_node));
}

std::unique_ptr<AST::CompoundStmt> Parser::parse_block(uint32_t& idx) {
    auto block = std::make_unique<AST::CompoundStmt>();

    check_eof(idx);

    if (source_manager.get(idx).get_lexeme() != "{") {
        throw vm::syntax_error("expected '{' to start block",
                               source_manager.get(idx).get_pos().row,
                               source_manager.get(idx).get_pos().column);
    }

    idx++;

    while (idx < source_manager.size()) {
        if (source_manager.get(idx).get_lexeme() == "}") {
            idx++;
            return block;
        }
        parse_func_decl(idx, *block);
    }

    throw vm::syntax_error("expected '}' to close block", 0, 0);
}

void Parser::parse_while(uint32_t& idx, AST::CompoundStmt& cstmt) {
    idx++;

    check_eof(idx);

    if (source_manager.get(idx).get_lexeme() != "(") {
        throw vm::syntax_error("expected '(' after while",
                               source_manager.get(idx).get_pos().row,
                               source_manager.get(idx).get_pos().column);
    }
    idx++;
    auto condition = pratt_parse(idx, 0);

    check_eof(idx);

    if (source_manager.get(idx).get_lexeme() != ")") {
        throw vm::syntax_error("expected ')' after while condition",
                               source_manager.get(idx).get_pos().row,
                               source_manager.get(idx).get_pos().column);
    }
    idx++;

    auto body = parse_block(idx);

    auto while_node = std::make_unique<AST::WhileStmt>();
    while_node->kids.push_back(std::move(condition));
    while_node->kids.push_back(std::move(body));

    cstmt.kids.push_back(std::move(while_node));
}

void Parser::parse_for(uint32_t& idx, AST::CompoundStmt& cstmt) {
    throw vm::syntax_error("for is not implemented yet",
                           source_manager.get(idx).get_pos().row,
                           source_manager.get(idx).get_pos().column);
}

void Parser::parse_ret(uint32_t& idx, AST::CompoundStmt& cstmt) {
    if (idx + 2 >= source_manager.size())
        throw vm::syntax_error("bad syntax for return operation", 0, 0);

    auto ret = std::make_unique<AST::RetStmt>();
    ret->ref_idx = idx++;

    std::unique_ptr<AST::Node> kid = nullptr;
    const auto&                raw_kid = source_manager.get(idx);
    if (raw_kid.get_type() == TokenType::IDENTIFIER) {
        kid = std::make_unique<AST::RefVarDecl>();
        kid->ref_idx = idx++;
    } else if (raw_kid.get_type() == TokenType::NUMBER) {
        kid = std::make_unique<AST::IntLiteralDecl>();
        kid->ref_idx = idx++;
    } else {
        throw vm::syntax_error("unknown token type of return",
                               raw_kid.get_pos().row, raw_kid.get_pos().column);
    }

    const auto& semicolon = source_manager.get(idx++);
    if (semicolon.get_lexeme() != ";")
        throw vm::syntax_error("forgot to add semicolon",
                               semicolon.get_pos().row,
                               semicolon.get_pos().column);

    ret->kids.push_back(std::move(kid));
    cstmt.kids.push_back(std::move(ret));
}

void Parser::check_eof(uint32_t idx) {
    if (idx >= source_manager.size())
        throw vm::syntax_error("unexpected end of file", 0, 0);
}
