#ifndef LITHP_VM_H
#define LITHP_VM_H

#include <stdarg.h>
#include "ds_util.h"

enum Cell_Type {
	CELL_SYMBOL,
	CELL_NUMBER,
	CELL_CONS,
};

struct Cell {
	Cell_Type cell_type;
	union {
		char * symbol;
		int    number;
		struct {
			Cell * car;
			Cell * cdr;
		} cons;
	};
	char * _debug_tag;
};

struct Lisp_VM {
	HashTable<char*, Cell*> bindings;
	Cell * truth;
	Cell * nil;

	char * err_str;
	bool thrown;

	void   init();
	Cell * evaluate(Cell * cell);
	Cell * special_form(Cell * form, Cell * arguments);
	Cell * apply_function(Cell * to_call, Cell * arguments);
	void substitute_arguments(Cell * procedure, char * param, Cell * arg);
	Cell * throw_error(char * format, ...); // returns NULL for convenience
	void display_error();
};

Cell * alloc_cell();
void print_cell_as_lisp(Lisp_VM * vm, Cell * cell, bool first_cons = true);

Cell * cell_push(Lisp_VM * vm, Cell * cell, Cell * to_push);
int    list_length(Lisp_VM * vm, Cell * cell);
Cell * list_index(Lisp_VM * vm, Cell * start, int index);

Cell * deep_copy_cell(Lisp_VM * vm, Cell * to_copy);

#endif
