#ifndef SCANNER_H
#define SCANNER_H

#include <vector>
#include <string>
#include <map>
#include <stdint.h>

#include "Enums.h"
#include "Token.h"
#include "ErrorHandler.h"

class Scanner
{
public:
	Scanner() = delete;
	Scanner(const char* buf, ErrorHandler* errorHandler, const char* filename)
	{
		m_buffer = std::string(buf);
		m_length = m_buffer.length();
		m_current = 0;
		m_start = 0;
		m_line = 1;
		m_errorHandler = errorHandler;
		m_filename = filename;

		m_keywordList.insert(std::make_pair("print", TOKEN_PRINT));
		m_keywordList.insert(std::make_pair("println", TOKEN_PRINTLN));
		m_keywordList.insert(std::make_pair("format", TOKEN_FORMAT));
		m_keywordList.insert(std::make_pair("false", TOKEN_FALSE));
		m_keywordList.insert(std::make_pair("true", TOKEN_TRUE));
		m_keywordList.insert(std::make_pair("i32", TOKEN_VAR_I32));
		m_keywordList.insert(std::make_pair("f32", TOKEN_VAR_F32));
		m_keywordList.insert(std::make_pair("vec", TOKEN_VAR_VEC));
		m_keywordList.insert(std::make_pair("enum", TOKEN_VAR_ENUM));
		m_keywordList.insert(std::make_pair("string", TOKEN_VAR_STRING));
		m_keywordList.insert(std::make_pair("bool", TOKEN_VAR_BOOL));
		m_keywordList.insert(std::make_pair("else", TOKEN_ELSE));
		m_keywordList.insert(std::make_pair("if", TOKEN_IF));
		m_keywordList.insert(std::make_pair("while", TOKEN_WHILE));
		m_keywordList.insert(std::make_pair("for", TOKEN_FOR));
		m_keywordList.insert(std::make_pair("in", TOKEN_IN));
		m_keywordList.insert(std::make_pair("as", TOKEN_AS));
		m_keywordList.insert(std::make_pair("break", TOKEN_BREAK));
		m_keywordList.insert(std::make_pair("continue", TOKEN_CONTINUE));
		m_keywordList.insert(std::make_pair("loop", TOKEN_LOOP));
		m_keywordList.insert(std::make_pair("def", TOKEN_DEF));
		m_keywordList.insert(std::make_pair("return", TOKEN_RETURN));
		m_keywordList.insert(std::make_pair("struct", TOKEN_STRUCT));
		m_keywordList.insert(std::make_pair("include", TOKEN_INCLUDE));
		m_keywordList.insert(std::make_pair("internal", TOKEN_INTERNAL));
		//m_keywordList.insert(std::make_pair("global", TOKEN_GLOBAL));
		m_keywordList.insert(std::make_pair("FILELINE", TOKEN_FILELINE));
		m_keywordList.insert(std::make_pair("CLEARENV", TOKEN_CLEARENV));
		

		// raylib custom
		m_keywordList.insert(std::make_pair("ray_font", TOKEN_VAR_FONT));
		m_keywordList.insert(std::make_pair("ray_image", TOKEN_VAR_IMAGE));
		m_keywordList.insert(std::make_pair("ray_sound", TOKEN_VAR_SOUND));
		m_keywordList.insert(std::make_pair("ray_shader", TOKEN_VAR_SHADER));
		m_keywordList.insert(std::make_pair("ray_texture", TOKEN_VAR_TEXTURE));
		m_keywordList.insert(std::make_pair("ray_renderTexture2D", TOKEN_VAR_RENDER_TEXTURE_2D));
	}

	TokenList ScanTokens();

private:
	void AddToken(TokenTypeEnum type);
	void AddToken(TokenTypeEnum type, std::string value);
	void AddToken(TokenTypeEnum type, int32_t value);
	void AddToken(TokenTypeEnum type, double value);

	char Advance();
	void Enum();
	void Identifier();
	bool IsAlpha(char c);
	bool IsAlphaNumeric(char c);
	bool IsAtEnd();
	bool IsDigit(char c);
	bool IsColon(char c);
	bool Match(char c);
	void Number();
	char Peek();
	char PeekNext();
	void ScanString();
	void ScanToken();

	std::string m_buffer;
	int m_length;
	int m_current;
	int m_start;
	int m_line;
	std::string m_filename;

	TokenList m_tokens;
	ErrorHandler* m_errorHandler;
	std::map<std::string, TokenTypeEnum> m_keywordList;
};


#endif // SCANNER_H