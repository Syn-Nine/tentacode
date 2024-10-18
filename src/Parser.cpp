#include "Parser.h"
#include "Scanner.h"

//std::set<std::string> Parser::m_includes;

Token Parser::Advance()
{
	if (!IsAtEnd()) m_current++;
	return Previous();
}

bool Parser::Check(TokenTypeEnum tokenType)
{
	if (IsAtEnd()) return false;
	return Peek().GetType() == tokenType;
}

bool Parser::CheckNext(TokenTypeEnum tokenType)
{
	if (IsAtEnd()) return false;
	if (m_current + 1 == m_tokenList.size()) return false;
	TokenTypeEnum next = m_tokenList.at(m_current + 1).GetType();
	if (TOKEN_END_OF_FILE == next) return false;
	return next == tokenType;
}

bool Parser::CheckNextNext(TokenTypeEnum tokenType)
{
	if (IsAtEnd()) return false;
	if (m_current + 1 == m_tokenList.size()) return false;
	if (m_current + 2 == m_tokenList.size()) return false;
	TokenTypeEnum next = m_tokenList.at(m_current + 2).GetType();
	if (TOKEN_END_OF_FILE == next) return false;
	return next == tokenType;
}

bool Parser::Consume(TokenTypeEnum tokenType, std::string err)
{
	if (Check(tokenType))
	{
		Advance();
		return true;
	}
	else
	{
		Error(Peek(), "Parser Error: " + err);
		return false;
	}
}

void Parser::Error(Token token, const std::string& err)
{
	if (token.GetType() == TOKEN_END_OF_FILE)
	{
		m_errorHandler->Error(Peek().Filename(), Peek().Line(), "at end", err);
	}
	else
	{
		m_errorHandler->Error(Peek().Filename(), Peek().Line(), "at '" + token.Lexeme() + "'", err);
	}
}

bool Parser::IsAtEnd()
{
	return Peek().GetType() == TOKEN_END_OF_FILE;
}

bool Parser::Match(int count, ...)
{
	std::va_list args;
	va_start(args, count);
	for (size_t i = 0; i < count; ++i)
	{
		TokenTypeEnum t = (TokenTypeEnum)(va_arg(args, int));
		
		if (Check(t))
		{
			Advance();
			va_end(args);
			return true;
		}
	}

	va_end(args);
	return false;
}

Token Parser::Peek()
{
	return m_tokenList.at(m_current);
}

Token Parser::Previous()
{
	return m_tokenList.at(m_current - 1);
}