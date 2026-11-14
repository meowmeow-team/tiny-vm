#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace AST {

enum NodeType {
    BASE,  // 0
    UNIT_DECL,
    FUNC_DECL,
    PARAM_DECL,
    COMPOUND_STMT,
    ASSIGN_STMT,  // 5
    VARIABLE_DECL,
    REF_VAR_DECL,
    INT_LITERAL_DECL,
    BINARY_OPERATION,  // 9
};

struct Node {
    Node(NodeType type);
    virtual ~Node() = default;

    Node(const Node& other) = delete;
    Node& operator=(const Node& other) = delete;

    Node(Node&& other) = default;
    Node& operator=(Node&& other) noexcept = default;

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

    // dumb debug thing
    void print(UnitDecl* unit) const noexcept;
    // TODO delete it then parser is gonna be ready, or make it cooler
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

struct IntLiteralDecl : public Node {
    IntLiteralDecl();
};

struct BinaryOperation : public Node {
    BinaryOperation();
};

};  // namespace AST
