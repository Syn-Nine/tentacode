#ifndef ENUMS_H
#define ENUMS_H

#include <vector>

enum TokenTypeEnum
{
	// single character tokens.
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_DOT_DOT,
	TOKEN_DOT_DOT_EQUAL,
	TOKEN_MINUS,
	TOKEN_PLUS,
	TOKEN_COLON,
	//TOKEN_COLON_COLON,
	TOKEN_SEMICOLON,
	TOKEN_SLASH,
	TOKEN_STAR,
	TOKEN_PERCENT,
	TOKEN_AT,

	// one or two character tokens
	TOKEN_BANG,
	TOKEN_BANG_EQUAL,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,

	// literals
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_INTEGER,
	TOKEN_FLOAT,
	TOKEN_ENUM,

	// keywords
	TOKEN_CLEARENV,
	TOKEN_FILELINE,
	TOKEN_FALSE,
	TOKEN_TRUE,
	TOKEN_PRINT,
	TOKEN_PRINTLN,
	TOKEN_FORMAT,
	TOKEN_VAR_I32,
	TOKEN_VAR_F32,
	TOKEN_VAR_STRING,
	TOKEN_VAR_VEC,
	TOKEN_VAR_ENUM,
	TOKEN_VAR_BOOL,
	TOKEN_IF,
	TOKEN_AND,
	TOKEN_ELSE,
	TOKEN_OR,
	TOKEN_WHILE,
	TOKEN_FOR,
	TOKEN_IN,
	TOKEN_AS,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_LOOP,
	TOKEN_DEF,
	TOKEN_RETURN,
	TOKEN_STRUCT,
	TOKEN_INCLUDE,
	TOKEN_INTERNAL,
	//TOKEN_GLOBAL,
	TOKEN_NAMESPACE_PUSH,
	TOKEN_NAMESPACE_POP,

	// raylib custom objects
	TOKEN_VAR_IMAGE,
	TOKEN_VAR_TEXTURE,
	TOKEN_VAR_FONT,
	TOKEN_VAR_SOUND,
	TOKEN_VAR_SHADER,
	TOKEN_VAR_RENDER_TEXTURE_2D,
	
	//
	TOKEN_END_OF_FILE,
};

enum ExpressionTypeEnum
{
	EXPRESSION_ASSIGN,
	EXPRESSION_BINARY,
	EXPRESSION_CALL,
	EXPRESSION_GROUP,
	EXPRESSION_BRACKET,
	EXPRESSION_LITERAL,
	EXPRESSION_LOGICAL,
	EXPRESSION_UNARY,
	EXPRESSION_VARIABLE,
	EXPRESSION_RANGE,
	EXPRESSION_REPLICATE,
	EXPRESSION_STRUCTURE,
	EXPRESSION_DESTRUCTURE,
	EXPRESSION_GET,
	EXPRESSION_SET,
	EXPRESSION_FUNCTOR,
	EXPRESSION_FORMAT,
};

enum StatementTypeEnum
{
	STATEMENT_BLOCK,
	STATEMENT_EXPRESSION,
	STATEMENT_PRINT,
	STATEMENT_CLEARENV,
	STATEMENT_PRINTLN,
	STATEMENT_VAR,
	STATEMENT_IF,
	STATEMENT_WHILE,
	STATEMENT_FUNCTION,
	STATEMENT_BREAK,
	STATEMENT_CONTINUE,
	STATEMENT_DESTRUCT,
	STATEMENT_RETURN,
	STATEMENT_STRUCT,
};

#endif // ENUMS_H