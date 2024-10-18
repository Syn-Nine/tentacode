#ifndef PARSER_H
#define PARSER_H

/*#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <fstream>
#include <set>*/

#include "Token.h"
#include "Expressions.h"
#include "ErrorHandler.h"
#include "Statements.h"

class Parser
{
public:
	Parser() = delete;
	Parser(TokenList tokenList, ErrorHandler* errorHandler)
	{
		m_tokenList = tokenList;
		m_current = 0;
		m_errorHandler = errorHandler;
	}

	StmtList Parse()
	{
		StmtList list;
		while (!IsAtEnd() && !m_errorHandler->HasErrors())
		{
			Stmt* stmt = Declaration();
			
			if (stmt)
			{
				list.push_back(stmt);
			}
			else
			{
				break;
			}
		}

		return list;
	}



private:

	Stmt* Declaration()
	{
		return Statement();
	}

	Stmt* Statement()
	{
		if (Match(1, TOKEN_PRINT)) return PrintStatement();
		if (Match(1, TOKEN_PRINTLN)) return PrintStatement(true);
		
		return ExpressionStatement();
	}

	Stmt* PrintStatement(bool newline = false)
	{
		if (Consume(TOKEN_LEFT_PAREN, "Expected '(' after print."))
		{
			Expr* expr = Expression();
			if (Consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.") && Consume(TOKEN_SEMICOLON, "Expected ';' after statement."))
			{
				return new PrintStmt(expr, newline);
			}
		}

		return nullptr;
	}

	Stmt* ExpressionStatement()
	{
		Expr* expr = Expression();
		Consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
		return new ExpressionStmt(expr);
	}

	Expr* Expression()
	{
		return Assignment();
	}

	Expr* Assignment()
	{
		Expr* expr = Or();
		
		return expr;
	}


	Expr* Or()
	{
		Expr* expr = And();

		while (Match(1, TOKEN_OR))
		{
			Token* op = new Token(Previous());
			Expr* right = And();
			expr = new LogicalExpr(expr, op, right);
		}

		return expr;
	}

	Expr* And()
	{
		Expr* expr = Equality();

		while (Match(1, TOKEN_AND))
		{
			Token* op = new Token(Previous());
			Expr* right = Equality();
			expr = new LogicalExpr(expr, op, right);
		}

		return expr;
	}

	Expr* Equality()
	{
		Expr* expr = Comparison();

		while (Match(2, TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL))
		{
			Token* oper = new Token(Previous());
			Expr* right = Comparison();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}

	Expr* Comparison()
	{
		Expr* expr = Struct();

		while (Match(4, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL))
		{
			Token* oper = new Token(Previous());
			Expr* right = Struct();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}

	Expr* Struct()
	{
		Expr* expr = Range();

		return expr;
	}

	Expr* Range()
	{
		Expr* expr = Addition();

		return expr;
	}

	Expr* Addition()
	{
		Expr* expr = Multiplication();
		
		while (Match(2, TOKEN_MINUS, TOKEN_PLUS))
		{
			Token* oper = new Token(Previous());
			Expr* right = Multiplication();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}


	Expr* Multiplication()
	{
		Expr* expr = Unary();

		while (Match(2, TOKEN_SLASH, TOKEN_STAR))
		{
			Token* oper = new Token(Previous());
			Expr* right = Unary();
			expr = new BinaryExpr(expr, oper, right);
		}
		
		return expr;
	}

	Expr* Unary()
	{
		if (Match(2, TOKEN_BANG, TOKEN_MINUS))
		{
			Token* oper = new Token(Previous());
			Expr* right = Unary();
			return new UnaryExpr(oper, right);
		}

		return Modulus();
	}


	Expr* Modulus()
	{
		Expr* expr = As();

		while (Match(1, TOKEN_PERCENT))
		{
			Token* oper = new Token(Previous());
			Expr* right = As();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}

	Expr* As()
	{
		Expr* expr = Primary();

		return expr;
	}

	Expr* Primary()
	{
		if (Match(1, TOKEN_FALSE)) return new LiteralExpr(false);
		if (Match(1, TOKEN_TRUE)) return new LiteralExpr(true);
		if (Match(1, TOKEN_FLOAT)) return new LiteralExpr(Previous().DoubleValue());
		if (Match(1, TOKEN_INTEGER)) return new LiteralExpr(Previous().IntValue());
		if (Match(1, TOKEN_STRING)) return new LiteralExpr(Previous().StringValue());

		if (Match(1, TOKEN_LEFT_PAREN))
		{
			Expr* expr = Expression();
			Consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
			return new GroupExpr(expr);
		}

		Error(Peek(), "Parser Error: Expected expression.");
		return nullptr;
	}


	Token Advance();
	bool Check(TokenTypeEnum tokenType);
	bool CheckNext(TokenTypeEnum tokenType);
	bool CheckNextNext(TokenTypeEnum tokenType);
	bool Consume(TokenTypeEnum tokenType, std::string err);
	bool IsAtEnd();
	bool Match(int count, ...);
	Token Peek();
	Token Previous();
	void Error(Token token, const std::string& err);

	ErrorHandler* m_errorHandler;
	TokenList m_tokenList;
	int m_current;
	
};

#endif // PARSER_H