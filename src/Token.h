#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <vector>
#include <stdint.h>

#include "Enums.h"
#include "Literal.h"

class Token
{
public:
	Token() = delete;
	Token(TokenTypeEnum type, std::string lexeme, int line, std::string filename)
	{
		m_type = type;
		m_lexeme = lexeme;
		m_line = line;
		m_filename = filename;
		if (TOKEN_ENUM == m_type) m_enumValue = EnumLiteral(lexeme);
	}

	Token(TokenTypeEnum type, std::string lexeme, std::string value, int line, std::string filename)
	{
		m_type = type;
		m_lexeme = lexeme;
		m_stringValue = value;
		m_line = line;
		m_filename = filename;
	}

	Token(TokenTypeEnum type, std::string lexeme, int32_t ival, double dval, int line, std::string filename)
	{
		m_type = type;
		m_lexeme = lexeme;
		m_intValue = ival;
		m_doubleValue = dval;
		m_line = line;
		m_filename = filename;
	}

	std::string ToString()
	{
		std::string type;
		switch (m_type)
		{
		// single character tokens.
		case TOKEN_LEFT_PAREN: type = "TOKEN_LEFT_PAREN"; break;
		case TOKEN_RIGHT_PAREN: type = "TOKEN_RIGHT_PAREN"; break;
		case TOKEN_LEFT_BRACE: type = "TOKEN_LEFT_BRACE"; break;
		case TOKEN_RIGHT_BRACE: type = "TOKEN_RIGHT_BRACE"; break;
		case TOKEN_LEFT_BRACKET: type = "TOKEN_LEFT_BRACKET"; break;
		case TOKEN_RIGHT_BRACKET: type = "TOKEN_RIGHT_BRACKET"; break;
		case TOKEN_COMMA: type = "TOKEN_COMMA"; break;
		case TOKEN_DOT_DOT: type = "TOKEN_DOT_DOT"; break;
		case TOKEN_DOT_DOT_EQUAL: type = "TOKEN_DOT_DOT_EQUAL"; break;
		case TOKEN_MINUS: type = "TOKEN_MINUS"; break;
		case TOKEN_PLUS: type = "TOKEN_PLUS"; break;
		case TOKEN_COLON: type = "TOKEN_COLON"; break;
		//case TOKEN_COLON_COLON: type = "TOKEN_COLON_COLON"; break;
		case TOKEN_SEMICOLON: type = "TOKEN_SEMICOLON"; break;
		case TOKEN_SLASH: type = "TOKEN_SLASH"; break;
		case TOKEN_STAR: type = "TOKEN_STAR"; break;

		// one or two character tokens
		case TOKEN_BANG: type = "TOKEN_BANG"; break;
		case TOKEN_BANG_EQUAL: type = "TOKEN_BANG_EQUAL"; break;
		case TOKEN_EQUAL: type = "TOKEN_EQUAL"; break;
		case TOKEN_EQUAL_EQUAL: type = "TOKEN_EQUAL_EQUAL"; break;
		case TOKEN_GREATER: type = "TOKEN_GREATER"; break;
		case TOKEN_GREATER_EQUAL: type = "TOKEN_GREATER_EQUAL"; break;
		case TOKEN_LESS: type = "TOKEN_LESS"; break;
		case TOKEN_LESS_EQUAL: type = "TOKEN_LESS_EQUAL"; break;

		// literals
		case TOKEN_IDENTIFIER: type = "TOKEN_IDENTIFIER"; break;
		case TOKEN_STRING: type = "TOKEN_STRING"; break;
		case TOKEN_INTEGER: type = "TOKEN_INTEGER"; break;
		case TOKEN_FLOAT: type = "TOKEN_FLOAT"; break;
		case TOKEN_ENUM: type = "TOKEN_ENUM"; break;

		// keywords
		case TOKEN_FALSE: type = "TOKEN_FALSE"; break;
		case TOKEN_TRUE: type = "TOKEN_TRUE"; break;
		case TOKEN_PRINT: type = "TOKEN_PRINT"; break;
		case TOKEN_PRINTLN: type = "TOKEN_PRINTLN"; break;
		case TOKEN_VAR_I32: type = "TOKEN_VAR_I32"; break;
		case TOKEN_VAR_F32: type = "TOKEN_VAR_F32"; break;
		case TOKEN_VAR_STRING: type = "TOKEN_VAR_STRING"; break;
		case TOKEN_VAR_VEC: type = "TOKEN_VAR_VEC"; break;
		case TOKEN_VAR_ENUM: type = "TOKEN_VAR_ENUM"; break;
		case TOKEN_VAR_BOOL: type = "TOKEN_VAR_BOOL"; break;
		case TOKEN_AND: type = "TOKEN_AND"; break;
		case TOKEN_ELSE: type = "TOKEN_ELSE"; break;
		case TOKEN_IF: type = "TOKEN_IF"; break;
		case TOKEN_OR: type = "TOKEN_OR"; break;
		case TOKEN_WHILE: type = "TOKEN_WHILE"; break;
		case TOKEN_FOR: type = "TOKEN_FOR"; break;
		case TOKEN_IN: type = "TOKEN_IN"; break;
		case TOKEN_AS: type = "TOKEN_AS"; break;
		case TOKEN_BREAK: type = "TOKEN_BREAK"; break;
		case TOKEN_CONTINUE: type = "TOKEN_CONTINUE"; break;
		case TOKEN_LOOP: type = "TOKEN_LOOP"; break;
		case TOKEN_DEF: type = "TOKEN_DEF"; break;
		case TOKEN_RETURN: type = "TOKEN_RETURN"; break;
		case TOKEN_STRUCT: type = "TOKEN_STRUCT"; break;
		case TOKEN_INCLUDE: type = "TOKEN_INCLUDE"; break;
		case TOKEN_INTERNAL: type = "TOKEN_INTERNAL"; break;
		//case TOKEN_GLOBAL: type = "TOKEN_GLOBAL"; break;
		case TOKEN_FILELINE: type = "TOKEN_FILELINE"; break;
		case TOKEN_CLEARENV: type = "TOKEN_CLEARENV"; break;

		// raylib custom
		case TOKEN_VAR_FONT: type = "TOKEN_VAR_FONT"; break;
		case TOKEN_VAR_IMAGE: type = "TOKEN_VAR_IMAGE"; break;
		case TOKEN_VAR_TEXTURE: type = "TOKEN_VAR_TEXTURE"; break;
		case TOKEN_VAR_SOUND: type = "TOKEN_VAR_SOUND"; break;

		//
		case TOKEN_END_OF_FILE: type = "TOKEN_END_OF_FILE"; break;
		}

		std::string val;
		if (TOKEN_STRING == m_type || TOKEN_ENUM) val = m_stringValue;
		if (TOKEN_INTEGER == m_type) val = std::to_string(m_intValue);
		if (TOKEN_FLOAT == m_type) val = std::to_string(m_doubleValue);

		return type + " " + m_lexeme + " " + val;
	}

	TokenTypeEnum GetType() { return m_type; }
	std::string Lexeme() { return m_lexeme; }
	double DoubleValue() { return m_doubleValue; }
	int32_t IntValue() { return m_intValue; }
	std::string StringValue() { return m_stringValue; }
	EnumLiteral EnumValue() { return m_enumValue; }
	int Line() { return m_line; }
	std::string Filename() { return m_filename; }

private:
	TokenTypeEnum m_type;
	std::string m_lexeme;
	int m_line;
	double m_doubleValue;
	int32_t m_intValue;
	std::string m_stringValue;
	EnumLiteral m_enumValue;
	std::string m_filename;
};

typedef std::vector<Token> TokenList;

#endif // TOKEN_H