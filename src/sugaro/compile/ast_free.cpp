#include "ast_free.hpp"

void free_node(ast_node *node) {
    if (!node) return;

    switch (node->ty) {
        case AST_IF_STATEMENT:
            _free_if_node((ast_node_if*) node);
            break;
        default:
            break;
    }

    delete[] node;
    node = nullptr;
}

void _free_if_node(ast_node_if *node) {
    if (!node) return;

    if (node->else_block)
        free_node(node->else_block);

    if (node->body)
        free_node(node->body);

    if (node->condition)
        free_node(node->condition);
}