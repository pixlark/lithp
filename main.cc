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

#define SPECIAL_FORM_COUNT 2
char * special_forms[SPECIAL_FORM_COUNT] = {
	"if",
	"=",
};

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
	if (strcmp(symbol, "if") == 0) {
		if (list_length(this, arguments) != 3) {
			printf("if takes three arguments.\n");
			return NULL;
		}
		Cell * condition = evaluate(list_index(this, arguments, 0)->cons.car);
		if (condition == truth) {
			return list_index(this, arguments, 1)->cons.car;
		} else if (condition == nil) {
			return list_index(this, arguments, 2)->cons.car;
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
	}
	return NULL;
}

Cell * Lisp_VM::apply_function(Cell * to_call, Cell * arguments)
{
	return NULL;
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
		assert(cell->cons.car->cell_type == CELL_SYMBOL);
		if (string_in_list(
				special_forms,
				SPECIAL_FORM_COUNT,
				cell->cons.car->symbol)) {
			return special_form(cell->cons.car, cell->cons.cdr);
		}
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
