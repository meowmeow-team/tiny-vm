#include "../include/ast.hpp"

#include <algorithm>
#include <iostream>
#include <queue>

using namespace AST;

Node::Node(NodeType type) : type(type), ref_idx(0), kids{} {}

UnitDecl::UnitDecl() : Node(UNIT_DECL), file_name() {}

UnitDecl::UnitDecl(const std::string& file_name)
    : Node(UNIT_DECL), file_name(file_name) {}

void UnitDecl::print(UnitDecl* unit) const noexcept {
    std::cout << "unit kids size: " << unit->kids.size() << std::endl;
    std::vector<AST::Node*> res;
    std::queue<AST::Node*>  qe;
    qe.push(unit);
    while (!qe.empty()) {
        AST::Node* n = qe.front();
        qe.pop();
        for (size_t i = 0; i < n->kids.size(); i++) {
            res.push_back(n->kids[i].get());
            qe.push(n->kids[i].get());
        }
    }

    std::cout << "res size: " << res.size() << std::endl;

    for (AST::Node* n : res) {
        std::cout << "node -> " << std::endl;
        std::cout << "  - type: " << std::to_string(n->type) << std::endl;
        std::cout << "  - kids size: " << n->kids.size() << std::endl;
        std::cout << "  - ref_idx: " << n->ref_idx << std::endl;
    }
}

FunctionDecl::FunctionDecl() : Node(FUNC_DECL), ref_type(0) {}

ParamDecl::ParamDecl() : Node(PARAM_DECL), ref_type(0) {}

CompoundStmt::CompoundStmt() : Node(COMPOUND_STMT) {}

AssignStmt::AssignStmt() : Node(ASSIGN_STMT) {}

VariableDecl::VariableDecl() : Node(VARIABLE_DECL), ref_type(0) {}

RefVarDecl::RefVarDecl() : Node(REF_VAR_DECL) {}

IntLiteralDecl::IntLiteralDecl() : Node(INT_LITERAL_DECL) {}

BinaryOperation::BinaryOperation() : Node(BINARY_OPERATION) {}
