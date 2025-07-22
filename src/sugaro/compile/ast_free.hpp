#include <iostream>
#include "ast_nodes.hpp"

void free_node(ast_node *node);
void _free_if_node(ast_node_if *node);