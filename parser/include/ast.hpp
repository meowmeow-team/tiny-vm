#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace AST {

enum NodeType { BASE, VARIABLE, OPERATION, ASSIGN };

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

struct Variable : public Node {
    Variable();
    ~Variable() = default;

    uint32_t ref_type;  // identical shithole like the ref_idx, but this thing
                        // is a reference to the token with this variable type
};

struct Operation : public Node {
    Operation();
    ~Operation() = default;
};

struct Assign : public Node {
    Assign();
    ~Assign() = default;
};

};  // namespace AST
