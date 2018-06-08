#include "lex-parse.h"

enum Token_Type {
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_T,
	TOKEN_IDENTIFIER,
	TOKEN_LITERAL,
};

union Token_Data {
	char * identifier;
	int literal;
};

struct Token {
	Token_Type type;
	Token_Data data;
	Token() { }
	Token(Token_Type type) : type(type) { }
};

void print_token(Token token)
{
	switch (token.type) {
	case TOKEN_OPEN_PAREN:
		printf("Open Paren (");
		break;
	case TOKEN_CLOSE_PAREN:
		printf("Close Paren )");
		break;
	case TOKEN_IDENTIFIER:
		printf("Identifier: %s", token.data.identifier);
		break;
	case TOKEN_LITERAL:
		printf("Literal: %d", token.data.literal);
		break;
	case TOKEN_T:
		printf("Boolean True");
		break;
	}
}

struct Lexer {
	char * source;
	int source_len;
	int cursor;

	void init(char * source);
	char consume();
	char peek();
	char * read_word();
	List<Token> lex();
};

void Lexer::init(char * source)
{
	this->source = source;
	source_len = strlen(source);
	cursor = 0;
}

char Lexer::consume()
{
	if (cursor >= source_len) return EOF;
	return source[cursor++];
}

char Lexer::peek()
{
	if (cursor >= source_len) return EOF;
	return source[cursor];
}

bool is_alpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool is_numeric(char c)
{
	return c >= '0' && c <= '9';
}

bool is_int_literal(char * word)
{
	if (*word == '+' || *word == '-') {
		word++;
		if (*word == '\0') return false;
	}
	while (*word != '\0') {
		if (!is_numeric(*word)) return false;
		word++;
	}
	return true;
}

bool is_whitespace(char c)
{
	return c == ' ' || c == '\t' || c == '\n';
}

char * Lexer::read_word()
{
	char buffer[512];
	int buffer_i = 0;
	while (
		peek() != '(' &&
		peek() != ')' &&
		!is_whitespace(peek()) &&
		peek() != EOF) {
		buffer[buffer_i++] = consume();
	}
	char * word = (char*) malloc(buffer_i + 1);
	word[buffer_i] = '\0';
	for (int i = 0; i < buffer_i; i++) word[i] = buffer[i];
	return word;
}

List<Token> Lexer::lex()
{
	List<Token> tokens;
	tokens.alloc();

	while (1) {
		char this_char = peek();
		if (this_char == EOF) {
			break;
		} else if (this_char == '(') {
			tokens.push(Token(TOKEN_OPEN_PAREN));
			consume();
		} else if (this_char == ')') {
			tokens.push(Token(TOKEN_CLOSE_PAREN));
			consume();
		} else if (is_whitespace(this_char)) {
			consume();
		} else {
			char * word = read_word();
			if (strcmp(word, "T") == 0) {
				Token t(TOKEN_T);
				free(word);
				tokens.push(t);
			} else if (is_int_literal(word)) {
				Token t(TOKEN_LITERAL);
				t.data.literal = atoi(word);
				free(word);
				tokens.push(t);
			} else {
				Token t(TOKEN_IDENTIFIER);
				t.data.identifier = word;
				tokens.push(t);
			}
		}
	}
	return tokens;
}

int next_debug_id() { static int i = 1000; return i++; }

char * ast_type_string[] = {
	"Identifier",
	"Literal",
	"List",
	"T",
};

void AST_Node::init(AST_Type type, bool root_node)
{
	this->type = type;
	next = NULL;
	// debug stuff
	this->root_node = root_node;
	debug_id     = next_debug_id();
	next_nil_id  = next_debug_id();
	child_nil_id = next_debug_id();
	//
	switch (type) {
	case AST_LIST:
		list_head = NULL;
		break;
	case AST_IDENT:
		identifier = NULL;
		break;
	case AST_LITERAL:
		literal = 0;
		break;
	}
}

void AST_Node::list_add_child(AST_Node * child)
{
	assert(type == AST_LIST);
	if (list_head == NULL) {
		list_head = child;
		return;
	}
	AST_Node * iter = list_head;
	while (iter->next != NULL) {
		iter = iter->next;
	}
	iter->next = child;
}

char * AST_Node::debug_info()
{
	switch (type) {
	case AST_LIST: {
		if (root_node) {
			char * str = (char*) malloc(strlen("ROOT") + 1);
			strcpy(str, "ROOT");
			return str;
		} else {
			char * str = (char*) malloc(strlen("list") + 1);
			strcpy(str, "list");
			return str;
		}
	}
	case AST_IDENT: {
		char buffer[512];
		sprintf(buffer, "ident: %s", identifier);
		char * str = (char*) malloc(strlen(buffer) + 1);
		str[strlen(buffer)] = '\0';
		for (int i = 0; i < strlen(buffer); i++) str[i] = buffer[i];
		return str;
	}
	case AST_LITERAL: {
		char buffer[512];
		sprintf(buffer, "literal: %d", literal);
		char * str = (char*) malloc(strlen(buffer) + 1);
		str[strlen(buffer)] = '\0';
		for (int i = 0; i < strlen(buffer); i++) str[i] = buffer[i];
		return str;
	}
	case AST_T: {
		char * str = (char*) malloc(2);
		strcpy(str, "T");
		return str;
	}
	}
}

void AST_Node::graph_viz_repr(FILE * file)
{
	List<int> group_ids;
	group_ids.alloc();
	defer { group_ids.dealloc(); };
	AST_Node * iter = this;
	while (iter != NULL) {
		group_ids.push(iter->debug_id);
		
		// Declare nil if the next member of the list is nil
		if (iter->next == NULL) {
			group_ids.push(iter->next_nil_id);
			char buffer[512];
			sprintf(buffer, "\"%d\" [label=\"NIL\"]\n", iter->next_nil_id);
			fwrite(buffer, sizeof(char), strlen(buffer), file);
		}
		// Declare nil if this is a list and the child is nil
		if (iter->type == AST_LIST && iter->list_head == NULL) {
			char buffer[512];
			sprintf(buffer, "\"%d\" [label=\"NIL\"]\n", iter->child_nil_id);
			fwrite(buffer, sizeof(char), strlen(buffer), file);
		}
		
		// Declare this member of the list
		{ 
			char buffer[512];
			char * debug_info_str = iter->debug_info();
			defer { free(debug_info_str); };
			sprintf(
				buffer, "\"%d\" [label=\"%s\"]\n",
				iter->debug_id, debug_info_str);
			fwrite(buffer, sizeof(char), strlen(buffer), file);
		}
		
		// Declare the connection to the next member of the list
		if (iter->next != NULL) {
			char buffer[512];
			sprintf(buffer, "\"%d\" -> \"%d\"\n",
				iter->debug_id, iter->next->debug_id);
			fwrite(buffer, sizeof(char), strlen(buffer), file);
		} else {
			char buffer[512];
			sprintf(buffer, "\"%d\" -> \"%d\"\n", iter->debug_id, iter->next_nil_id);
			fwrite(buffer, sizeof(char), strlen(buffer), file);
		}

		// Declare the connection to children if this is a list
		if (iter->type == AST_LIST) {
			if (iter->list_head == NULL) {
				char buffer[512];
				sprintf(buffer, "\"%d\" -> \"%d\"\n",
					iter->debug_id, iter->child_nil_id);
				fwrite(buffer, sizeof(char), strlen(buffer), file);
			} else {
				char buffer[512];
				sprintf(buffer, "\"%d\" -> \"%d\"\n",
					iter->debug_id, iter->list_head->debug_id);
				fwrite(buffer, sizeof(char), strlen(buffer), file);
				iter->list_head->graph_viz_repr(file);
			}
		}
		
		iter = iter->next;
	}
	String_Builder group_builder;
	group_builder.alloc();
	defer { group_builder.dealloc(); };
	for (int i = 0; i < group_ids.len; i++) {
		char buffer[512];
		sprintf(buffer, "\"%d\" ", group_ids[i]);
		group_builder.append(buffer);
	}
	group_builder.prepend("{ rank=same; ");
	group_builder.append ("}\n");
	fwrite(group_builder.str(), sizeof(char), strlen(group_builder.str()), file);
}

AST_Node * alloc_node(AST_Type type)
{
	AST_Node * node = (AST_Node*) malloc(sizeof(AST_Node));
	node->init(type);
	return node;
}

struct Parser {
	Token * tokens;
	int tokens_len;
	int cursor;
	void init(Token * tokens, int tokens_len);
	Token * peek();
	Token * consume();
	AST_Node * parse();
};

void Parser::init(Token * tokens, int tokens_len)
{
	this->tokens = tokens;
	this->tokens_len = tokens_len;
	cursor = 0;
}

Token * Parser::peek()
{
	if (cursor >= tokens_len) return NULL;
	return tokens + cursor;
}

Token * Parser::consume()
{
	if (cursor >= tokens_len) return NULL;
	return tokens + cursor++;
}

AST_Node * Parser::parse()
{
	AST_Node * parent_list = alloc_node(AST_LIST);
	while (1) {
		Token * token = consume();
		if (token == NULL) {
			break;
		} else if (token->type == TOKEN_IDENTIFIER) {
			AST_Node * ident_node = alloc_node(AST_IDENT);
			ident_node->identifier = token->data.identifier;
			parent_list->list_add_child(ident_node);
		} else if (token->type == TOKEN_LITERAL) {
			AST_Node * literal_node = alloc_node(AST_LITERAL);
			literal_node->literal = token->data.literal;
			parent_list->list_add_child(literal_node);
		} else if (token->type == TOKEN_OPEN_PAREN) {
			AST_Node * list_node = parse();
			parent_list->list_add_child(list_node);
		} else if (token->type == TOKEN_CLOSE_PAREN) {
			break;
		} else if (token->type == TOKEN_T) {
			AST_Node * t_node = alloc_node(AST_T);
			parent_list->list_add_child(t_node);
		}
	}
	return parent_list;
}

void print_ast_as_lisp(AST_Node * root)
{
	if (root->type == AST_LIST) {
		printf("(");
		AST_Node * iter = root->list_head;
		while (iter != NULL) {
			print_ast_as_lisp(iter);
			if (iter->next != NULL) printf(" ");
			iter = iter->next;
		}
		printf(")");
	} else if (root->type == AST_LITERAL) {
		printf("%d", root->literal);
	} else if (root->type == AST_IDENT) {
		printf("%s", root->identifier);
	} else if (root->type == AST_T) {
		printf("T");
	}
}

void write_gvr_and_view(AST_Node * root)
{	
	FILE * repr_file = fopen("graph.dot", "w");

	fwrite("digraph G {\n", sizeof(char), strlen("digraph G {\n"), repr_file);
	root->graph_viz_repr(repr_file);
	fwrite("}\n", sizeof(char), strlen("}\n"), repr_file);

	fclose(repr_file);

	system("python3 view_graph.py");
}

AST_Node * get_ast(char * source)
{	
	List<Token> tokens;
	{
		Lexer lexer;
		lexer.init(source);
		tokens = lexer.lex();
	}

	for (int i = 0; i < tokens.len; i++) {
		//print_token(tokens[i]);
		//printf("\n");
	}

	AST_Node * root;
	{
		Parser parser;
		parser.init(tokens.arr, tokens.len);
		root = parser.parse();
		root->root_node = true;
	}
	
	return root;
}
