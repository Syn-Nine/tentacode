#ifndef PARSER_H
#define PARSER_H

#include <cstdarg>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <fstream>
#include <set>

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
		m_internal = false;
		//m_global = false;
		m_namespace.push_back("global");
		UpdateFQNS();
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

	void UpdateFQNS()
	{
		m_fqns = "";
		for (auto& s : m_namespace)
		{
			m_fqns.append(s + "::");
		}
	}

	void Print(const StmtList& list)
	{
		// AST pretty? printer
		for (auto& s : list)
		{
			if (STATEMENT_BLOCK == s->GetType())
			{
				Print(*((BlockStmt*)s)->GetBlock());
			}
			else
			{
				printf("  %s\n", PrintExpr(s->Expression()).c_str());
			}
		}
	}

	std::string PrintExpr(Expr* expr)
	{
		if (nullptr == expr) return "(error)";

		std::string ret = "";

		switch (expr->GetType())
		{
		case EXPRESSION_BINARY:
		{
			BinaryExpr* ex = (BinaryExpr*)expr;
			Expr* left = ex->Left();
			Token* token = ex->Operator();
			Expr* right = ex->Right();

			ret.append("( ");
			ret.append(token->Lexeme() + " ");
			ret.append(PrintExpr(left) + " ");
			ret.append(PrintExpr(right) + " ");
			ret.append(")");
			break;
		}
		case EXPRESSION_GROUP:
		{
			GroupExpr* ex = (GroupExpr*)expr;
			Expr* right = ex->Expression();

			ret.append("( ");
			ret.append("group ");
			ret.append(PrintExpr(right) + " ");
			ret.append(")");
			break;
		}
		case EXPRESSION_LITERAL:
		{
			LiteralExpr* ex = (LiteralExpr*)expr;
			ret.append(ex->GetLiteral().ToString() + " ");
			break;
		}
		case EXPRESSION_UNARY:
		{
			UnaryExpr* ex = (UnaryExpr*)expr;
			Token* token = ex->Operator();
			Expr* right = ex->Right();

			ret.append("( ");
			ret.append(token->Lexeme() + " ");
			ret.append(PrintExpr(right) + " ");
			ret.append(")");
			break;
		}
		case EXPRESSION_ASSIGN:
		{
			AssignExpr* ex = (AssignExpr*)expr;
			Token* token = ex->Operator();
			Expr* right = ex->Right();

			ret.append("( ");
			ret.append("= ");
			ret.append(token->Lexeme() + " ");
			ret.append(PrintExpr(right) + " ");
			ret.append(")");
			break;
		}
		case EXPRESSION_VARIABLE:
		{
			VariableExpr* ex = (VariableExpr*)expr;
			Token* token = ex->Operator();
			ret.append(token->Lexeme() + " ");
			break;
		}
		}

		return ret;
	}



private:

	Stmt* Declaration()
	{
		bool keep_going = true;
		while (keep_going)
		{
			keep_going = false;
			if (Match(1, TOKEN_INCLUDE))
			{
				Include();
				keep_going = true;
			}

			if (Match(1, TOKEN_NAMESPACE_PUSH))
			{
				m_namespace.push_back(Previous().Lexeme());
				UpdateFQNS();
				keep_going = true;
			}

			if (Match(1, TOKEN_NAMESPACE_POP))
			{
				if (m_namespace.empty())
				{
					Error(Previous(), "Attempting to pop from empty namespace stack.");
					return nullptr;
				}
				if (Previous().Lexeme() != m_namespace.back())
				{
					Error(Previous(), "Attempting to pop invalid namespace: " + Previous().Lexeme());
					return nullptr;
				}
				m_namespace.pop_back();
				UpdateFQNS();
				keep_going = true;
			}
		}

		m_internal = false;
		if (Match(1, TOKEN_INTERNAL)) m_internal = true;

		//m_global = false;
		//if (Match(1, TOKEN_GLOBAL)) m_global = true;
		
		if (Match(1, TOKEN_STRUCT)) return StructDeclaration();

		if (Match(1, TOKEN_DEF)) return Function("function");
		
		if (Match(6, TOKEN_VAR_I32, TOKEN_VAR_F32, TOKEN_VAR_STRING,
			TOKEN_VAR_VEC, TOKEN_VAR_ENUM, TOKEN_VAR_BOOL))
			return VarDeclaration();
		
		// raylib custom
		if (Match(4, TOKEN_VAR_FONT, TOKEN_VAR_IMAGE, TOKEN_VAR_TEXTURE, TOKEN_VAR_SOUND))
			return VarDeclaration();

		// udt declaration
		if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_IDENTIFIER))
			return UdtDeclaration();

		return Statement();
	}

	Stmt* Statement()
	{
		if (Match(1, TOKEN_CLEARENV)) return ClearEnvStatement();
		if (Match(1, TOKEN_PRINT)) return PrintStatement();
		if (Match(1, TOKEN_PRINTLN)) return PrintStatement(true);
		if (Match(1, TOKEN_LEFT_BRACE)) return new BlockStmt(BlockStatement());
		if (Match(1, TOKEN_IF)) return IfStatement();
		if (Match(1, TOKEN_WHILE)) return WhileStatement();
		if (Match(1, TOKEN_FOR)) return ForStatement();
		if (Match(1, TOKEN_LOOP)) return LoopStatement();
		if (Match(1, TOKEN_BREAK)) return BreakStatement();
		if (Match(1, TOKEN_CONTINUE)) return ContinueStatement();
		if (Match(1, TOKEN_RETURN)) return ReturnStatement();
		
		return ExpressionStatement();
	}

	void Include();

	Stmt* Function(std::string kind)
	{
		std::string fqns = m_fqns;
		//if (m_global) fqns = "global::";

		bool internal = m_internal;
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
		return new FunctionStmt(name, params, body, fqns, internal);
	}
	
	Stmt* StructDeclaration()
	{
		std::string fqns = m_fqns;
		//if (m_global) fqns = "global::";

		bool internal = m_internal;
		if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
		Token* id = new Token(Previous());

		if (!Consume(TOKEN_LEFT_BRACE, "Expected { for struct definition.")) return nullptr;

		StmtList* vars = new StmtList();
		while (!Check(TOKEN_RIGHT_BRACE) && !IsAtEnd())
		{
			if (Match(10, TOKEN_VAR_I32, TOKEN_VAR_F32, TOKEN_VAR_STRING,
				TOKEN_VAR_VEC, TOKEN_VAR_ENUM, TOKEN_VAR_BOOL,

				// raylib custom
				TOKEN_VAR_FONT, TOKEN_VAR_IMAGE, TOKEN_VAR_TEXTURE, TOKEN_VAR_SOUND))
			{
				vars->push_back(VarDeclaration());
			}
			else if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_IDENTIFIER))
			{
				vars->push_back(UdtDeclaration());
			}
			else
			{
				Error(Previous(), "Invalid variable type inside structure definition.");
				return nullptr;
			}
		}

		if (!Consume(TOKEN_RIGHT_BRACE, "Expected '}' after struct definition.")) return nullptr;
		//if (!Consume(TOKEN_SEMICOLON, "Expected ';' after struct definition.")) return nullptr;

		return new StructStmt(id, vars, fqns, internal);
	}


	Stmt* VarDeclaration()
	{
		std::string fqns = m_fqns;
		//if (m_global) fqns = "global::";

		bool internal = m_internal;
		Token* type = new Token(Previous());

		Literal::LiteralTypeEnum vtype = Literal::LITERAL_TYPE_INVALID; 
		if (TOKEN_VAR_VEC == type->GetType())
		{
			if (Consume(TOKEN_LESS, "Expected <type> after vec."))
			{
				if (Match(5, TOKEN_VAR_BOOL, TOKEN_VAR_I32, TOKEN_VAR_F32,
					TOKEN_VAR_STRING, TOKEN_VAR_ENUM))
				{
					TokenTypeEnum prevType = Previous().GetType();
					if (TOKEN_VAR_BOOL == prevType)
						vtype = Literal::LITERAL_TYPE_BOOL;
					else if (TOKEN_VAR_I32 == prevType)
						vtype = Literal::LITERAL_TYPE_INTEGER;
					else if (TOKEN_VAR_F32 == prevType)
						vtype = Literal::LITERAL_TYPE_DOUBLE;
					else if (TOKEN_VAR_STRING == prevType)
						vtype = Literal::LITERAL_TYPE_STRING;
					else if (TOKEN_VAR_ENUM == prevType)
						vtype = Literal::LITERAL_TYPE_ENUM;
				}
				else if (Match(1, TOKEN_IDENTIFIER))
				{
					vtype = Literal::LITERAL_TYPE_TT_STRUCT;
				}
				else
				{
					Error(Previous(), "Invalid vector type.");
				}
				Consume(TOKEN_GREATER, "Expected '>' after vector type.");
			}
		}

		std::vector<Token*> ids;
		std::vector<Token*> types;
		std::vector<Literal::LiteralTypeEnum> vtypes;

		do
		{
			if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
			ids.push_back(new Token(TOKEN_IDENTIFIER, Previous().Lexeme(), type->Line(), type->Filename()));
			types.push_back(type);
			vtypes.push_back(vtype);
		
		} while (Match(1, TOKEN_COMMA));

		Expr* expr = nullptr;
		if (Match(1, TOKEN_EQUAL)) expr = Expression();

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) return nullptr;

		if (1 < ids.size())
		{
			return new DestructStmt(types, ids, expr, vtypes, fqns, internal);
		}

		return new VarStmt(type, ids[0], expr, vtype, fqns, internal);
	}

	Stmt* UdtDeclaration()
	{
		std::string fqns = m_fqns;
		//if (m_global) fqns = "global::";

		bool internal = m_internal;
		Token* type = new Token(Advance());
		
		Token* id = nullptr;
		Token prev = Advance();
		std::string name = prev.Lexeme();

		id = new Token(TOKEN_IDENTIFIER, name, prev.Line(), prev.Filename());

		Expr* expr = nullptr;
		if (Match(1, TOKEN_EQUAL)) expr = Expression();

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) return nullptr;

		return new VarStmt(type, id, expr, Literal::LITERAL_TYPE_INVALID, fqns, internal);
	}

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
					return new VarStmt(new Token(TOKEN_VAR_I32, "i32", id->Line(), id->Filename()), id, expr, Literal::LITERAL_TYPE_INVALID, m_fqns);
				}
			}
		}

		return nullptr;
	}

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
				Expr* lhs = new VariableExpr(new Token(*var), nullptr, m_fqns);
				Expr* binary = new BinaryExpr(lhs, new Token(TOKEN_PLUS, "+", var->Line(), var->Filename()), rhs);
				Expr* increment = new AssignExpr(var, binary, nullptr, m_fqns);
					
				Expr* rangeLeft = ((RangeExpr*)((VarStmt*)initializer)->Expression())->Left();
				Expr* rangeRight = ((RangeExpr*)((VarStmt*)initializer)->Expression())->Right();

				// build condition
				Expr* condition = new BinaryExpr(lhs, new Token(TOKEN_LESS, "<", var->Line(), var->Filename()), rangeRight);

				// build while statement
				std::string label = GenerateUUID();
				body = new WhileStmt(condition, body, increment, label);

				// build new initializer
				Stmt* newinit = new VarStmt(new Token(TOKEN_VAR_I32, "i32", var->Line(), var->Filename()), var, rangeLeft, Literal::LITERAL_TYPE_INVALID, m_fqns);

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

	Stmt* LoopStatement()
	{
		if (Check(TOKEN_LEFT_BRACE))
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
		}

		return nullptr;
	}

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

	Stmt* ClearEnvStatement()
	{
		return new ClearEnvStmt();
	}

	Stmt* PrintStatement(bool newline = false)
	{
		if (Consume(TOKEN_LEFT_PAREN, "Expected '(' after print."))
		{
			Expr* expr = Expression();
			if (Consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.") && Consume(TOKEN_SEMICOLON, "Expected ';' after statement."))
			{
				if (newline) return new PrintLnStmt(expr);
				return new PrintStmt(expr);
			}
		}

		return nullptr;
	}

	Stmt* BreakStatement()
	{
		Token* keyword = new Token(Previous());
		if (Consume(TOKEN_SEMICOLON, "Expected ';' after break."))
		{
			return new BreakStmt(keyword);
		}
		
		return nullptr;
	}

	Stmt* ContinueStatement()
	{
		Token* keyword = new Token(Previous());
		if (Consume(TOKEN_SEMICOLON, "Expected ';' after continue."))
		{
			return new ContinueStmt(keyword);
		}

		return nullptr;
	}

	Stmt* ReturnStatement()
	{
		Token* keyword = new Token(Previous());
		Expr* value = nullptr;
		if (!Check(TOKEN_SEMICOLON))
		{
			value = Expression();
		}

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after return value.")) return nullptr;
		return new ReturnStmt(keyword, value);
	}

	Stmt* WhileStatement()
	{
		Expr* condition = Expression();
		
		if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* body = Statement();
			std::string label = GenerateUUID();
			return new WhileStmt(condition, body, nullptr, label);
		}
		else
		{
			Error(Previous(), "Expected '{' after while condition.");
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
		
		if (Match(1, TOKEN_EQUAL))
		{
			Token equals = Previous();
			Expr* value = Assignment();

			if (expr->GetType() == EXPRESSION_VARIABLE)
			{
				VariableExpr* v = (VariableExpr*)expr;
				Token* name = v->Operator();
				return new AssignExpr(name, value, v->VecIndex(), m_fqns);
			}
			else if (expr->GetType() == EXPRESSION_GET)
			{
				while (true)
				{
					GetExpr* get = (GetExpr*)expr;
					Token* name = get->Name();
					value = new SetExpr(get->Object(), name, value, get->VecIndex(), m_fqns);

					expr = get->Object();
					if (EXPRESSION_GET != expr->GetType()) break;
				};

				return value;				
			}
			else if (expr->GetType() == EXPRESSION_STRUCTURE)
			{
				if (value->GetType() != EXPRESSION_STRUCTURE)
				{
					Error(Previous(), "Invalid destructure source.");
				}
				else
				{
					Token* prev = new Token(Previous());
					ArgList lhs = ((StructExpr*)expr)->GetArguments();
					ArgList rhs = ((StructExpr*)value)->GetArguments();
					if (lhs.size() != rhs.size())
					{
						Error(Previous(), "Destructure assignment quantity mismatch.");
					}
					else
					{	
						return new DestructExpr(lhs, rhs, prev);
					}
				}
			}

			Error(Previous(), "Invalid assignment target.");
		}

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

		if (Check(TOKEN_COMMA))
		{
			Advance();
			ArgList args;
			args.push_back(expr);
			do
			{
				args.push_back(Range());
			} while (Match(1, TOKEN_COMMA));

			Token* oper = oper = new Token(Previous());

			return new StructExpr(args, oper);
		}

		return expr;
	}

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

		while (Match(1, TOKEN_AS))
		{
			Token* oper = new Token(Previous());
			Expr* right = Primary();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}

	Expr* Primary()
	{
		if (Match(1, TOKEN_FILELINE)) return new LiteralExpr(std::string("File:" + Previous().Filename() + ", Line:" + std::to_string(Previous().Line())));
		if (Match(1, TOKEN_FALSE)) return new LiteralExpr(false);
		if (Match(1, TOKEN_TRUE)) return new LiteralExpr(true);

		if (Match(1, TOKEN_FLOAT)) return new LiteralExpr(Previous().DoubleValue());
		if (Match(1, TOKEN_INTEGER)) return new LiteralExpr(Previous().IntValue());
		if (Match(1, TOKEN_STRING)) return new LiteralExpr(Previous().StringValue());
		if (Match(1, TOKEN_ENUM)) return new LiteralExpr(Previous().EnumValue());

		if (Match(1, TOKEN_LEFT_PAREN))
		{
			Expr* expr = Expression();
			Consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
			return new GroupExpr(expr);
		}

		// used for AS syntax
		if (Match(3, TOKEN_VAR_I32, TOKEN_VAR_F32, TOKEN_VAR_STRING)) return new VariableExpr(new Token(Previous()), nullptr, m_fqns);
		
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

			Expr* expr = new VariableExpr(new Token(TOKEN_IDENTIFIER, name, prev.Line(), prev.Filename()), vecIndex, m_fqns);

			// check for function call brace
			while (true)
			{
				if (Match(1, TOKEN_LEFT_PAREN))
				{
					expr = FinishCall(expr);
				}
				else if (Check(TOKEN_DOT))
				{
					expr = FinishGet(expr);
				}
				else
				{
					break;
				}
			}

			return expr;
		}

		// bracket list
		if (Match(1, TOKEN_LEFT_BRACKET))
		{
			ArgList args;
			if (!Check(TOKEN_RIGHT_BRACKET))
			{
				do
				{
					Expr* argA = Range();
					if (argA)
					{
						if (Match(1, TOKEN_SEMICOLON))
						{
							Token* semicolon = new Token(Previous());

							// replicate value
							Expr* argB = Range();
							if (argB)
							{
								argA = new ReplicateExpr(argA, semicolon, argB);
							}
							else
							{
								Error(Peek(), "Parser Error: Expected expression after ';' inside bracket.");
							}
						}
						args.push_back(argA);
					}
				} while (Match(1, TOKEN_COMMA));
			}

			Token* bracket = nullptr;
			if (Consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression."))
			{
				bracket = new Token(Previous());
			}

			return new BracketExpr(bracket, args);
		}


		Error(Peek(), "Parser Error: Expected expression.");
		return nullptr;
	}

	Expr* FinishCall(Expr* callee)
	{
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

	Expr* FinishGet(Expr* callee)
	{
		if (Match(1, TOKEN_DOT))
		{
			if (!Match(1, TOKEN_IDENTIFIER))
			{
				Error(Previous(), "Expected structure parameter after '.'.");
				return nullptr;
			}

			Token* temp = new Token(Previous());

			// check for bracket index
			Expr* vecIndex = nullptr;
			if (Match(1, TOKEN_LEFT_BRACKET))
			{
				vecIndex = Expression();
				Consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression.");
			}

			return FinishGet(new GetExpr(callee, temp, vecIndex));

			//expr = new GetExpr(expr, temp, vecIndex);
		}
		return callee;
	}

	std::string GenerateUUID()
	{
		unsigned char bytes[16];

		std::stringstream ss;
		ss << std::hex << std::setfill('0');

		for (unsigned i = 0; i < 16; i++)
		{
			bytes[i] = rand() % 256;
			if (i == 0) bytes[i] = bytes[i] & 0x0f;
			if (i == 6) bytes[i] = 0x40 | (bytes[i] & 0x0F);
			if (i == 8) bytes[i] = 0x80 | (bytes[i] & 0x3F);

			ss << std::setw(2) << static_cast<unsigned>(bytes[i]);
			if (i == 3 || i == 5 || i == 7 || i == 9) ss << '-';
		}

		return ss.str();
	}


	Token Advance();
	bool Check(TokenTypeEnum tokenType);
	bool CheckNext(TokenTypeEnum tokenType);
	bool Consume(TokenTypeEnum tokenType, std::string err);
	bool IsAtEnd();
	bool Match(int count, ...);
	Token Peek();
	Token Previous();
	void Error(Token token, const std::string& err);

	ErrorHandler* m_errorHandler;
	TokenList m_tokenList;
	int m_current;

	std::vector<std::string> m_namespace;
	std::string m_fqns;

	static std::set<std::string> m_includes;
	bool m_internal;
	//bool m_global;
	
};

#endif // PARSER_H