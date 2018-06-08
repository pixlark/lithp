#ifndef LITHP_LEX_PARSE_H
#define LITHP_LEX_PARSE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "ds_util.h"

enum AST_Type {
	AST_IDENT,
	AST_LITERAL,
	AST_LIST,
	AST_T,
};

extern char * ast_type_string[];

struct AST_Node {
	AST_Type type;
	AST_Node * next;
	union {
		AST_Node * list_head;
		char * identifier;
		int literal;
	};
	
	// Debugging stuff
	bool root_node;
	int debug_id;
	int next_nil_id;
	int child_nil_id;
	//

	void init(AST_Type type, bool root_node=false);
	void list_add_child(AST_Node * child);
	char * debug_info();
	void graph_viz_repr(FILE * file);
	char * graph_viz_groups();
};

AST_Node * alloc_node(AST_Type type);

void print_ast_as_lisp(AST_Node * root);
void write_gvr_and_view(AST_Node * root);
AST_Node * get_ast(char * file_path);

#endif
