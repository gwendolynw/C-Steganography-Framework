//
//  node.h
//  f5+dct
//
//  Created by Gwendolyn Weston on 7/20/13.
//  Copyright (c) 2013 Gwendolyn. All rights reserved.
//

#ifndef f5_dct_node_h
#define f5_dct_node_h

typedef struct buffer_coefficient buffer_coefficient;

//struct containing coefficient and metadata
struct buffer_coefficient {
    int row_index;
    int column_index;
    int block_index;
    int coefficient;
};

typedef struct node node;

//linked list node
struct node{
    buffer_coefficient coeff_struct;
    node *next;
    node *prev;
};

void add_to_linked_list(node *new_node, node *current_node);
void remove_from_linked_list(node *delete_node);
node *traverse_n_nodes_forward(node *current_node, size_t n);
node *traverse_n_nodes_backward(node *current_node, size_t n);
void print_linked_list(node *root);



#endif
