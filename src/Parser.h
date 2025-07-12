#ifndef PARSER_H
#define PARSER_H

#include <fstream>
#include <set>

#include "Token.h"
#include "Expressions.h"
#include "ErrorHandler.h"
#include "Statements.h"
#include "Utility.h"

class Parser
{
public:
	Parser() = delete;
	Parser(TokenList tokenList, ErrorHandler* errorHandler)
	{
		m_tokenList = tokenList;
		m_current = 0;
		m_errorHandler = errorHandler;
		m_anon_func_counter = 0;
		//m_internal = false;
		//m_global = false;
	}

	StmtList Parse()
	{
		StmtList ret;
		PushNamespace("global");

		while (!IsAtEnd() && !m_errorHandler->HasErrors())
		{
			bool success = Namespace(ret);
			if (!success) break;
		}

		PopNamespace();
		
		return ret;
	}

	std::map<std::string, StmtList*> GetAnonFunctors() { return m_anon_funcs; }
	

private:

	// evaluate compile time constant expression
	Expr* Evaluate(Expr* expr);
	Expr* Evaluate_Binary(BinaryExpr* expr);
	Expr* Evaluate_Unary(UnaryExpr* expr);
	Expr* Evaluate_Variable(VariableExpr* expr);

	void PushNamespace(std::string name)
	{
		m_namespace.push_back(name);
		m_fqns = StrJoin(m_namespace, ":");
	}

	void PopNamespace()
	{
		m_namespace.pop_back();
		m_fqns = StrJoin(m_namespace, ":");
	}

	//-----------------------------------------------------------------------------
	bool Namespace(StmtList& list)
	{
		bool ret = false;

		// is this the start of a namespace container?
		if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_COLON))
		{
			Token ns = Advance();
			
			// consume colon
			Advance();

			if (!Consume(TOKEN_LEFT_BRACE, "Expected '{' before namespace body.")) return false;

			list.push_back(new NamespacePushStmt(ns.Lexeme()));
			PushNamespace(ns.Lexeme());

			// scan internals
			while (!IsAtEnd() && !m_errorHandler->HasErrors() && !Check(TOKEN_RIGHT_BRACE))
			{
				bool success = Namespace(list);
				if (!success) return false;
			}

			if (!Consume(TOKEN_RIGHT_BRACE, "Expected '}' after namespace body.")) return false;

			PopNamespace();
			list.push_back(new NamespacePopStmt());
			
			return true;
		}

		// not a namespace, keep going
		Stmt* stmt = Declaration();
		if (!stmt) return false;

		list.push_back(stmt);
		return true;
	}
	
	//-----------------------------------------------------------------------------
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
		}

		//m_internal = false;
		//if (Match(1, TOKEN_INTERNAL)) m_internal = true;

		//m_global = false;
		//if (Match(1, TOKEN_GLOBAL)) m_global = true;

		if (Match(1, TOKEN_STRUCT)) return StructDeclaration();
		
		if (Match(1, TOKEN_DEF)) return Function("function");
		
		if (Match(1, TOKEN_CONST)) return VarConstDeclaration();
		
		if (Match_Vars() || Match_Vars_Raylib())
			return VarDeclaration();
		
		// udt declaration
		if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_IDENTIFIER))
			return UdtDeclaration();

		
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
		if (Match(1, TOKEN_BREAK)) return BreakStatement();
		if (Match(1, TOKEN_CONTINUE)) return ContinueStatement();
		if (Match(1, TOKEN_RETURN)) return ReturnStatement();
		
		return ExpressionStatement();
	}

	void Include();


	TypeToken ScanTypeToken(Token* token)
	{
		TypeToken ret;

		TokenTypeEnum type = token->GetType();
		TokenPtrList* typeArgs = new TokenPtrList();
		Token* arg0 = nullptr;
		Token* arg1 = nullptr;

		if (TOKEN_VAR_VEC == type)
		{
			if (!Consume(TOKEN_LESS, "Expected <type> after vec.")) return ret;
			arg0 = new Token(Advance());
			if (Check(TOKEN_COMMA))
			{
				Advance();
				Token rhs = Peek();
				if (Check(TOKEN_INTEGER))
				{
					Advance();
					arg1 = new Token(Previous());
				}
				else
				{
					Expr* expr = Addition();
					if (!expr || EXPRESSION_LITERAL != expr->GetType())
					{
						Error(rhs, "Failed to evaluate constant expression.");
						return ret;
					}
					Literal val = static_cast<LiteralExpr*>(expr)->GetLiteral();
					int32_t ival = val.IntValue();
					if (LITERAL_TYPE_INTEGER != val.GetType())
					{
						Error(rhs, "Fixed length size must resolve to constant integer.");
						return ret;
					}
					arg1 = new Token(TOKEN_INTEGER, std::to_string(ival), ival, ival, rhs.Line(), rhs.Filename());
				}
			}
			if (!Consume(TOKEN_GREATER, "Expected '>' after vector type.")) return ret;
			typeArgs->push_back(arg0);
			typeArgs->push_back(arg1);
		}
		else if (TOKEN_VAR_MAP == type)
		{
			if (!Consume(TOKEN_LESS, "Expected <type> after map.")) return ret;
			arg0 = new Token(Advance());
			if (!Consume(TOKEN_COMMA, "Expected map container type")) return ret;
			arg1 = new Token(Advance());
			if (!Consume(TOKEN_GREATER, "Expected '>' after map type.")) return ret;
			typeArgs->push_back(arg0);
			typeArgs->push_back(arg1);
		}
		else if (TOKEN_VAR_SET == type)
		{
			if (!Consume(TOKEN_LESS, "Expected <type> after set.")) return ret;
			arg0 = new Token(Advance());
			if (!Consume(TOKEN_GREATER, "Expected '>' after set type.")) return ret;
			typeArgs->push_back(arg0);
		}
		else if (TOKEN_VAR_TUPLE == type)
		{
			if (!Consume(TOKEN_LESS, "Expected <type> after tuple.")) return ret;
			arg0 = new Token(Advance());
			typeArgs->push_back(arg0);
			while (Check(TOKEN_COMMA))
			{
				Advance();
				typeArgs->push_back(new Token(Advance()));
			}
			if (!Consume(TOKEN_GREATER, "Expected '>' after tuple type.")) return ret;
		}
		else if (TOKEN_AT == type)
		{
			// parse return type
			typeArgs->push_back(nullptr);

			if (!Consume(TOKEN_LEFT_PAREN, "Expected '(' after '@'.")) return ret;

			// parse params
			typeArgs->push_back(nullptr);

			if (!Consume(TOKEN_RIGHT_PAREN, "Expected ')' after functor parameters.")) return ret;
		}

		ret.type = token;
		ret.args = typeArgs;
		return ret;
	}


	//-----------------------------------------------------------------------------
	Stmt* Function(std::string kind)
	{
		// check for return type
		TokenPtrList* retArgs = new TokenPtrList();

		TypeToken* rettype = nullptr;
		// if theres an identifier after the current, use the current one as the rettype
		if (CheckNext(TOKEN_LESS) || CheckNext(TOKEN_IDENTIFIER))
		{
			rettype = new TypeToken(ScanTypeToken(new Token(Advance())));
		}
				
		if (!Consume(TOKEN_IDENTIFIER, "Expected " + kind + " name.")) return nullptr;
		Token* name = new Token(Previous());

		if (!Consume(TOKEN_LEFT_PAREN, "Expected '(' after " + kind + " name.")) return nullptr;
		
		TokenList params;
		TypeTokenList types;
		std::vector<bool> mut;
		std::vector<Expr*> defaults;

		if (!Check(TOKEN_RIGHT_PAREN))
		{
			do
			{
				types.push_back(ScanTypeToken(new Token(Advance())));
				if (Check(TOKEN_AMPERSAND))
				{
					mut.push_back(true);
					Advance();
				}
				else
				{
					mut.push_back(false);
				}

				if (Consume(TOKEN_IDENTIFIER, "Expect parameter name."))
				{
					params.push_back(Token(Previous()));
				}

				if (Check(TOKEN_EQUAL))
				{
					Advance();
					if (Match(1, TOKEN_FALSE)) defaults.push_back(new LiteralExpr(new Token(Previous()), false));
					else if (Match(1, TOKEN_TRUE)) defaults.push_back(new LiteralExpr(new Token(Previous()), true));
					else if (Match(1, TOKEN_FLOAT)) defaults.push_back(new LiteralExpr(new Token(Previous()), Previous().DoubleValue()));
					else if (Match(1, TOKEN_INTEGER)) defaults.push_back(new LiteralExpr(new Token(Previous()), Previous().IntValue()));
					else if (Match(1, TOKEN_STRING)) defaults.push_back(new LiteralExpr(new Token(Previous()), Previous().StringValue()));
					else if (Match(1, TOKEN_ENUM)) defaults.push_back(new LiteralExpr(new Token(Previous()), Previous().EnumValue()));
					else
					{
						Error(Peek(), "Expected literal for default argument.");
						return nullptr;
					}
				}
				else
				{
					defaults.push_back(nullptr);
				}

			} while (Match(1, TOKEN_COMMA));
		}

		if (!Consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.")) return nullptr;

		if (!Consume(TOKEN_LEFT_BRACE, "Expected '{' before " + kind + " body.")) return nullptr;

		StmtList* body = BlockStatement();
		return new FunctionStmt(rettype, name, types, params, mut, defaults, body, m_fqns);
	}

	
	//-----------------------------------------------------------------------------
	Stmt* StructDeclaration()
	{
		if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
		Token* id = new Token(Previous());

		if (!Consume(TOKEN_LEFT_BRACE, "Expected { for struct definition."))
		{
			delete id;
			return nullptr;
		}

		StmtList* vars = new StmtList();
		while (!Check(TOKEN_RIGHT_BRACE) && !IsAtEnd() && !m_errorHandler->HasErrors())
		{
			if (Match_Vars() || Match_Vars_Raylib())
			{
				vars->push_back(VarDeclaration());
			}
			else if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_IDENTIFIER))
			{
				vars->push_back(UdtDeclaration());
			}
			else if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_SEMICOLON))
			{
				// struct composition
				Token name = Advance();
				
				// consume semicolon
				Advance();

				if (0 == m_struct_defs.count(name.Lexeme()))
				{
					Error(name, "Missing type for structure member.");
					delete id;
					delete vars;
					return nullptr;
				}

				StructStmt* stmt = m_struct_defs.at(name.Lexeme());
				for (auto& v : *stmt->GetVars())
				{
					vars->push_back(v);
				}
			}
			else
			{
				Error(Previous(), "Invalid variable type inside structure definition.");
				delete id;
				delete vars;
				return nullptr;
			}
		}

		if (!Consume(TOKEN_RIGHT_BRACE, "Expected '}' after struct definition."))
		{
			delete id;
			delete vars;
			return nullptr;
		}

		StructStmt* ret = new StructStmt(id, vars, m_fqns);
		m_struct_defs.insert(std::make_pair(id->Lexeme(), ret)); // make a copy for struct composition
		return ret;
	}
	

	//-----------------------------------------------------------------------------
	Stmt* VarDeclaration()
	{
		TypeToken typeToken = ScanTypeToken(new Token(Previous()));
		if (!typeToken.type) return nullptr;

		// if this is a functor, need to add to prototype list
		std::string anon_sig;
		if (TOKEN_AT == typeToken.type->GetType())
		{
			anon_sig = "__USER_ANON_NOOP";
			if (0 == m_anon_funcs.count(anon_sig))
			{
				m_anon_funcs.insert(std::make_pair(anon_sig, nullptr));
			}
		}

		TokenPtrList ids;
		ArgList exprs;

		while (true)
		{
			Token* id;
			if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
			id = new Token(TOKEN_IDENTIFIER, Previous().Lexeme(), typeToken.type->Line(), typeToken.type->Filename());

			ids.push_back(id);
			if (Check(TOKEN_SEMICOLON) || Check(TOKEN_EQUAL)) break;
			if (!Consume(TOKEN_COMMA, "Expected ',' after identifier.")) return nullptr;
		}

		Expr* expr = nullptr;
		if (Match(1, TOKEN_EQUAL))
		{
			expr = Assignment();
			if (!expr) return nullptr;
			exprs.push_back(expr);

			// unpack the collect expression
			if (EXPRESSION_COLLECT == expr->GetType())
			{
				exprs = static_cast<CollectExpr*>(expr)->GetArguments();
			}
		}
		else
		{
			for (size_t i = 0; i < ids.size(); ++i)
			{
				exprs.push_back(nullptr);
			}
		}

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) return nullptr;

		if (ids.size() != exprs.size())
		{
			Error(*typeToken.type, "Assignment count mismatch.");
			return nullptr;
		}

		return new VarStmt(typeToken, ids, exprs, anon_sig);
	}



	//-----------------------------------------------------------------------------
	Stmt* UdtDeclaration()
	{
		TypeToken typeToken = ScanTypeToken(new Token(Advance()));
		if (!typeToken.type) return nullptr;

		Token prev = Advance();
		std::string name = prev.Lexeme();

		TokenPtrList ids;
		ArgList exprs;

		bool paired = true;
		if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_COMMA)) paired = false;

		if (paired)
		{
			while (true)
			{
				Token* id = nullptr;
				id = new Token(TOKEN_IDENTIFIER, name, prev.Line(), prev.Filename());
				ids.push_back(id);

				Expr* expr = nullptr;
				if (Match(1, TOKEN_EQUAL)) expr = Assignment();
				exprs.push_back(expr);

				if (Check(TOKEN_SEMICOLON)) break;
				if (!Consume(TOKEN_COMMA, "Expected ',' after expression.")) return nullptr;
			}
		}
		else
		{
			while (true)
			{
				Token* id = nullptr;
				id = new Token(TOKEN_IDENTIFIER, name, prev.Line(), prev.Filename());
				ids.push_back(id);

				if (Check(TOKEN_SEMICOLON) || Check(TOKEN_EQUAL)) break;
				if (!Consume(TOKEN_COMMA, "Expected ',' after identifier.")) return nullptr;
			}

			Expr* expr = nullptr;
			if (Match(1, TOKEN_EQUAL))
			{
				while (true)
				{
					expr = Assignment();
					exprs.push_back(expr);

					if (Check(TOKEN_SEMICOLON)) break;
					if (!Consume(TOKEN_COMMA, "Expected ',' after expression.")) return nullptr;
				}
			}
			else
			{
				for (size_t i = 0; i < ids.size(); ++i)
				{
					exprs.push_back(nullptr);
				}
			}
		}

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) return nullptr;

		if (ids.size() != exprs.size())
		{
			Error(*typeToken.type, "Assignment count mismatch.");
			return nullptr;
		}

		return new VarStmt(typeToken, ids, exprs);
	}


	//-----------------------------------------------------------------------------
	Stmt* VarConstDeclaration()
	{
		if (!Match(3, TOKEN_VAR_I16, TOKEN_VAR_I32, TOKEN_VAR_I64))
		{
			Error(Previous(), "Expected integer type after const.");
			return nullptr;
		}

		Token* type = new Token(Previous());

		TokenPtrList ids;
		ArgList exprs;

		bool paired = true;
		if (Check(TOKEN_IDENTIFIER) && CheckNext(TOKEN_COMMA)) paired = false;

		if (paired)
		{
			while (true)
			{
				Token* id;
				if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
				id = new Token(TOKEN_IDENTIFIER, Previous().Lexeme(), type->Line(), type->Filename());

				std::string fqid = m_fqns + ":" + id->Lexeme();
				if (0 != m_const_vars.count(fqid))
				{
					Error(*id, "Constant already defined in environment.");
					return nullptr;
				}

				ids.push_back(id);

				if (!Consume(TOKEN_EQUAL, "Expected assignment after identifier.")) return nullptr;

				Token equals = Previous();
				Expr* expr = Addition();
				if (!expr || EXPRESSION_LITERAL != expr->GetType())
				{
					Error(equals, "Failed to evaluate constant expression.");
					return nullptr;
				}
				if (LITERAL_TYPE_INTEGER != static_cast<LiteralExpr*>(expr)->GetLiteral().GetType())
				{
					Error(equals, "Constant expression does not evaluate to integer.");
					return nullptr;
				}

				m_const_vars.insert(std::make_pair(fqid, static_cast<LiteralExpr*>(expr)));

				exprs.push_back(expr);

				if (Check(TOKEN_SEMICOLON)) break;
				if (!Consume(TOKEN_COMMA, "Expected ',' after expression.")) return nullptr;
			}
		}
		else
		{
			while (true)
			{
				Token* id;
				if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
				id = new Token(TOKEN_IDENTIFIER, Previous().Lexeme(), type->Line(), type->Filename());

				std::string fqid = m_fqns + ":" + id->Lexeme();
				if (0 != m_const_vars.count(fqid))
				{
					Error(*id, "Constant already defined in environment.");
					return nullptr;
				}

				ids.push_back(id);

				// todo - fix this logic to make more appropriate errors
				if (Check(TOKEN_SEMICOLON) || Check(TOKEN_EQUAL)) break;
				if (!Consume(TOKEN_COMMA, "Expected ',' after identifier.")) return nullptr;
			}

			if (!Consume(TOKEN_EQUAL, "Expected assignment after identifier.")) return nullptr;

			while (true)
			{
				Token equals = Previous();
				Expr* expr = Addition();
				if (!expr || EXPRESSION_LITERAL != expr->GetType())
				{
					Error(equals, "Failed to evaluate constant expression.");
					return nullptr;
				}
				if (LITERAL_TYPE_INTEGER != static_cast<LiteralExpr*>(expr)->GetLiteral().GetType())
				{
					Error(equals, "Constant expression does not evaluate to integer.");
					return nullptr;
				}

				std::string fqid = m_fqns + ":" + ids[exprs.size()]->Lexeme();

				m_const_vars.insert(std::make_pair(fqid, static_cast<LiteralExpr*>(expr)));

				exprs.push_back(expr);
				
				// todo - fix this logic to make more appropriate errors
				if (Check(TOKEN_SEMICOLON)) break;
				if (!Consume(TOKEN_COMMA, "Expected ',' after expression.")) return nullptr;
				if (exprs.size() == ids.size()) break;
			}
		}

		if (!Consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.")) return nullptr;

		if (ids.size() != exprs.size())
		{
			Error(*type, "Assignment count mismatch.");
			return nullptr;
		}

		return new VarConstStmt(type, ids, exprs, m_fqns);
	}
	

	//-----------------------------------------------------------------------------
	Stmt* ForStatement()
	{
		Token* key = nullptr;
		Token* val = nullptr;
		Expr* expr = nullptr;
		if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;

		val = new Token(Previous());
		if (Check(TOKEN_COMMA))
		{
			Advance();
			if (!Consume(TOKEN_IDENTIFIER, "Expected identifier.")) return nullptr;
			key = val;
			val = new Token(Previous());
		}

		if (!Consume(TOKEN_IN, "Expected 'in'.")) return nullptr;

		expr = Expression();
		if (EXPRESSION_LITERAL == expr->GetType())
		{
			// attempt to build range expression starting at 0
			expr = new RangeExpr(new LiteralExpr(val, int32_t(0)), new Token(TOKEN_DOT_DOT, "..", val->Line(), val->Filename()), expr);
		}
		if (!expr) return nullptr;


		if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* body = Statement();
			if (body)
			{
				// wrap foreach in a block to get a local environment scope
				StmtList* list = new StmtList();
				list->push_back(new ForEachStmt(key, val, expr, body));
				return new BlockStmt(list);
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
		// check for counter expression
		Expr* expr = nullptr;
		Token* prev = new Token(Previous());
		if (!Check(TOKEN_LEFT_BRACE))
		{
			expr = Expression();
			if (expr)
			{
				if (EXPRESSION_RANGE != expr->GetType())
				{
					// attempt to build range expression starting at 0
					expr = new RangeExpr(new LiteralExpr(prev, int32_t(0)), new Token(TOKEN_DOT_DOT, "..", prev->Line(), prev->Filename()), expr);
				}
			}
			if (!expr) return nullptr;
		}
		
		if (Check(TOKEN_LEFT_BRACE))
		{
			Stmt* body = Statement();
			if (body)
			{
				if (expr)
				{
					// make a for loop to use the expression as a counter.
					StmtList* list = new StmtList();
					list->push_back(new ForEachStmt(nullptr, new Token(TOKEN_IDENTIFIER, "_", prev->Line(), prev->Filename()), expr, body));
					return new BlockStmt(list);
				}
				// else make a while true loop
				return new WhileStmt(new LiteralExpr(nullptr, true), body, nullptr);
			}
		}
		else
		{
			Error(Previous(), "Expected '{' after 'loop'.");
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
			if (body)
			{
				return new WhileStmt(condition, body, nullptr);
			}
		}
		else
		{
			Error(Previous(), "Expected '{' after while condition.");
		}

		return nullptr;
	}

	
	//-----------------------------------------------------------------------------
	Stmt* IfStatement()
	{
		Token* token = new Token(Previous());
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
				else if (Check(TOKEN_LEFT_BRACE))
				{
					elseBranch = Statement();
				}
				else
				{
					Error(Previous(), "Expected '{' after else condition.");
				}
			}
			return new IfStmt(token, condition, thenBranch, elseBranch);
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

		while (!Check(TOKEN_RIGHT_BRACE) && !IsAtEnd() && !m_errorHandler->HasErrors())
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
			Expr* expr = nullptr;
			if (!Check(TOKEN_RIGHT_PAREN)) expr = Expression();
			
			if (Consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.") && Consume(TOKEN_SEMICOLON, "Expected ';' after statement."))
			{
				return new PrintStmt(expr, newline);
			}
		}

		return nullptr;
	}


	//-----------------------------------------------------------------------------
	Stmt* BreakStatement()
	{
		Token* keyword = new Token(Previous());
		if (Consume(TOKEN_SEMICOLON, "Expected ';' after break."))
		{
			return new BreakStmt(keyword);
		}
		
		return nullptr;
	}


	//-----------------------------------------------------------------------------
	Stmt* ContinueStatement()
	{
		Token* keyword = new Token(Previous());
		if (Consume(TOKEN_SEMICOLON, "Expected ';' after continue."))
		{
			return new ContinueStmt(keyword);
		}

		return nullptr;
	}


	//-----------------------------------------------------------------------------
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
		
		if (Match(7, TOKEN_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_PLUS_EQUAL,
			TOKEN_SLASH_EQUAL, TOKEN_STAR_EQUAL, TOKEN_HAT_EQUAL, TOKEN_PERCENT_EQUAL))
		{
			Token* equals = new Token(Previous());
			Expr* value = Assignment();

			// todo - make this skip the temporary
			int line = equals->Line();
			std::string filename = equals->Filename();

			switch (equals->GetType())
			{
			case TOKEN_MINUS_EQUAL: value = new BinaryExpr(expr, new Token(TOKEN_MINUS, "-", line, filename), value); break;
			case TOKEN_PLUS_EQUAL: value = new BinaryExpr(expr, new Token(TOKEN_PLUS, "+", line, filename), value); break;
			case TOKEN_SLASH_EQUAL: value = new BinaryExpr(expr, new Token(TOKEN_SLASH, "/", line, filename), value); break;
			case TOKEN_STAR_EQUAL: value = new BinaryExpr(expr, new Token(TOKEN_STAR, "*", line, filename), value); break;
			case TOKEN_HAT_EQUAL: value = new BinaryExpr(expr, new Token(TOKEN_HAT, "^", line, filename), value); break;
			case TOKEN_PERCENT_EQUAL: value = new BinaryExpr(expr, new Token(TOKEN_PERCENT, "%", line, filename), value); break;
			default:
			}
			
			if (expr->GetType() == EXPRESSION_VARIABLE)
			{
				VariableExpr* v = (VariableExpr*)expr;
				Token* name = v->Operator();
				return new AssignExpr(name, value, v->VecIndex());
			}
			else if (expr->GetType() == EXPRESSION_GET)
			{
				return new SetExpr(expr, value);
			}
			else if (expr->GetType() == EXPRESSION_COLLECT)
			{
				return new DestructExpr(expr, equals, value);
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
		Expr* expr = Collect();//Pair();//Range();//Struct();

		while (Match(4, TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL))
		{
			Token* oper = new Token(Previous());
			Expr* right = Pair();//Range();//Struct();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}

	
	//-----------------------------------------------------------------------------
	Expr* Collect()
	{
		Expr* expr = Pair();//Range();//Struct();

		if (Check(TOKEN_COMMA))
		{
			Token* oper = new Token(Previous());
			ArgList args;
			args.push_back(expr);

			while (Match(1, TOKEN_COMMA))
			{
				Expr* arg = Pair();
				if (!arg) return nullptr;
				
				args.push_back(arg);
			}

			return new CollectExpr(oper, args);
		}

		return expr;
	}


	//-----------------------------------------------------------------------------
	Expr* Pair()
	{
		Expr* expr = Range();//Struct();

		while (Match(1, TOKEN_COLON))
		{
			Token* oper = new Token(Previous());
			Expr* right = Range();//Struct();
			expr = new BinaryExpr(expr, oper, right);
		}

		return expr;
	}

	//-----------------------------------------------------------------------------
	/*Expr* Struct()
	{
		Expr* expr = Range();

		return expr;
	}*/


	//-----------------------------------------------------------------------------
	Expr* Range()
	{
		Expr* expr = Addition();

		while (Match(2, TOKEN_DOT_DOT, TOKEN_DOT_DOT_EQUAL))
		{
			Token* oper = new Token(Previous());
			Expr* right = Addition();

			if (TOKEN_DOT_DOT_EQUAL == oper->GetType())
			{
				// overwrite the right side with one that increments by one
				right = new BinaryExpr(right, new Token(TOKEN_PLUS, "+", oper->Line(), oper->Filename()), new LiteralExpr(oper, int32_t(1)));
			}

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

		if (expr) expr = Evaluate(expr); // attempt to evaluate at compile time
		return expr;
	}


	//-----------------------------------------------------------------------------
	Expr* Multiplication()
	{
		Expr* expr = Power();

		while (Match(2, TOKEN_SLASH, TOKEN_STAR))
		{
			Token* oper = new Token(Previous());
			Expr* right = Power();
			expr = new BinaryExpr(expr, oper, right);
		}
		
		return expr;
	}


	//-----------------------------------------------------------------------------
	Expr* Power()
	{
		Expr* expr = Unary();

		while (Match(1, TOKEN_HAT))
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
		if (Match(1, TOKEN_FILELINE)) return new LiteralExpr(new Token(Previous()), std::string("File:" + Previous().Filename() + ", Line:" + std::to_string(Previous().Line())));
		if (Match(1, TOKEN_FALSE)) return new LiteralExpr(new Token(Previous()), false);
		if (Match(1, TOKEN_TRUE)) return new LiteralExpr(new Token(Previous()), true);
		if (Match(1, TOKEN_FLOAT)) return new LiteralExpr(new Token(Previous()), Previous().DoubleValue());
		if (Match(1, TOKEN_INTEGER)) return new LiteralExpr(new Token(Previous()), Previous().IntValue());
		if (Match(1, TOKEN_STRING)) return new LiteralExpr(new Token(Previous()), Previous().StringValue());
		if (Match(1, TOKEN_ENUM)) return new LiteralExpr(new Token(Previous()), Previous().EnumValue());
		if (Match(1, TOKEN_PI)) return new LiteralExpr(new Token(Previous()), acos(-1));

		if (Match(1, TOKEN_LEFT_PAREN))
		{
			Expr* expr = Expression();
			Consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
			return new GroupExpr(expr);
		}

		// used for AS syntax
		if (Match(7, TOKEN_VAR_I16, TOKEN_VAR_I32, TOKEN_VAR_I64,
			TOKEN_VAR_F32, TOKEN_VAR_F64,
			TOKEN_VAR_STRING, TOKEN_VAR_ENUM)) return new VariableExpr(new Token(Previous()), nullptr);

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
			while (true && !m_errorHandler->HasErrors())
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

		
		// anonymous brace
		if (Match(1, TOKEN_LEFT_BRACE))
		{
			Token* brace = new Token(Previous());
			ArgList args;
			if (!Check(TOKEN_RIGHT_BRACE))
			{
				Expr* arg = Collect();
				if (arg) args.push_back(arg);
				
				// unpack the collect expression
				if (1 == args.size() && EXPRESSION_COLLECT == args[0]->GetType())
				{
					args = static_cast<CollectExpr*>(args[0])->GetArguments();
				}
			}

			if (!Consume(TOKEN_RIGHT_BRACE, "Expect '}' after expression."))
			{
				return nullptr;
			}

			return new BraceExpr(brace, args);
		}

		// functor
		if (Match(1, TOKEN_AT))
		{
			Token* at = new Token(Previous());

			// parse return type
			ArgList args;
			args.push_back(nullptr);

			if (!Consume(TOKEN_LEFT_PAREN, "Expected '(' after '@'.")) return nullptr;

			// parse params
			args.push_back(nullptr);

			if (!Consume(TOKEN_RIGHT_PAREN, "Expected ')' after functor parameters.")) return nullptr;

			if (!Consume(TOKEN_LEFT_BRACE, "Expected '{' for functor body.")) return nullptr;

			StmtList* body = BlockStatement();
			if (!body) return nullptr;

			std::string sig = "__USER_ANON_" + std::to_string(m_anon_func_counter);
			m_anon_funcs.insert(std::make_pair(sig, body));
			m_anon_func_counter++;

			return new FunctorExpr(at, args, sig);
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

		// unpack the collect expression
		if (1 == args.size() && EXPRESSION_COLLECT == args[0]->GetType())
		{
			args = static_cast<CollectExpr*>(args[0])->GetArguments();
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


	Token Advance();
	bool Check(TokenTypeEnum tokenType);
	bool CheckNext(TokenTypeEnum tokenType);
	bool CheckNextNext(TokenTypeEnum tokenType);
	bool Consume(TokenTypeEnum tokenType, std::string err);
	bool IsAtEnd();
	bool Match_Vars();
	bool Match_Vars_Raylib();
	bool Match(int count, ...);
	Token Peek();
	Token Previous();
	void Error(Token token, const std::string& err);

	ErrorHandler* m_errorHandler;
	TokenList m_tokenList;
	int m_current;

	std::set<std::string> m_includes;
	//bool m_internal;
	//bool m_global;

	std::map<std::string, StructStmt*> m_struct_defs;
	
	llvm::SmallVector<std::string> m_namespace;
	std::string m_fqns;

	std::map<std::string, LiteralExpr*> m_const_vars;
	
	std::map<std::string, StmtList*> m_anon_funcs;
	size_t m_anon_func_counter;
};

#endif // PARSER_H