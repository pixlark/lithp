#ifndef LITHP_VM_H
#define LITHP_VM_H

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
	Cell * nil;

	void   init();
	Cell * evaluate(Cell * cell);
};

Cell * alloc_cell();
void print_cell_as_lisp(Lisp_VM * vm, Cell * cell, bool first_cons = true);
Cell * cell_push(Lisp_VM * vm, Cell * cell, Cell * to_push);

#endif
