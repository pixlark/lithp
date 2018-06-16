#include "lex-parse.h"

#define DS_UTIL_IMPLEMENTATION
#include "ds_util.h"

bool string_in_list(char ** list, int list_len, char * str)
{
	for (int i = 0; i < list_len; i++) {
		if (strcmp(list[i], str) == 0) return true;
	}
	return false;
}

#define SPECIAL_FORM_COUNT 5
char * special_forms[SPECIAL_FORM_COUNT] = {
	"set",
	"if",
	"=",
	"+",
	"*",
};

int hash_str(char * key, int table_size)
{
	// TODO(pixlark): Better algorithm
	int acc = 0;
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

int list_length(Lisp_VM * vm, Cell * cell)
{
	assert(cell->cell_type == CELL_CONS);
	if (cell == vm->nil) return 0;
	else return 1 + list_length(vm, cell->cons.cdr);
}

Cell * list_index(Lisp_VM * vm, Cell * start, int index)
{
	assert(start->cell_type == CELL_CONS);
	if (index == 0) return start;
	else return list_index(vm, start->cons.cdr, index - 1);
}

Cell * deep_copy_cell(Lisp_VM * vm, Cell * to_copy)
{
	if (to_copy == vm->nil)   return vm->nil;
	if (to_copy == vm->truth) return vm->truth;
	Cell * copy = alloc_cell();
	copy->cell_type = to_copy->cell_type;
	// Could probably do a direct bit-by-bit copy of the union here?
	switch (to_copy->cell_type) {
	case CELL_SYMBOL:
		copy->symbol = to_copy->symbol;
		break;
	case CELL_NUMBER:
		copy->number = to_copy->number;
		break;
	case CELL_CONS:
		copy->cons.car = deep_copy_cell(vm, to_copy->cons.car);
		copy->cons.cdr = deep_copy_cell(vm, to_copy->cons.cdr);
		break;
	}
	copy->_debug_tag = "COPY";
	return copy;
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
	// NIL
	nil = alloc_cell();
	nil->cell_type = CELL_CONS;
	nil->cons.car  = nil;
	nil->cons.cdr  = nil;
	nil->_debug_tag = "NIL";
	// T
	truth = alloc_cell();
	truth->cell_type = CELL_SYMBOL;
	truth->symbol = "T";
	truth->_debug_tag = "T";
}

Cell * Lisp_VM::special_form(Cell * form, Cell * arguments)
{
	assert(form->cell_type == CELL_SYMBOL);
	assert(arguments->cell_type == CELL_CONS);
	char * symbol = form->symbol;
	if (strcmp(symbol, "set") == 0) {
		if (list_length(this, arguments) != 2) {
			printf("set takes two arguments");
		}
		assert(list_index(this, arguments, 0)->cons.car->cell_type == CELL_SYMBOL);
		char * bind_symbol = list_index(this, arguments, 0)->cons.car->symbol;
		Cell * bind_cell   =
			deep_copy_cell(this, list_index(this, arguments, 1)->cons.car);
		bindings.insert(bind_symbol, bind_cell);
		return bind_cell;
	} else if (strcmp(symbol, "if") == 0) {
		if (list_length(this, arguments) != 3) {
			printf("if takes three arguments.\n");
			return NULL;
		}
		Cell * condition = evaluate(list_index(this, arguments, 0)->cons.car);
		if (condition == truth) {
			return evaluate(list_index(this, arguments, 1)->cons.car);
		} else if (condition == nil) {
			return evaluate(list_index(this, arguments, 2)->cons.car);
		} else {
			printf("if condition didn't resolve to T or NIL.\n");
			return NULL;
		}
	} else if (strcmp(symbol, "=") == 0) {
		if (list_length(this, arguments) != 2) {
			printf("= takes two arguments.\n");
			return NULL;
		}
		Cell * a = evaluate(list_index(this, arguments, 0)->cons.car);
		Cell * b = evaluate(list_index(this, arguments, 1)->cons.car);
		if (a == NULL || b == NULL) return NULL;
		assert(a->cell_type == CELL_NUMBER);
		assert(b->cell_type == CELL_NUMBER);
		if (a->number == b->number) {
			return truth;
		} else {
			return nil;
		}
	} else if (strcmp(symbol, "+") == 0) {
		if (list_length(this, arguments) != 2) {
			printf("+ takes two arguments.\n");
			return NULL;
		}
		Cell * a = evaluate(list_index(this, arguments, 0)->cons.car);
		Cell * b = evaluate(list_index(this, arguments, 1)->cons.car);
		if (a == NULL || b == NULL) return NULL;
		assert(a->cell_type == CELL_NUMBER);
		assert(b->cell_type == CELL_NUMBER);
		Cell * ret = alloc_cell();
		ret->cell_type = CELL_NUMBER;
		ret->number = a->number + b->number;
		return ret;
	} else if (strcmp(symbol, "*") == 0) {
		if (list_length(this, arguments) != 2) {
			printf("* takes two arguments.\n");
			return NULL;
		}
		Cell * a = evaluate(list_index(this, arguments, 0)->cons.car);
		Cell * b = evaluate(list_index(this, arguments, 1)->cons.car);
		if (a == NULL || b == NULL) return NULL;
		assert(a->cell_type == CELL_NUMBER);
		assert(b->cell_type == CELL_NUMBER);
		Cell * ret = alloc_cell();
		ret->cell_type = CELL_NUMBER;
		ret->number = a->number * b->number;
		return ret;
	}
	return NULL;
}

void Lisp_VM::substitute_arguments(Cell * procedure, char * param, Cell * arg)
{
	assert(procedure->cell_type == CELL_CONS);
	switch (procedure->cons.car->cell_type) {
	case CELL_CONS:
		substitute_arguments(procedure->cons.car, param, arg);
		break;
	case CELL_SYMBOL:
		if (strcmp(procedure->cons.car->symbol, param) == 0) {
			procedure->cons.car = deep_copy_cell(this, arg);
		}
		break;
	default:
		break;
	}
	if (procedure->cons.cdr != nil) {
		substitute_arguments(procedure->cons.cdr, param, arg);
	}
}

Cell * Lisp_VM::apply_function(Cell * to_call, Cell * arguments)
{
	/* 1. Copy procedure tree
	 * 2. Substitute arguments in procedure tree
	 * 3. Substitute function with their recursively
     *    applied bodies
	 * 4. Evaluate
	 */
	assert(to_call->cell_type == CELL_CONS);
	assert(to_call->cons.car->cell_type == CELL_SYMBOL);
	assert(strcmp(to_call->cons.car->symbol, "lambda") == 0);
	Cell * lambda = deep_copy_cell(this, to_call);
	Cell * procedure = lambda->cons.cdr->cons.cdr->cons.car;
	
	Cell * params = lambda->cons.cdr->cons.car;
	Cell * args   = arguments;

	assert(list_length(this, params) == list_length(this, args));
	
	for (int i = 0; i < list_length(this, params); i++) {
		Cell * param_cons = list_index(this, params, i);
		assert(param_cons->cell_type == CELL_CONS);
		assert(param_cons->cons.car->cell_type == CELL_SYMBOL);
		char * param = param_cons->cons.car->symbol;
		
		Cell * arg_cons = list_index(this, args, i);
		assert(arg_cons->cell_type == CELL_CONS);
		Cell * arg = deep_copy_cell(this, evaluate(arg_cons->cons.car));

		substitute_arguments(procedure, param, arg);
	}

	Cell * resolved = evaluate(procedure);
	
	return resolved;
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
		if (cell == nil) return nil;
		// Check for special forms
		if (cell->cons.car->cell_type == CELL_SYMBOL) {
			if (string_in_list(
					special_forms,
					SPECIAL_FORM_COUNT,
					cell->cons.car->symbol)) {
				return special_form(cell->cons.car, cell->cons.cdr);
			}
		}
		// Evaluate head and apply to body
		Cell * head = evaluate(cell->cons.car);
		if (head == NULL) return NULL;
		return apply_function(head, cell->cons.cdr);
	}
	return NULL;
}

int main(int argc, char ** argv)
{
	Lisp_VM __vm;
	Lisp_VM * vm = &__vm;
	vm->init();

	List<char*> start_inputs;
	start_inputs.alloc();
	
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			start_inputs.push(load_string_from_file(argv[i]));
		}
	}
	
	while (1) {
		char * source;
		if (start_inputs.len > 0) {
			source = start_inputs.pop();
		} else {
			printf("$ ");
			char source_buffer[512];
			fgets(source_buffer, 512, stdin);
			if (strcmp(source_buffer, "(quit)\n") == 0) break;
			source = source_buffer;
		}
		Cell * parsed = parse_source(vm, source);
		while (parsed != vm->nil) {
			Cell * evaluated = vm->evaluate(parsed->cons.car);
			if (evaluated != NULL) {
				print_cell_as_lisp(vm, evaluated);
				printf("\n");
			}
			parsed = parsed->cons.cdr;
		}
	}
}
