#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace AST {

enum NodeType {
    BASE,
    UNIT_DECL,
    FUNC_DECL,
    PARAM_DECL,
    COMPOUND_STMT,
    VARIABLE_DECL,
    BINARY_OPERATION,
    ASSIGN
};

struct Node {
    Node(NodeType type);
    virtual ~Node() = default;

    NodeType type;
    uint32_t ref_idx;  // an index of corresponding element (lexeme) in the
                       // tokens vector for example: if this node contains
                       // variable a (int a = 10), ref_idx gonna be reference to
                       // the token with 'a' lexeme, damn

    std::vector<std::unique_ptr<Node>> kids;
};

// root of the unit (ex. file)
struct UnitDecl : public Node {
    UnitDecl();
    UnitDecl(const std::string& file_name);

    std::string file_name;
};

// funcition declaration
// contains: ref_idx - to the name,
// ref_type - to the type, param structs in the kids
struct FunctionDecl : public Node {
    FunctionDecl();

    uint32_t ref_type;
};

// param declaration inside the function
// contains: ref_idx - to the name, ref_type - to the type
struct ParamDecl : public Node {
    ParamDecl();

    uint32_t ref_type;
};

// body of the fucntion, like  { ... }
struct CompoundStmt : public Node {
    CompoundStmt();
};

struct AssignStmt : public Node {
    AssignStmt();
};

// variable declaration
// contains ref to the type(token) and to the value (token)
struct VariableDecl : public Node {
    VariableDecl();

    uint32_t ref_type;  // identical shithole like the ref_idx, but this thing
                        // is a reference to the token with this variable type
};

// just a reference to the another variable
// contains as a ref_idx, but this is not a reference to previous token
struct RefVarDecl : public Node {
    RefVarDecl();
};

struct BinaryOperation : public Node {
    BinaryOperation();
};

};  // namespace AST
