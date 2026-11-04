#include "../include/ast.hpp"

using namespace AST;

Node::Node(NodeType type) : type(type), ref_idx(0), kids{} {}

UnitDecl::UnitDecl() : Node(UNIT_DECL), file_name() {}

UnitDecl::UnitDecl(const std::string& file_name)
    : Node(UNIT_DECL), file_name(file_name) {}

FunctionDecl::FunctionDecl() : Node(FUNC_DECL), ref_type(0) {}

ParamDecl::ParamDecl() : Node(PARAM_DECL), ref_type(0) {}

CompoundStmt::CompoundStmt() : Node(COMPOUND_STMT) {}

VariableDecl::VariableDecl() : Node(VARIABLE_DECL), ref_type(0) {}

BinaryOperation::BinaryOperation() : Node(BINARY_OPERATION) {}
