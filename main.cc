#include "lex-parse.h"

#define DS_UTIL_IMPLEMENTATION
#include "ds_util.h"

int main(int argc, char ** argv)
{
	if (argc != 2) {
		printf("Provide one file to parse please.\n");
		return 1;
	}

	AST_Node * ast_root = get_ast(argv[1], true);
}
