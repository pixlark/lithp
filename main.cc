#include "lex-parse.h"

#define DS_UTIL_IMPLEMENTATION
#include "ds_util.h"

struct Lisp_VM {
	void init();
};

void Lisp_VM::init()
{
	// ...
}

int main(int argc, char ** argv)
{
#if 0
	if (argc != 2) {
		printf("Provide one file to parse please.\n");
		return 1;
	}
#endif

	Lisp_VM lisp_vm;
	lisp_vm.init();

	while (1) {
		printf("$ ");
		char source_buffer[512];
		fgets(source_buffer, 512, stdin); // WARNING: Unsafe
		AST_Node * ast_root = get_ast(source_buffer, true);
	}
}
