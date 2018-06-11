#include "lex-parse.h"

#define DS_UTIL_IMPLEMENTATION
#include "ds_util.h"

#if 0
struct Lisp_VM {
	HashTable<char*, Cons*> bindings;

	void        init();
	Cons * apply_function(Function function, Cons * arguments);
	Cons * evaluate(Cons * list);
};

void Lisp_VM::init()
{
	bindings.init(100, hash_str, hash_str_comp);
}

Cons * substitute_arguments(Cons * procedure, char * symbol, Cons * arg)
{
	assert(procedure->type == NODE_LIST);
	Cons * iter = procedure->list;
	while (iter != NULL) {
		if (iter->type == NODE_LIST) {
			substitute_arguments(iter, symbol, arg);
		}
		if (iter->type == NODE_ATOM &&
			iter->atom.type == ATOM_SYMBOL) {
			if (strcmp(iter->atom.symbol, symbol) == 0) {
				Cons * arg_copy = node_deep_copy(arg);
				Cons * next = iter->next;
				*iter = *arg_copy;
				iter->next = next;
			}
		}
		iter = iter->next;
	}
}

Cons * Lisp_VM::apply_function(Function function, Cons * arguments)
{
	int arg_count = list_length(function.args);
	assert(arg_count == chain_length(arguments));
	Cons * procedure = node_deep_copy(function.procedure);
	for (int i = 0; i < arg_count; i++) {
		Cons * arg = list_index(function.args, i);
		assert(arg->type      == NODE_ATOM);
		assert(arg->atom.type == ATOM_SYMBOL);
		char * arg_symbol = list_index(function.args, i)->atom.symbol;
		Cons * replace_arg = chain_index(arguments, i);
		substitute_arguments(procedure, arg_symbol, replace_arg);
	}
	return procedure;
}

Cons * Lisp_VM::evaluate(Cons * list)
{
	// Nil
	if (list->type == NODE_NIL) {
		return list;
	}
	// Atom
	if (list->type == NODE_ATOM) {
		// Numbers and Functions
		if (list->atom.type == ATOM_NUMBER || list->atom.type == ATOM_FUNCTION) {
			return list;
		}
		// Symbol
		if (bindings.index(list->atom.symbol, NULL)) {
			printf("Symbol %s not bound.\n", list->atom.symbol);
			return NULL;
		}
		Cons * node;
		bindings.index(list->atom.symbol, &node);
		return node;
	}
	// List
	Cons * head = list->list;
	if (head->type == NODE_ATOM) {
		if (head->atom.type == ATOM_SYMBOL) {
			// Special forms
			if (strcmp(head->atom.symbol, "quote") == 0) {
				if (list_length(list) != 2) {
					printf("Wrong number of arguments to quote.\n");
					return NULL;
				}
				return head->next;
			} else if (strcmp(head->atom.symbol, "lambda") == 0) {
				if (list_length(list) != 3) {
					printf("Wrong number of arguments to lambda.\n");
					return NULL;
				}
				Cons * function = alloc_atom();
				function->atom.type = ATOM_FUNCTION;
				assert(head->next->type == NODE_LIST);
				function->atom.function.args = node_copy_independent(head->next);
				assert(head->next->next->type == NODE_LIST);
				function->atom.function.procedure = node_copy_independent(head->next->next);
				return function;
			} else if (strcmp(head->atom.symbol, "define") == 0) {
				if (list_length(list) != 3) {
					printf("Wrong number of arguments to define.\n");
					return NULL;
				}
				assert(head->next->atom.type == ATOM_SYMBOL);
				Cons * copy = node_copy_independent(head->next->next);
				copy = evaluate(copy);
				if (copy == NULL) return NULL;
				bindings.insert(head->next->atom.symbol, copy);
				return copy;
			} else if (strcmp(head->atom.symbol, "*") == 0) {
				if (list_length(list) != 3) {
					printf("Wrong number of arguments to *.\n");
					return NULL;
				}
				Cons * ret = node_copy_independent(head->next);
				ret->atom.number *= head->next->next->atom.number;
				return ret;
			} else {
				// Lookup function
				Cons * resolved = evaluate(head);
				if (resolved->atom.type != ATOM_FUNCTION) {
					printf("Can't apply as function.\n");
					return NULL;
				}
				// Apply function
				Cons * applied = apply_function(resolved->atom.function, head->next);
				return applied;
			}
		}
	}
	printf("Unimplemented.\n");
	return NULL;
}
#endif

int hash_str(char * key, int table_size)
{
	// TODO(pixlark): Better algorithm
	int acc;
	while (*key != '\0') acc += *(key++);
	return acc % table_size;
}

bool hash_str_comp(char * a, char * b)
{
	return strcmp(a, b) == 0;
}

Cell * cell_push(Lisp_VM * vm, Cell * cell, Cell * to_push)
{
	assert(cell->cell_type == CELL_CONS);
	if (cell == vm->nil) {
		// If we're trying to push onto NIL, we have to start up a new list
		Cell * new_cons = alloc_cell();
		new_cons->cell_type = CELL_CONS;
		new_cons->cons.car  = to_push;
		new_cons->cons.cdr  = vm->nil;
		return new_cons;
	} else {
		// If we're not at the end of the list, recurse on CDR
		cell->cons.cdr = cell_push(vm, cell->cons.cdr, to_push);
		return cell;
	}
}

void print_cell_as_lisp(Lisp_VM * vm, Cell * cell, bool first_cons)
{
	if (cell == vm->nil) {
		printf("NIL");
	} else if (cell->cell_type == CELL_CONS) {
		if (first_cons) printf("(");
		print_cell_as_lisp(vm, cell->cons.car, true);
		if (cell->cons.cdr == vm->nil) {
			printf(")");
		} else {
			printf(" ");
			print_cell_as_lisp(vm, cell->cons.cdr, false);
		}
	} else if (cell->cell_type == CELL_NUMBER) {
		printf("%d", cell->number);
	} else if (cell->cell_type == CELL_SYMBOL) {
		printf("%s", cell->symbol);
	}
}

Cell * alloc_cell()
{
	Cell * cell = (Cell*) malloc(sizeof(Cell));
	cell->_debug_tag = "ALLOCATED";
}

void Lisp_VM::init()
{
	bindings.init(100, hash_str, hash_str_comp);
	// Nil
	nil = alloc_cell();
	nil->cell_type = CELL_CONS;
	nil->cons.car  = nil;
	nil->cons.cdr  = nil;
	nil->_debug_tag = "NIL";
}

Cell * Lisp_VM::apply_function(Cell * to_call, Cell * arguments)
{
	
}

Cell * Lisp_VM::evaluate(Cell * cell)
{
	if (cell->cell_type == CELL_NUMBER) {
		return cell;
	} else if (cell->cell_type == CELL_SYMBOL) {
		if (bindings.index(cell->symbol, NULL)) {
			printf("Symbol %s not bound.\n", cell->symbol);
			return NULL;
		}
		Cell * resolved;
		bindings.index(cell->symbol, &resolved);
		return resolved;
	} else if (cell->cell_type == CELL_CONS) {
		if (cell == this->nil) return cell;
		return apply_function(cell->cons.car, cell->cons.cdr);
	}
	return NULL;
}

int main()
{
	Lisp_VM vm;
	vm.init();
	
	while (1) {
		printf("$ ");
		char source_buffer[512];
		fgets(source_buffer, 512, stdin);
		if (strcmp(source_buffer, "(quit)\n") == 0) break;
		Cell * parsed = parse_source(&vm, source_buffer);
		while (parsed != vm.nil) {
			Cell * evaluated = vm.evaluate(parsed->cons.car);
			if (evaluated != NULL) {
				print_cell_as_lisp(&vm, evaluated);
				printf("\n");
			}
			parsed = parsed->cons.cdr;
		}
	}
}
