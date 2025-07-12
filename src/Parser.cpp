#include "Parser.h"
#include "Scanner.h"

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

void Parser::Include()
{
	if (!Consume(TOKEN_STRING, "Expected filename after include.")) return;

	std::string filename = Previous().StringValue();

	std::string namespc;

	/*if (Match(1, TOKEN_AS))
	{
		if (!Consume(TOKEN_IDENTIFIER, "Expected identifier after as.")) return;
		namespc = Previous().Lexeme();
	}*/

	if (!Consume(TOKEN_SEMICOLON, "Expected ';' after include.")) return;

	// skip if already included
	if (0 != m_includes.count(filename))
	{
		Error(Previous(), filename + "Already included in project.");
		return;
	}

	int incLine = Previous().Line();

	//printf("Including file... %s\n", filename.c_str());

	std::ifstream f;
	f.open(filename, std::ios::in | std::ios::binary | std::ios::ate);

	if (!f.is_open())
	{
		printf("Failed to open file: '%s'\n", filename.c_str());
		return;
	}

	const std::size_t n = f.tellg();
	char* buffer = new char[n + 1];
	buffer[n] = '\0';

	f.seekg(0, std::ios::beg);
	f.read(buffer, n);
	f.close();

	const char* fname = filename.c_str();
	Scanner scanner(buffer, m_errorHandler, fname);
	TokenList tokens = scanner.ScanTokens();

	if (tokens.size() > 1)
	{
		/*if (!namespc.empty())
		{
			// wrap tokens in a namespace
			TokenList t;
			t.push_back(Token(TOKEN_NAMESPACE_PUSH, namespc, incLine, fname));
			t.insert(t.begin() + 1, tokens.begin(), tokens.begin() + tokens.size() - 1);
			t.push_back(Token(TOKEN_NAMESPACE_POP, namespc, incLine, fname));
			t.push_back(Token(TOKEN_END_OF_FILE, "", incLine, fname));
			tokens = t;
		}*/

		m_tokenList.insert(m_tokenList.begin() + m_current, tokens.begin(), tokens.begin() + tokens.size() - 1);
	}
}

bool Parser::IsAtEnd()
{
	return Peek().GetType() == TOKEN_END_OF_FILE;
}

bool Parser::Match_Vars()
{
	return Match(14,
		TOKEN_VAR_I16, TOKEN_VAR_I32, TOKEN_VAR_I64,
		TOKEN_VAR_F32, TOKEN_VAR_F64, TOKEN_VAR_STRING,
		TOKEN_VAR_VEC, TOKEN_VAR_MAP, TOKEN_VAR_TUPLE,
		TOKEN_VAR_SET, TOKEN_VAR_ENUM, TOKEN_VAR_BOOL,
		TOKEN_VAR_AUTO, TOKEN_AT);
}

bool Parser::Match_Vars_Raylib()
{
	return Match(6,
		TOKEN_VAR_FONT, TOKEN_VAR_IMAGE, TOKEN_VAR_RENDER_TEXTURE_2D,
		TOKEN_VAR_TEXTURE, TOKEN_VAR_SOUND, TOKEN_VAR_SHADER);
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


// evaluate compile time constant expressions
Expr* Parser::Evaluate(Expr* expr)
{
	ExpressionTypeEnum type = expr->GetType();
	if (EXPRESSION_BINARY == type) { return Evaluate_Binary(static_cast<BinaryExpr*>(expr)); }
	else if (EXPRESSION_GROUP == type) { return Evaluate(static_cast<GroupExpr*>(expr)->Expression()); }
	else if (EXPRESSION_UNARY == type) { return Evaluate_Unary(static_cast<UnaryExpr*>(expr)); }
	else if (EXPRESSION_VARIABLE == type) { return Evaluate_Variable(static_cast<VariableExpr*>(expr)); }
	
	// nothing we can do at compile time
	return expr;
}

Expr* Parser::Evaluate_Binary(BinaryExpr* expr)
{
	// todo allow non int evaluation
	// division probably will need 2 versions, one for int and one for float
	// - want to keep value as int for fixed vec sizing if possible
	
	if (!expr->Left()) return expr;
	Expr* lhs = Evaluate(expr->Left());
	if (EXPRESSION_LITERAL != lhs->GetType()) return expr;
	Literal lhs_val = static_cast<LiteralExpr*>(lhs)->GetLiteral();
	if (LITERAL_TYPE_INTEGER != lhs_val.GetType()) return expr;

	if (!expr->Right()) return expr;
	Expr* rhs = Evaluate(expr->Right());
	if (EXPRESSION_LITERAL != rhs->GetType()) return expr;
	Literal rhs_val = static_cast<LiteralExpr*>(rhs)->GetLiteral();
	if (LITERAL_TYPE_INTEGER != rhs_val.GetType()) return expr;

	Token* op = expr->Operator();

	switch (op->GetType())
	{
	case TOKEN_PLUS: return new LiteralExpr(expr->Operator(), lhs_val.IntValue() + rhs_val.IntValue());
	case TOKEN_MINUS: return new LiteralExpr(expr->Operator(), lhs_val.IntValue() - rhs_val.IntValue());
	case TOKEN_STAR: return new LiteralExpr(expr->Operator(), lhs_val.IntValue() * rhs_val.IntValue());
	case TOKEN_SLASH:
	{
		int32_t left = lhs_val.IntValue();
		int32_t right = rhs_val.IntValue();
		if (0 != right && 0 == left % right)
		{
			return new LiteralExpr(expr->Operator(), left / right);
		}
		break;
	}
	}

	// nothing we can do at compile time
	return expr;
}

Expr* Parser::Evaluate_Unary(UnaryExpr* expr)
{
	Token* op = expr->Operator();
	if (!expr->Right()) return expr;

	Expr* rhs = Evaluate(expr->Right());
	if (EXPRESSION_LITERAL == rhs->GetType())
	{
		Literal rhs_val = static_cast<LiteralExpr*>(rhs)->GetLiteral();
		if (TOKEN_MINUS == op->GetType())
		{
			if (LITERAL_TYPE_INTEGER == rhs_val.GetType())
			{
				return new LiteralExpr(expr->Operator(), -rhs_val.IntValue());
			}
			else if (LITERAL_TYPE_FLOAT == rhs_val.GetType())
			{
				return new LiteralExpr(expr->Operator(), -rhs_val.DoubleValue());
			}
		}
		else if (TOKEN_BANG == op->GetType())
		{
			if (LITERAL_TYPE_BOOL == rhs_val.GetType())
			{
				return new LiteralExpr(expr->Operator(), !rhs_val.BoolValue());
			}
		}
	}

	// nothing we can do at compile time
	return expr;
}

Expr* Parser::Evaluate_Variable(VariableExpr* expr)
{
	// check for existing constant variable definition
	std::string fqns = m_fqns;
	std::string var = expr->Operator()->Lexeme();

	while (true)
	{
		std::string temp_fqns = fqns;
		std::string temp_var = var;
		Environment::FixNamespacing(temp_fqns, temp_var);

		if (0 != m_const_vars.count(temp_fqns + ":" + temp_var))
		{
			return m_const_vars.at(temp_fqns + ":" + temp_var);
		}
		size_t idx = fqns.find_last_of(':');
		if (std::string::npos == idx) break;
		fqns = fqns.substr(0, idx);
	}

	return expr;
}