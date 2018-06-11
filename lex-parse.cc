#include "lex-parse.h"

/*
 * LEXER
 */

enum Token_Type {
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
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
			if (is_int_literal(word)) {
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

/*
 * PARSER
 */

struct Parser {
	Lisp_VM * vm;
	Token * tokens;
	int tokens_len;
	int cursor;
	void init(Lisp_VM * vm, Token * tokens, int tokens_len);
	Token * peek();
	Token * consume();
	Cell * parse();
};

void Parser::init(Lisp_VM * vm, Token * tokens, int tokens_len)
{
	this->vm = vm;
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

Cell * Parser::parse()
{
	Cell * this_cons = vm->nil;

	while (1) {
		Token * token = consume();
		if (token == NULL || token->type == TOKEN_CLOSE_PAREN) {
			break;
		} else if (token->type == TOKEN_IDENTIFIER) {
			Cell * symbol = alloc_cell();
			symbol->cell_type = CELL_SYMBOL;
			symbol->symbol = token->data.identifier;
			this_cons = cell_push(vm, this_cons, symbol);
		} else if (token->type == TOKEN_LITERAL) {
			Cell * number = alloc_cell();
			number->cell_type = CELL_NUMBER;
			number->number = token->data.literal;
			this_cons = cell_push(vm, this_cons, number);
		} else if (token->type == TOKEN_OPEN_PAREN) {
			Cell * cons = parse();
			this_cons = cell_push(vm, this_cons, cons);
		}
	}

	return this_cons;
}

Cell * parse_source(Lisp_VM * vm, char * source)
{
	List<Token> tokens;
	{
		Lexer lexer;
		lexer.init(source);
		tokens = lexer.lex();
	}
	
	Cell * parsed;
	{
		Parser parser;
		parser.init(vm, tokens.arr, tokens.len);
		parsed = parser.parse();
	}
	
	return parsed;
}
