#include "../include/ast.hpp"

using namespace AST;

Node::Node(NodeType type) : type(type), ref_idx(0), kids{} {}

Variable::Variable() : Node(VARIABLE), ref_type(0) {}

Operation::Operation() : Node(OPERATION) {}

Assign::Assign() : Node(ASSIGN) {}
