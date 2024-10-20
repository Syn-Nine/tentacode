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

	
	//-----------------------------------------------------------------------------
	Stmt* Declaration()
	{
		
		if (Check(TOKEN_DEF)) return Function("function");
		
		if (Match(8, TOKEN_VAR_I32, TOKEN_VAR_F32, TOKEN_VAR_STRING,
			TOKEN_VAR_VEC, TOKEN_VAR_MAP, TOKEN_VAR_ENUM, TOKEN_VAR_BOOL, TOKEN_DEF))
			return VarDeclaration();
		
		return Statement();
	}


	//-----------------------------------------------------------------------------
	Stmt* Statement()
	{
		if (Match(1, TOKEN_PRINT)) return PrintStatement();
		if (Match(1, TOKEN_PRINTLN)) return PrintStatement(true);
		if (Match(1, TOKEN_LEFT_BRACE)) return new BlockStmt(BlockStatement());
		if (Match(1, TOKEN_IF)) return IfStatement();
		if (Match(1, TOKEN_WHILE)) return WhileStatement();
		if (Match(1, TOKEN_FOR)) return ForStatement();
		if (Match(1, TOKEN_LOOP)) return LoopStatement();
		
		return ExpressionStatement();
	}


	//-----------------------------------------------------------------------------
	Stmt* Function(std::string kind)
	{
		Consume(TOKEN_DEF, "");

		// check for return type
		Token* rettype = nullptr;
		if (Check(TOKEN_VAR_I32) ||
			Check(TOKEN_VAR_F32) ||
			Check(TOKEN_VAR_BOOL) ||
			Check(TOKEN_VAR_STRING))
		{
			rettype = new Token(Previous());
			Advance();
		}
		
		if (!Consume(TOKEN_IDENTIFIER, "Expected " + kind + " name.")) return nullptr;
		Token* name = new Token(Previous());

		if (!Consume(TOKEN_LEFT_PAREN, "Expected '(' after " + kind + " name.")) return nullptr;
		
		TokenList params;
		if (!Check(TOKEN_RIGHT_PAREN))
		{
			do
			{
				if (Consume(TOKEN_IDENTIFIER, "Expect parameter name."))
				{
					params.push_back(Token(Previous()));
				}
			} while (Match(1, TOKEN_COMMA));
		}

		if (!Consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.")) return nullptr;

		if (!Consume(TOKEN_LEFT_BRACE, "Expected '{' before " + kind + " body.")) return nullptr;

		StmtList* body = BlockStatement();
		return new FunctionStmt(rettype, name, params, body);
	}
	
	
	//-----------------------------------------------------------------------------
	Stmt* VarDeclaration()
	{
		Token* type = new Token(Previous());

		Token* id;
		if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
		id = new Token(TOKEN_IDENTIFIER, Previous().Lexeme(), type->Line(), type->Filename());

		Expr* expr = nullptr;
		if (Match(1, TOKEN_EQUAL)) expr = Expression();

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) return nullptr;

		return new VarStmt(type, id, expr);
	}


	//-----------------------------------------------------------------------------
	Stmt* VarDeclarationForInRange()
	{
		Token* id = nullptr;
		if (Consume(TOKEN_IDENTIFIER, "Expected identifier."))
		{
			id = new Token(Previous());

			if (Consume(TOKEN_IN, "Expected 'in'."))
			{
				Expr* expr = Expression();
				if (EXPRESSION_RANGE == expr->GetType())
				{
					RangeExpr* r = (RangeExpr*)expr;
					if (TOKEN_DOT_DOT_EQUAL == r->Operator()->GetType())
					{
						// overwrite old range expression with one that increments the right side
						Expr* temp = new BinaryExpr(r->Right(), new Token(TOKEN_PLUS, "+", r->Operator()->Line(), r->Operator()->Filename()), new LiteralExpr(int32_t(1)));
						Expr* old = expr;
						expr = new RangeExpr(r->Left(), r->Operator(), temp);
						delete old;
					}
					return new VarStmt(new Token(TOKEN_VAR_I32, "i32", id->Line(), id->Filename()), id, expr);
				}
			}
		}
		
		return nullptr;
	}
	

	//-----------------------------------------------------------------------------
	Stmt* ForStatement()
	{
		Stmt* initializer = VarDeclarationForInRange();
		if (!initializer)
		{
			Error(Previous(), "Expected initializer after for.");
			return nullptr;
		}

		if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* body = Statement();
			if (body)
			{
				// build increment
				Expr* rhs = new LiteralExpr(int32_t(1));
				Token* var = ((VarStmt*)initializer)->Operator();
				Expr* lhs = new VariableExpr(new Token(*var), nullptr);
				Expr* binary = new BinaryExpr(lhs, new Token(TOKEN_PLUS, "+", var->Line(), var->Filename()), rhs);
				Expr* increment = new AssignExpr(var, binary);
					
				Expr* rangeLeft = ((RangeExpr*)((VarStmt*)initializer)->Expression())->Left();
				Expr* rangeRight = ((RangeExpr*)((VarStmt*)initializer)->Expression())->Right();

				// build condition
				Expr* condition = new BinaryExpr(lhs, new Token(TOKEN_LESS, "<", var->Line(), var->Filename()), rangeRight);

				// build while statement
				body = new WhileStmt(condition, body, increment);

				// build new initializer
				Stmt* newinit = new VarStmt(new Token(TOKEN_VAR_I32, "i32", var->Line(), var->Filename()), var, rangeLeft);

				// build outer block
				StmtList* list = new StmtList();
				list->push_back(newinit);
				list->push_back(body);
				body = new BlockStmt(list);

				return body;
			}
		}
		else
		{
			Error(Previous(), "Expected '{' after initializer.");
		}

		return nullptr;
	}

	//-----------------------------------------------------------------------------
	Stmt* LoopStatement()
	{
		/*if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* body = Statement();
			if (body)
			{
				std::string label = GenerateUUID();
				return new WhileStmt(new LiteralExpr(true), body, nullptr, label);
			}
		}
		else
		{
			Error(Previous(), "Expected '{' after loop.");
		}*/

		return nullptr;
	}

	
	//-----------------------------------------------------------------------------
	Stmt* IfStatement()
	{
		Expr* condition = Expression();

		if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* thenBranch = Statement();
			Stmt* elseBranch = nullptr;
			if (Match(1, TOKEN_ELSE))
			{
				if (Check(TOKEN_IF))
				{
					elseBranch = Statement();
				}
				else
				{
					if (Check(TOKEN_LEFT_BRACE))
					{
						elseBranch = Statement();
					}
					else
					{
						Error(Previous(), "Expected '{' after else condition.");
					}
				}
			}
			return new IfStmt(condition, thenBranch, elseBranch);
		}
		else
		{
			Error(Previous(), "Expected '{' after if condition.");
		}
		return nullptr;
	}


	//-----------------------------------------------------------------------------
	StmtList* BlockStatement()
	{
		StmtList* statements = new StmtList();

		while (!Check(TOKEN_RIGHT_BRACE) && !IsAtEnd())
		{
			statements->push_back(Declaration());
		}

		Consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
		return statements;
	}
	

	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
	Stmt* WhileStatement()
	{
		Expr* condition = Expression();
		
		if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* body = Statement();
			return new WhileStmt(condition, body, nullptr);
		}
		else
		{
			Error(Previous(), "Expected '{' after while condition.");
		}
		
		return nullptr;
	}


	//-----------------------------------------------------------------------------
	Stmt* ExpressionStatement()
	{
		Expr* expr = Expression();
		Consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
		return new ExpressionStmt(expr);
	}


	//-----------------------------------------------------------------------------
	Expr* Expression()
	{
		return Assignment();
	}


	//-----------------------------------------------------------------------------
	Expr* Assignment()
	{
		Expr* expr = Or();
		
		if (Match(1, TOKEN_EQUAL))
		{
			Token equals = Previous();
			Expr* value = Assignment();

			if (expr->GetType() == EXPRESSION_VARIABLE)
			{
				VariableExpr* v = (VariableExpr*)expr;
				Token* name = v->Operator();
				return new AssignExpr(name, value);
			}
			
			Error(Previous(), "Invalid assignment target.");
		}

		return expr;
	}


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
	Expr* Struct()
	{
		Expr* expr = Range();

		return expr;
	}


	//-----------------------------------------------------------------------------
	Expr* Range()
	{
		Expr* expr = Addition();

		while (Match(2, TOKEN_DOT_DOT, TOKEN_DOT_DOT_EQUAL))
		{
			Token* oper = new Token(Previous());
			Expr* right = Addition();
			expr = new RangeExpr(expr, oper, right);
		}

		return expr;
	}


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
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


	//-----------------------------------------------------------------------------
	Expr* As()
	{
		Expr* expr = Primary();

		while (Match(1, TOKEN_AS))
		{
			Token* oper = new Token(Previous());
			Expr* right = Primary();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}


	//-----------------------------------------------------------------------------
	Expr* Primary()
	{
		if (Match(1, TOKEN_FILELINE)) return new LiteralExpr(std::string("File:" + Previous().Filename() + ", Line:" + std::to_string(Previous().Line())));
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

		// used for AS syntax
		if (Match(4, TOKEN_VAR_I32, TOKEN_VAR_F32, TOKEN_VAR_STRING, TOKEN_VAR_ENUM)) return new VariableExpr(new Token(Previous()), nullptr);

		if (Match(1, TOKEN_IDENTIFIER))
		{
			Token prev = Previous();
			std::string name = prev.Lexeme();

			// check for bracket index
			Expr* vecIndex = nullptr;
			if (Match(1, TOKEN_LEFT_BRACKET))
			{
				vecIndex = Expression();
				Consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression.");
			}

			Expr* expr = new VariableExpr(new Token(TOKEN_IDENTIFIER, name, prev.Line(), prev.Filename()), vecIndex);

			// check for function call parenthesis
			while (true)
			{
				if (Match(1, TOKEN_LEFT_PAREN))
				{
					expr = FinishCall(expr);
				}
				else
				{
					break;
				}
			}

			return expr;
		}
		Error(Peek(), "Parser Error: Expected expression.");
		return nullptr;
	}
	

	//-----------------------------------------------------------------------------
	Expr* FinishCall(Expr* callee)
	{
		// similar to FinishFormat and FinishPair
		ArgList args;
		if (!Check(TOKEN_RIGHT_PAREN))
		{
			do
			{
				args.push_back(Expression());
			} while (Match(1, TOKEN_COMMA));
		}

		Token* paren = nullptr;
		if (Consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments."))
		{
			paren = new Token(Previous());
		}

		return new CallExpr(callee, paren, args);
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