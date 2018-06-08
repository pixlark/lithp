#include "lex-parse.h"

#define DS_UTIL_IMPLEMENTATION
#include "ds_util.h"

struct Lisp_VM {
	HashTable<char*, int> variable_bindings;

	void throw_unbound(char * ident);
	void throw_argnum(char * proc, int expected, int got);
	void throw_argtype(char * proc, int argnum, char * expected, char * got);
	
	void init();
	AST_Node * evaluate(AST_Node * root);
};

void Lisp_VM::throw_unbound(char * ident)
{
	printf("Identifier '%s' is unbound.\n",
		ident);
}

void Lisp_VM::throw_argnum(char * proc, int expected, int got)
{
	printf("Procedure '%s' expected %d args, got %d.\n",
		proc, expected, got);
}

void Lisp_VM::throw_argtype(char * proc, int argnum, char * expected, char * got)
{
	printf("Procedure '%s' arg %d expected type %s, got %s.\n",
		proc, argnum, expected, got);
}

int hash_str(char * key, int table_size)
{
	// Not very good, but it works
	int accumulator = 0;
	while (*key != '\0') accumulator += *(key++);
	return accumulator % table_size;
}

bool hash_str_comp(char * a, char * b)
{
	return strcmp(a, b) == 0;
}

void Lisp_VM::init()
{
	variable_bindings.init(100, hash_str, hash_str_comp);
}

int ast_list_length(AST_Node * list)
{
	assert(list->type == AST_LIST);
	AST_Node * start = list->list_head;
	int len = 0;
	while (start != NULL) {
		len++;
		start = start->next;
	}
	return len;
}

AST_Node * ast_list_index(AST_Node * list, int i)
{
	assert(list->type == AST_LIST);
	AST_Node * start = list->list_head;
	while (i-- > 0) {
		start = start->next;
		if (start == NULL) return NULL;
	}
	return start;
}

void ast_list_replace(AST_Node * list, AST_Node * new_node, int i)
{
	if (i == 0) {
		new_node->next = list->list_head->next;
		list->list_head = new_node;
	} else {
		AST_Node * before = ast_list_index(list, i - 1);
		new_node->next = before->next->next;
		before->next = new_node;
	}
}

AST_Node * Lisp_VM::evaluate(AST_Node * root)
{
	printf("..");
	print_ast_as_lisp(root);
	printf("\n");
	// Atoms evaluate to themselves
	if (root->type == AST_LITERAL ||
		root->type == AST_T)
		return root;
	// Evaluate identifiers
	if (root->type == AST_IDENT) {
		if (variable_bindings.index(root->identifier, NULL)) {
			printf("%s is not bound.\n", root->identifier);
			return NULL;
		}
		AST_Node * resolved = alloc_node(AST_LITERAL);
		variable_bindings.index(root->identifier, &resolved->literal);
		return resolved;
	}
	assert(root->type == AST_LIST);
	// Get procedure name
	AST_Node * head = root->list_head;
	if (head->type == AST_LIST) {
		head = evaluate(head);
		ast_list_replace(root, head, 0);
	}
	if (head->type == AST_LITERAL ||
		head->type == AST_LIST ||
		head->type == AST_T) {
		printf("Only identifiers can represent procedure calls.\n");
		return NULL;
	}
	assert(head->type == AST_IDENT);
	// Call procedure on arguments
	if (strcmp(head->identifier, "define") == 0) {
		assert(ast_list_length(root) == 3);
		AST_Node * ident_to_bind = ast_list_index(root, 1);
		assert(ident_to_bind->type == AST_IDENT);
		AST_Node * value = ast_list_index(root, 2);
		if (value->type != AST_LITERAL) {
			value = evaluate(value);
			assert(value->type == AST_LITERAL);
			ast_list_replace(root, value, 2);
		}
		variable_bindings.insert(ident_to_bind->identifier, value->literal);
		return value;
	} else if (strcmp(head->identifier, "+") == 0) {
		assert(ast_list_length(root) == 3);
		for (int i = 1; i < 3; i++) {
			AST_Node * it = ast_list_index(root, i);
			if (it->type != AST_LITERAL) {
				it = evaluate(it);
				if (it->type != AST_LITERAL) {
					printf("Only numbers can be used in +.\n");
					return NULL;
				}
				ast_list_replace(root, it, i);
			}
		}
		AST_Node * new_node = alloc_node(AST_LITERAL);
		new_node->literal =
			ast_list_index(root, 1)->literal +
			ast_list_index(root, 2)->literal;
		return new_node;
	} else if (strcmp(head->identifier, "=") == 0) {
		assert(ast_list_length(root) == 3);
		for (int i = 1; i < 3; i++) {
			AST_Node * it = ast_list_index(root, i);
			if (it->type != AST_LITERAL) {
				it = evaluate(it);
				if (it->type != AST_LITERAL) {
					printf("Only numbers can be used in =.\n");
					return NULL;
				}
				ast_list_replace(root, it, i);
			}
		}
		AST_Node * new_node;
		if (ast_list_index(root, 1)->literal ==
			ast_list_index(root, 2)->literal) {
			new_node = alloc_node(AST_T);
		} else {
			new_node = alloc_node(AST_LIST);
		}
		return new_node;
	} else {
		printf("That procedure is not bound.\n");
		return NULL;
	}
}

int main()
{
	Lisp_VM lisp_vm;
	lisp_vm.init();

	while (1) {
		// READ
		printf("$ ");
		char source_buffer[512];
		fgets(source_buffer, 512, stdin);
		if (strcmp(source_buffer, "(quit)\n") == 0) break; // Probably a better way to do this
		AST_Node * ast_root = get_ast(source_buffer);
		/*
		printf("Syntax tree...\n");
		write_gvr_and_view(ast_root);*/
		
		AST_Node * iter = ast_root->list_head;
		while (iter != NULL) {
			// EVAL
			AST_Node * resolved = lisp_vm.evaluate(iter);
			if (resolved == NULL) break;
			// PRINT

			/*
			printf("Evaluated tree...\n");
			write_gvr_and_view(resolved);*/
			print_ast_as_lisp(resolved);
			printf("\n");
			iter = iter->next;
		}
	}
}
