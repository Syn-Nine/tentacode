#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <iostream>


#include "Literal.h"
#include "Expressions.h"
#include "ErrorHandler.h"
#include "Statements.h"
#include "Environment.h"
#include "Extensions.h"


class Interpreter
{
public:
	Interpreter() = delete;
	Interpreter(ErrorHandler* errorHandler)
	{
		m_errorHandler = errorHandler;
		m_globals = new Environment(errorHandler);
		m_environment = m_globals;

		// built in standard library
		Extensions::Include_Std(m_globals);

		// raylib support
		Extensions::Include_Raylib(m_globals);

	}

	~Interpreter()
	{
		delete m_globals;
	}

	ErrorHandler* GetErrorHandler() { return m_errorHandler; }
	Environment* GetGlobals() { return m_globals; }

	void Interpret(StmtList stmts)
	{
		//try
		//{
			// first pass for struct & function definitions
			for (auto& statement : stmts)
			{
				StatementTypeEnum stype = statement->GetType();
				if (STATEMENT_STRUCT == stype || STATEMENT_FUNCTION == stype)
					Execute(statement);
			}

			// second pass for everything else
			for (auto& statement : stmts)
			{
				StatementTypeEnum stype = statement->GetType();
				if (STATEMENT_STRUCT != stype && STATEMENT_FUNCTION != stype)
					Execute(statement);
			}
		//}
		/*catch (...)
		{
			printf("Error caught by interpreter.\n");
		}*/
	}

	// public so it can be called by Literal::Call
	void ExecuteBlock(StmtList* block, Environment* environment)
	{
		Environment* previous = m_environment;
		m_environment = environment;

		try {
			for (size_t i = 0; i < block->size(); ++i)
			{
				Execute(block->at(i));
			}
		}
		catch (std::string label)
		{
			delete environment;
			m_environment = previous;
			throw label;
		}
		catch (Literal value)
		{
			delete environment;
			m_environment = previous;
			throw value;
		}

		delete environment;
		m_environment = previous;
	}


private:


	void Execute(Stmt* statement)
	{
		if (!statement)
		{
			m_errorHandler->Error("", 0, "Interpreter: Invalid statements.");
			return;
		}

		switch (statement->GetType())
		{
		case STATEMENT_CLEARENV: VisitClearEnvStatement(); break;
		case STATEMENT_EXPRESSION: VisitExpressionStatement((ExpressionStmt*)statement); break;
		case STATEMENT_PRINT: VisitPrintStatement((PrintStmt*)statement); break;
		case STATEMENT_PRINTLN: VisitPrintStatement((PrintStmt*)statement, true); break;
		case STATEMENT_VAR: VisitVarStatement((VarStmt*)statement); break;
		case STATEMENT_DESTRUCT: VisitDestructStatement((DestructStmt*)statement); break;
		case STATEMENT_BLOCK: VisitBlockStatement((BlockStmt*)statement); break;
		case STATEMENT_IF: VisitIfStatement((IfStmt*)statement); break;
		case STATEMENT_WHILE: VisitWhileStatement((WhileStmt*)statement); break;
		case STATEMENT_BREAK: VisitBreakStatement((BreakStmt*)statement); break;
		case STATEMENT_CONTINUE: VisitContinueStatement((ContinueStmt*)statement); break;
		case STATEMENT_FUNCTION: VisitFunctionStatement((FunctionStmt*)statement); break;
		case STATEMENT_STRUCT: VisitStructStatement((StructStmt*)statement); break;
		case STATEMENT_RETURN: VisitReturnStatement((ReturnStmt*)statement); break;
		}
	}


	Literal Evaluate(Expr* expr)
	{
		if (!expr)
		{
			m_errorHandler->Error("", 0, "Interpreter: Invalid expression.");
			return Literal();
		}

		switch (expr->GetType())
		{
		case EXPRESSION_ASSIGN: return VisitAssign((AssignExpr*)expr);
		case EXPRESSION_BINARY: return VisitBinary((BinaryExpr*)expr);
		case EXPRESSION_BRACKET: return VisitBracket((BracketExpr*)expr);
		case EXPRESSION_CALL: return VisitCall((CallExpr*)expr);
		case EXPRESSION_GROUP: return VisitGroup((GroupExpr*)expr);
		case EXPRESSION_LITERAL: return VisitLiteral((LiteralExpr*)expr);
		case EXPRESSION_LOGICAL: return VisitLogical((LogicalExpr*)expr);
		case EXPRESSION_RANGE: return VisitRange((RangeExpr*)expr);
		case EXPRESSION_STRUCTURE: return VisitStructure((StructExpr*)expr);
		case EXPRESSION_DESTRUCTURE: return VisitDestructure((DestructExpr*)expr);
		case EXPRESSION_UNARY: return VisitUnary((UnaryExpr*)expr);
		case EXPRESSION_VARIABLE: return VisitVariable((VariableExpr*)expr);
		case EXPRESSION_GET: return VisitGet((GetExpr*)expr);
		case EXPRESSION_SET: return VisitSet((SetExpr*)expr);
		}

		return Literal();
	}

	void VisitClearEnvStatement()
	{
		//printf("Clearing Environment.\n");
		m_environment->Clear();
	}

	void VisitBlockStatement(BlockStmt* stmt)
	{
		ExecuteBlock(stmt->GetBlock(), new Environment(m_environment, m_errorHandler));
	}

	void VisitFunctionStatement(FunctionStmt* stmt)
	{
		Literal ftn = Literal();
		ftn.SetCallable(stmt);
		m_globals->Define(stmt->Operator()->Lexeme(), ftn, stmt->FQNS(), stmt->Internal());
	}

	void VisitStructStatement(StructStmt* stmt)
	{
		Literal temp = Literal();
		temp.SetCallable(stmt);
		m_globals->Define(stmt->Operator()->Lexeme(), temp, stmt->FQNS(), stmt->Internal());
	}

	void VisitReturnStatement(ReturnStmt* stmt)
	{
		Literal value;
		Expr* expr = stmt->GetValueExpr();
		if (expr) value = Evaluate(expr);

		throw value;
	}

	void VisitIfStatement(IfStmt* stmt)
	{
		if (IsTruthy(Evaluate(stmt->GetCondition())))
		{
			Execute(stmt->GetThenBranch());
		}
		else if (stmt->GetElseBranch() != nullptr)
		{
			Execute(stmt->GetElseBranch());
		}
	}

	void VisitDestructStatement(DestructStmt* stmt)
	{
		// TODO, reduce duplicate code with VisitVarStatement
		Literal value;
		if (stmt->Expression())
		{
			value = Evaluate(stmt->Expression());
		}

		const std::vector<Token*>& names = stmt->Operators();
		const std::vector<Token*>& varTypes = stmt->VarTypes();
		const std::vector<Literal::LiteralTypeEnum>& vecTypes = stmt->VarVecTypes();

		// if value is invalid, nothing to assign
		if (value.IsInvalid())
		{
			for (size_t i = 0; i < names.size(); ++i)
			{
				auto varType = varTypes[i]->GetType();
				auto vecType = vecTypes[i];

				if (TOKEN_VAR_I32 == varType) value = Literal(int32_t(0));
				if (TOKEN_VAR_F32 == varType) value = Literal(0.0);
				if (TOKEN_VAR_STRING == varType) value = Literal(std::string(""));
				if (TOKEN_VAR_ENUM == varType) value = Literal(EnumLiteral());
				if (TOKEN_VAR_BOOL == varType) value = Literal(false);
				if (TOKEN_VAR_VEC == varType)
				{
					if (Literal::LITERAL_TYPE_BOOL == vecType)
						value = Literal(std::vector<bool>());
					else if (Literal::LITERAL_TYPE_INTEGER == vecType)
						value = Literal(std::vector<int32_t>());
					else if (Literal::LITERAL_TYPE_DOUBLE == vecType)
						value = Literal(std::vector<double>());
					else if (Literal::LITERAL_TYPE_STRING == vecType)
						value = Literal(std::vector<std::string>());
					else if (Literal::LITERAL_TYPE_ENUM == vecType)
						value = Literal(std::vector<EnumLiteral>());
					else if (Literal::LITERAL_TYPE_TT_STRUCT == vecType)
						value = Literal(std::vector<Literal>(), vecType);
				}

				// user defined type
				if (TOKEN_IDENTIFIER == varType)
				{
					Literal vtype = m_environment->Get(varTypes[i], stmt->FQNS());

					if (!vtype.IsCallable())
					{
						printf("Invalid user defined type in variable declaration.\n");
					}
					else
					{
						value = vtype.Call(this, LiteralList());
					}
				}

				m_environment->Define(names[i]->Lexeme(), value, stmt->FQNS(), stmt->Internal());
			}
		}
		else
		{
			if (value.IsVecAnonymous())
			{
				auto values = value.VecValue_U();
				if (values.size() != names.size())
				{
					m_errorHandler->Error("", 0, "Mismatch in destructure quantities.");
					return;
				}
				
				for (size_t i = 0; i < names.size(); ++i)
				{
					auto varType = varTypes[i]->GetType();
					auto vecType = vecTypes[i];
					auto value = values[i];

					// check for type casting
					if (TOKEN_VAR_I32 == varType && value.IsDouble()) value = Literal(int32_t(value.DoubleValue()));
					if (TOKEN_VAR_F32 == varType && value.IsInt()) value = Literal(double(value.IntValue()));
					if ((TOKEN_VAR_STRING == varType && !value.IsString()) ||
						(TOKEN_VAR_VEC == varType && !value.IsVector()) ||
						(TOKEN_VAR_VEC != varType && value.IsVector()) ||
						(TOKEN_VAR_ENUM == varType && !value.IsEnum()) ||
						(TOKEN_VAR_VEC == varType && value.IsVector() && vecType != value.GetVecType())
						)
					{
						m_errorHandler->Error("", 0, "Unable to cast between types.");
					}

					m_environment->Define(names[i]->Lexeme(), value, stmt->FQNS(), stmt->Internal());
				}
			}
			else
			{
				m_errorHandler->Error("", 0, "Unable to destructure type.");
			}
		}
		
	}


	void VisitVarStatement(VarStmt* stmt)
	{
		Literal value;
		if (stmt->Expression())
		{
			value = Evaluate(stmt->Expression());
		}

		// check for type casting
		bool pass = false;
		TokenTypeEnum varType = stmt->VarType()->GetType();
		Literal::LiteralTypeEnum vecType = stmt->VarVecType();

		if (!value.IsRange())
		{
			if (value.IsInvalid())
			{
				if (TOKEN_VAR_I32 == varType) value = Literal(int32_t(0));
				if (TOKEN_VAR_F32 == varType) value = Literal(0.0);
				if (TOKEN_VAR_STRING == varType) value = Literal(std::string(""));
				if (TOKEN_VAR_ENUM == varType) value = Literal(EnumLiteral());
				if (TOKEN_VAR_BOOL == varType) value = Literal(false);
				if (TOKEN_VAR_VEC == varType)
				{
					if (Literal::LITERAL_TYPE_BOOL == vecType)
						value = Literal(std::vector<bool>());
					else if (Literal::LITERAL_TYPE_INTEGER == vecType)
						value = Literal(std::vector<int32_t>());
					else if (Literal::LITERAL_TYPE_DOUBLE == vecType)
						value = Literal(std::vector<double>());
					else if (Literal::LITERAL_TYPE_STRING == vecType)
						value = Literal(std::vector<std::string>());
					else if (Literal::LITERAL_TYPE_ENUM == vecType)
						value = Literal(std::vector<EnumLiteral>());
					else if (Literal::LITERAL_TYPE_TT_STRUCT == vecType)
						value = Literal(std::vector<Literal>(), vecType);
				}

				// user defined type
				if (TOKEN_IDENTIFIER == varType)
				{
					Literal vtype = m_environment->Get(stmt->VarType(), stmt->FQNS());

					if (!vtype.IsStructDef())
					{
						printf("Invalid user defined type in variable declaration.\n");
					}
					else
					{
						value = vtype.Call(this, LiteralList());
					}
				}
			}
			else
			{
				if (TOKEN_VAR_I32 == varType && value.IsDouble()) value = Literal(int32_t(value.DoubleValue()));
				if (TOKEN_VAR_F32 == varType && value.IsInt()) value = Literal(double(value.IntValue()));
				if ((TOKEN_VAR_STRING == varType && !value.IsString()) ||
					(TOKEN_VAR_VEC == varType && !value.IsVector()) ||
					(TOKEN_VAR_VEC != varType && value.IsVector()) ||
					(TOKEN_VAR_ENUM == varType && !value.IsEnum()) ||
					(TOKEN_VAR_VEC == varType && value.IsVector() && vecType != value.GetVecType())
					)
				{
					m_errorHandler->Error("", 0, "Unable to cast between types.");
				}
			}

			m_environment->Define(stmt->Operator()->Lexeme(), value, stmt->FQNS(), stmt->Internal());
		}
		else
		{
			m_errorHandler->Error("", 0, "Unable to assign range type.");
		}
	}


	void VisitPrintStatement(PrintStmt* stmt, bool newline = false)
	{
		char* nl = "";
		if (newline) nl = "\n";

		Literal literal = Evaluate(stmt->Expression());
		if (literal.IsString())
		{
			printf("%s%s", literal.ToString().c_str(), nl);
		}
		else
		{
			printf("%s%s", literal.ToString().c_str(), nl);
			//printf("Parser Error: Expected string expression.\n");
		}
	}


	void VisitBreakStatement(BreakStmt* stmt)
	{
		std::string scope = "BREAK:" + m_environment->GetNearestScopeLabel();
		//printf("throwing break scope: %s\n", scope.c_str());
		throw scope;
	}


	void VisitContinueStatement(ContinueStmt* stmt)
	{
		std::string scope = "CONTINUE:" + m_environment->GetNearestScopeLabel();
		//printf("throwing continue scope: %s\n", scope.c_str());
		throw scope;
	}


	void VisitWhileStatement(WhileStmt* stmt)
	{
		std::string scopeLabel = stmt->GetLabel();
		std::string breakLabel = "BREAK:" + scopeLabel;
		std::string continueLabel = "CONTINUE:" + scopeLabel;
		Environment* previous = m_environment;

		try
		{
			m_environment->SetNextScopeLabel(scopeLabel);
			//printf("entering scope: %s\n", scopeLabel.c_str());
			while (IsTruthy(Evaluate(stmt->GetCondition())))
			{
				try
				{
					Execute(stmt->GetBody());
				}
				catch (std::string label)
				{
					// throw if break
					if (0 != label.compare(continueLabel))
					{
						throw label;
					}
				}
				
				// post operation using in for loop
				Expr* post = stmt->GetPost();
				if (post) Evaluate(post);
			}
		}
		catch (std::string label)
		{
			//printf("caught scope: %s from inside: %s\n", label.c_str(), scopeLabel.c_str());
			if (0 != label.compare(breakLabel))
			{
				m_environment = previous;
				throw label;
			}
		}

		m_environment = previous;
		m_environment->SetNextScopeLabel(m_environment->GetNearestScopeLabel());
	}


	void VisitExpressionStatement(ExpressionStmt* stmt)
	{
		Literal literal = Evaluate(stmt->Expression());
		//printf("%s\n", literal.ToString().c_str());
	}


	Literal VisitAssign(AssignExpr* expr)
	{
		Literal value = Evaluate(expr->Right());
		int idx = -1;
		if (expr->VecIndex()) idx = Evaluate(expr->VecIndex()).IntValue();
		m_environment->Assign(expr->Operator()->Lexeme(), value, idx, expr->FQNS());
		return value;
	}


	Literal VisitBinary(BinaryExpr* expr)
	{
		Literal left = Evaluate(expr->Left());

		// don't evaluate right side if using explicit casting
		if (TOKEN_AS == expr->Operator()->GetType())
		{
			//CheckNumberOrStringOrEnumOperand(expr->Operator(), left);
			if (EXPRESSION_VARIABLE == expr->Right()->GetType())
			{
				TokenTypeEnum new_type = ((VariableExpr*)(expr->Right()))->Operator()->GetType();
				if (TOKEN_VAR_I32 == new_type)
				{
					if (left.IsInt()) return left;
					if (left.IsDouble()) return Literal(int32_t(left.DoubleValue()));
					if (left.IsBool()) return Literal(int32_t(left.BoolValue()));
					if (left.IsString())
					{
						int32_t ret = 0;
						try
						{
							ret = std::stoi(left.StringValue());
						}
						catch (std::invalid_argument)
						{
						}
						return ret;
					}
				}
				else if (TOKEN_VAR_F32 == new_type)
				{
					if (left.IsInt()) return Literal(left.DoubleValue());
					if (left.IsDouble()) return left;
					if (left.IsString())
					{
						double ret = 0;
						try
						{
							ret = std::stod(left.StringValue());
						}
						catch (std::invalid_argument)
						{
						}
						return ret;
					}
				}
				else if (TOKEN_VAR_STRING == new_type)
				{
					if (left.IsInt()) return Literal(std::to_string(left.IntValue()));
					if (left.IsDouble()) return Literal(std::to_string(left.DoubleValue()));
					if (left.IsString()) return left;
					if (left.IsEnum()) return Literal(left.EnumValue().enumValue);
					if (left.IsBool()) return Literal(left.ToString());
					if (left.IsVector()) return Literal(left.ToString());
				}
			}
			printf("Invalid explicit cast.\n");
			return Literal();
		}

		// keep going
		Literal right = Evaluate(expr->Right());

		switch (expr->Operator()->GetType())
		{
		case TOKEN_MINUS:
			CheckNumberOperand(expr->Operator(), left, right);
			if (left.IsDouble() || right.IsDouble())
			{
				// auto convert to double if one side is double
				return Literal(left.DoubleValue() - right.DoubleValue());
			}
			else
			{
				return Literal(left.IntValue() - right.IntValue());
			}
			
		case TOKEN_PLUS:
			CheckNumberOrStringOperand(expr->Operator(), left, right);
			if (left.IsNumeric() && right.IsNumeric())
			{
				if (left.IsDouble() || right.IsDouble())
				{
					// auto convert to double if one side is double
					return Literal(left.DoubleValue() + right.DoubleValue());
				}
				else
				{
					return Literal(left.IntValue() + right.IntValue());
				}
			}

			if (left.IsString() && right.IsString())
			{
				return Literal(left.StringValue() + right.StringValue());
			}

		case TOKEN_SLASH:
			CheckNumberOperand(expr->Operator(), left, right);
			// always auto convert to double
			return Literal(left.DoubleValue() / right.DoubleValue());

		case TOKEN_STAR:
			CheckNumberOperand(expr->Operator(), left, right);
			if (left.IsDouble() || right.IsDouble())
			{
				// auto convert to double if one side is double
				return Literal(left.DoubleValue() * right.DoubleValue());
			}
			else
			{
				return Literal(left.IntValue() * right.IntValue());
			}

		case TOKEN_PERCENT:
			CheckIntegerOperand(expr->Operator(), left, right);
			if (left.IsInt() && right.IsInt())
			{
				return Literal(int32_t(left.IntValue() % right.IntValue()));
			}
			return Literal();

		case TOKEN_DOT_DOT_EQUAL: // intentional fall-through
		case TOKEN_DOT_DOT:
		{
			CheckIntegerOperand(expr->Operator(), left, right);
			int rval = right.IntValue();
			if (TOKEN_DOT_DOT_EQUAL == expr->Operator()->GetType()) rval += 1;
			return Literal(left.IntValue(), rval);
		}
		
		case TOKEN_GREATER:
			CheckNumberOperand(expr->Operator(), left, right);
			// always check double value
			return (left.DoubleValue() > right.DoubleValue());

		case TOKEN_GREATER_EQUAL:
			CheckNumberOperand(expr->Operator(), left, right);
			// always check double value
			return (left.DoubleValue() >= right.DoubleValue());

		case TOKEN_LESS:
			CheckNumberOperand(expr->Operator(), left, right);
			// always check double value
			return (left.DoubleValue() < right.DoubleValue());

		case TOKEN_LESS_EQUAL:
			CheckNumberOperand(expr->Operator(), left, right);
			// always check double value
			return (left.DoubleValue() <= right.DoubleValue());

		case TOKEN_BANG_EQUAL:
			return !IsEqual(left, right);

		case TOKEN_EQUAL_EQUAL:
			return IsEqual(left, right);
		}

		return Literal();
	}


	Literal VisitBracket(BracketExpr* expr)
	{
		
		Literal::LiteralTypeEnum vecType = Literal::LITERAL_TYPE_INVALID;

		std::vector<bool> args_b;
		std::vector<int32_t> args_i;
		std::vector<double> args_d;
		std::vector<std::string> args_s;
		std::vector<EnumLiteral> args_e;
		std::vector<Literal> args_u;

		for (Expr* arg : expr->GetArguments())
		{
			if (EXPRESSION_REPLICATE == arg->GetType())
			{
				Literal lhs = Evaluate(((ReplicateExpr*)arg)->Left());
				Literal rhs = Evaluate(((ReplicateExpr*)arg)->Right());

				if (!rhs.IsInt() || (!lhs.IsInt() && !lhs.IsBool() && !lhs.IsDouble() && !lhs.IsString() && !lhs.IsEnum() && !lhs.IsInstance()))
				{
					m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid replicator.");
					continue;
				}

				int32_t rval = rhs.IntValue();
				for (size_t i = 0; i < rval; ++i)
				{
					if (lhs.IsBool())
					{
						vecType = Literal::LITERAL_TYPE_BOOL;
						args_b.push_back(lhs.BoolValue());
					}
					else if (lhs.IsInt())
					{
						vecType = Literal::LITERAL_TYPE_INTEGER;
						args_i.push_back(lhs.IntValue());
					}
					else if (lhs.IsDouble())
					{
						vecType = Literal::LITERAL_TYPE_DOUBLE;
						args_d.push_back(lhs.DoubleValue());
					}
					else if (lhs.IsString())
					{
						vecType = Literal::LITERAL_TYPE_STRING;
						args_s.push_back(lhs.StringValue());
					}
					else if (lhs.IsEnum())
					{
						vecType = Literal::LITERAL_TYPE_ENUM;
						args_e.push_back(lhs.EnumValue());
					}
					else if (lhs.IsInstance())
					{
						vecType = Literal::LITERAL_TYPE_TT_STRUCT;
						args_u.push_back(lhs);
					}
				}
				continue;
			}

			Literal x = Evaluate(arg);
			if (x.IsVector())
			{
				/*if (x.IsVecBool())
				{
					if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_BOOL == vecType)
					{
						vecType = Literal::LITERAL_TYPE_BOOL;
						std::vector<bool> v = x.VecValue_B();
						if (!args_b.empty()) {
							args_b.insert(args_b.end(), v.begin(), v.end());
						} else {
							args_b = v;
						}
					}
					else
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
					}
				}
				else if (x.IsVecInteger())
				{
					if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_INTEGER == vecType)
					{
						vecType = Literal::LITERAL_TYPE_INTEGER;
						std::vector<int32_t> v = x.VecValue_I();
						if (!args_i.empty()) {
							args_i.insert(args_i.end(), v.begin(), v.end());
						} else {
							args_i = v;
						}
					}
					else
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
					}
				}
				else if (x.IsVecDouble())
				{
					if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_DOUBLE == vecType)
					{
						vecType = Literal::LITERAL_TYPE_DOUBLE;
						std::vector<double> v = x.VecValue_D();
						if (!args_d.empty()) {
							args_d.insert(args_d.end(), v.begin(), v.end());
						} else {
							args_d = v;
						}
					}
					else
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
					}
				}
				else if (x.IsVecString())
				{
					if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_STRING == vecType)
					{
						vecType = Literal::LITERAL_TYPE_STRING;
						std::vector<std::string> v = x.VecValue_S();
						if (!args_s.empty()) {
							args_s.insert(args_s.end(), v.begin(), v.end());
						} else {
							args_s = v;
						}
					}
					else
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
					}
				}
				else if (x.IsVecEnum())
				{
					if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_ENUM == vecType)
					{
						vecType = Literal::LITERAL_TYPE_ENUM;
						std::vector<EnumLiteral> v = x.VecValue_E();
						if (!args_e.empty()) {
							args_e.insert(args_e.end(), v.begin(), v.end());
						} else {
							args_e = v;
						}
					}
					else
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
					}
				}
				else if (x.IsVecStruct())
				{
					if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_TT_STRUCT == vecType)
					{
						vecType = Literal::LITERAL_TYPE_TT_STRUCT;
						std::vector<Literal> v = x.VecValue_U();
						if (!args_u.empty()) {
							args_u.insert(args_u.end(), v.begin(), v.end());
						} else {
							args_u = v;
						}
					}
					else
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
					}
				}*/
				m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Use vec::append() to append vectors");
			}
			else if (x.IsRange())
			{
				if (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_INTEGER == vecType)
				{
					vecType = Literal::LITERAL_TYPE_INTEGER;
					int lhs = x.LeftValue();
					int rhs = x.RightValue();
					/*int newsz = args_i.size() + rhs - lhs;
					if (newsz < 0) newsz = 0;
					if (args_i.capacity() < newsz) args_i.reserve(newsz);*/
					for (int i = lhs; i < rhs; ++i)
					{
						args_i.push_back(i);
					}
				}
			}
			else
			{
				if (x.IsBool() && (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_BOOL == vecType))
				{
					vecType = Literal::LITERAL_TYPE_BOOL;
					args_b.push_back(x.BoolValue());
				}
				else if (x.IsInt() && (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_INTEGER == vecType))
				{
					vecType = Literal::LITERAL_TYPE_INTEGER;
					args_i.push_back(x.IntValue());
				}
				else if (x.IsDouble() && (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_DOUBLE == vecType))
				{
					vecType = Literal::LITERAL_TYPE_DOUBLE;
					args_d.push_back(x.DoubleValue());
				}
				else if (x.IsString() && (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_STRING == vecType))
				{
					vecType = Literal::LITERAL_TYPE_STRING;
					args_s.push_back(x.StringValue());
				}
				else if (x.IsEnum() && (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_ENUM == vecType))
				{
					vecType = Literal::LITERAL_TYPE_ENUM;
					args_e.push_back(x.EnumValue());
				}
				else if (x.IsInstance() && (Literal::LITERAL_TYPE_INVALID == vecType || Literal::LITERAL_TYPE_TT_STRUCT == vecType))
				{
					vecType = Literal::LITERAL_TYPE_TT_STRUCT;
					args_u.push_back(x);
				}
				else
				{
					m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid type in [].");
				}
			}
		}

		if (Literal::LITERAL_TYPE_BOOL == vecType) return Literal(args_b);
		if (Literal::LITERAL_TYPE_INTEGER == vecType) return Literal(args_i);
		if (Literal::LITERAL_TYPE_DOUBLE == vecType) return Literal(args_d);
		if (Literal::LITERAL_TYPE_STRING == vecType) return Literal(args_s);
		if (Literal::LITERAL_TYPE_ENUM == vecType) return Literal(args_e);
		if (Literal::LITERAL_TYPE_TT_STRUCT == vecType) return Literal(args_u, vecType);

		return Literal();
	}


	Literal VisitCall(CallExpr* expr)
	{
		Literal callee = Evaluate(expr->GetCallee());

		ArgList arglist = expr->GetArguments();
		LiteralList args;

		if (1 == arglist.size())
		{
			if (EXPRESSION_STRUCTURE == arglist[0]->GetType())
			{
				StructExpr* argexpr = (StructExpr*)arglist[0];
				for (Expr* arg : argexpr->GetArguments())
				{
					args.push_back(Evaluate(arg));
				}
			}
			else
			{
				args.push_back(Evaluate(arglist[0]));
			}
		}

		if (!callee.IsCallable())
		{
			printf("Can only call functions.\n");
			return Literal();
		}

		if (callee.ExplicitArgs() && args.size() != callee.Arity())
		{
			printf("Expected %d arguments for '%s', but found %d.\n", callee.Arity(), callee.ToString().c_str(), args.size());
			return Literal();
		}

		return callee.Call(this, args);
	}


	Literal VisitDestructure(DestructExpr* expr)
	{
		ArgList lhs = expr->GetLhsArguments();
		ArgList rhs = expr->GetRhsArguments();

		LiteralList values;
		// evaluate rhs first
		for (size_t i = 0; i < rhs.size(); ++i)
		{
			values.push_back(Evaluate(rhs[i]));
		}

		// make assignments
		for (size_t i = 0; i < lhs.size(); ++i)
		{
			if (lhs[i]->GetType() == EXPRESSION_VARIABLE)
			{
				VariableExpr* expr = (VariableExpr*)lhs[i];
				int idx = -1;
				if (expr->VecIndex()) idx = Evaluate(expr->VecIndex()).IntValue();
				m_environment->Assign(expr->Operator()->Lexeme(), values[i], idx, expr->FQNS());
			}
			else
			{
				printf("Can't destructure into structure members.\n");
			}
		}

		return Literal();
	}


	Literal VisitLiteral(LiteralExpr* expr)
	{
		return expr->GetLiteral();
	}


	Literal VisitLogical(LogicalExpr* expr)
	{
		Literal left = Evaluate(expr->Left());

		if (expr->Operator()->GetType() == TOKEN_OR)
		{
			if (IsTruthy(left)) return true;
		}
		else
		{
			if (!IsTruthy(left)) return false;
		}

		return Evaluate(expr->Right());
	}


	Literal VisitGroup(GroupExpr* expr)
	{
		return Evaluate(expr->Expression());
	}


	Literal VisitRange(RangeExpr* expr)
	{
		Literal left = Evaluate(expr->Left());
		Literal right = Evaluate(expr->Right());

		if (!left.IsInt() || !right.IsInt())
		{
			m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid range.");
			return Literal();
		}

		int32_t lval = left.IntValue();
		int32_t rval = right.IntValue();

		if (TOKEN_DOT_DOT_EQUAL == expr->Operator()->GetType()) rval += 1;

		return Literal(lval, rval);
	}


	Literal VisitStructure(StructExpr* expr)
	{
		LiteralList args;
		for (Expr* arg : expr->GetArguments())
		{
			args.push_back(Evaluate(arg));
		}

		return Literal(args, Literal::LITERAL_TYPE_ANONYMOUS);
	}


	Literal VisitUnary(UnaryExpr* expr)
	{
		Literal right = Evaluate(expr->Right());

		switch (expr->Operator()->GetType())
		{
		case TOKEN_BANG:
			return !IsTruthy(right);

		case TOKEN_MINUS:
			CheckNumberOperand(expr->Operator(), right);
			if (right.IsDouble())
			{
				return Literal(right.DoubleValue() * -1);
			}
			else
			{
				return Literal(right.IntValue() * -1);
			}
		}

		return Literal();
	}

	Literal VisitVariable(VariableExpr* expr)
	{
		Literal v = m_environment->Get(expr->Operator(), expr->FQNS());
		if (v.IsVector())
		{
			if (expr->VecIndex())
			{
				Literal x = Evaluate(expr->VecIndex());
				if (x.IsInt())
				{
					int32_t idx = x.IntValue();
					if (idx < 0 || idx >= v.Len())
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Vector index [" + std::to_string(idx) + "] out of bounds (Size: " + std::to_string(v.Len()) + ").");
					}
					else
					{
						if (v.IsVecBool()) return Literal(v.VecValueAt_B(idx));
						if (v.IsVecInteger()) return Literal(v.VecValueAt_I(idx));
						if (v.IsVecDouble()) return Literal(v.VecValueAt_D(idx));
						if (v.IsVecString()) return Literal(v.VecValueAt_S(idx));
						if (v.IsVecEnum()) return Literal(v.VecValueAt_E(idx));
						if (v.IsVecStruct()) return Literal(v.VecValueAt_U(idx));
					}
				}
				else if (x.IsRange())
				{
					int lhs = x.LeftValue();
					int rhs = x.RightValue();

					if (lhs < 0 || lhs >= v.Len())
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Vector index [" + std::to_string(lhs) + "] out of bounds (Size: " + std::to_string(v.Len()) + ").");
					}
					else if (rhs < 0 || rhs >= v.Len())
					{
						m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Vector index [" + std::to_string(rhs) + "] out of bounds (Size: " + std::to_string(v.Len()) + ").");
					}
					else
					{
						if (v.IsVecBool())
						{
							std::vector<bool> temp = v.VecValue_B();
							std::vector<bool> ret;
							ret.insert(ret.begin(), temp.begin() + lhs, temp.begin() + rhs);
							return ret;
						}
						else if (v.IsVecInteger())
						{
							std::vector<int32_t> temp = v.VecValue_I();
							std::vector<int32_t> ret;
							ret.insert(ret.begin(), temp.begin() + lhs, temp.begin() + rhs);
							return ret;
						}
						else if (v.IsVecDouble())
						{
							std::vector<double> temp = v.VecValue_D();
							std::vector<double> ret;
							ret.insert(ret.begin(), temp.begin() + lhs, temp.begin() + rhs);
							return ret;
						}
						else if (v.IsVecString())
						{
							std::vector<std::string> temp = v.VecValue_S();
							std::vector<std::string> ret;
							ret.insert(ret.begin(), temp.begin() + lhs, temp.begin() + rhs);
							return ret;
						}
						else if (v.IsVecEnum())
						{
							std::vector<EnumLiteral> temp = v.VecValue_E();
							std::vector<EnumLiteral> ret;
							ret.insert(ret.begin(), temp.begin() + lhs, temp.begin() + rhs);
							return ret;
						}
						else if (v.IsVecStruct())
						{
							std::vector<Literal> temp = v.VecValue_U();
							std::vector<Literal> ret;
							ret.insert(ret.begin(), temp.begin() + lhs, temp.begin() + rhs);
							return Literal(ret, Literal::LITERAL_TYPE_TT_STRUCT);
						}
					}
				}
				else
				{
					m_errorHandler->Error(expr->Operator()->Filename(), expr->Operator()->Line(), "Invalid vector index provided.");
				}
			}
		}
		return v;
	}

	Literal VisitGet(GetExpr* expr)
	{
		Expr* obj = expr->Object();
		Literal v = Evaluate(obj);

		if (v.IsInstance())
		{
			Literal ret = v.GetParameter(expr->Name()->Lexeme());
			if (ret.IsInvalid())
			{
				m_errorHandler->Error(expr->Name()->Filename(), expr->Name()->Line(), "Invalid property '" + expr->Name()->Lexeme() + "'.");
			}
			else
			{
				int idx = -1;
				if (expr->VecIndex()) idx = Evaluate(expr->VecIndex()).IntValue();
				if (idx != -1 && ret.IsVector())
				{
					if (ret.IsVecBool()) return ret.VecValueAt_B(idx);
					if (ret.IsVecInteger()) return ret.VecValueAt_I(idx);
					if (ret.IsVecDouble()) return ret.VecValueAt_D(idx);
					if (ret.IsVecString()) return ret.VecValueAt_S(idx);
					if (ret.IsVecEnum()) return ret.VecValueAt_E(idx);
					if (ret.IsVecStruct()) return ret.VecValueAt_U(idx);
				}
			}
			return ret;
		}

		m_errorHandler->Error(expr->Name()->Filename(), expr->Name()->Line(), "Only instances have properties.");
		return v;
	}

	Literal VisitSet(SetExpr* expr)
	{
		Literal v = Evaluate(expr->Object());
		if (v.IsInstance())
		{
			Literal value = Evaluate(expr->Value());

			Literal ret = v.GetParameter(expr->Name()->Lexeme());
			if (!ret.IsInvalid())
			{
				int idx = -1;
				if (expr->VecIndex()) idx = Evaluate(expr->VecIndex()).IntValue();

				if (!v.SetParameter(expr->Name()->Lexeme(), value, idx))
				{
					m_errorHandler->Error(expr->Name()->Filename(), expr->Name()->Line(), "Cannot cast to type of property '" + expr->Name()->Lexeme() + "'.");
				}
			}
			else
			{
				m_errorHandler->Error(expr->Name()->Filename(), expr->Name()->Line(), "Invalid property '" + expr->Name()->Lexeme() + "'.");
			}

			if (EXPRESSION_VARIABLE == expr->Object()->GetType())
			{
				int idx = -1;
				VariableExpr* obj = (VariableExpr*)expr->Object();
				if (obj->VecIndex()) idx = Evaluate(obj->VecIndex()).IntValue();

				m_environment->Assign(obj->Operator()->Lexeme(), v, idx, obj->FQNS());
			}
			return v;
		}

		m_errorHandler->Error(expr->Name()->Filename(), expr->Name()->Line(), "Only instances have properties.");
		return Literal();
	}

	void CheckNumberOperand(Token* token, const Literal& left)
	{
		if (left.IsNumeric()) return;
		m_errorHandler->Error(token->Filename(), token->Line(), "Operand must be a numeric.");
	}

	void CheckNumberOperand(Token* token, const Literal& left, const Literal& right)
	{
		if (left.IsNumeric() && right.IsNumeric()) return;
		m_errorHandler->Error(token->Filename(), token->Line(), "Operands must be numeric.");
	}

	void CheckIntegerOperand(Token* token, const Literal& left, const Literal& right)
	{
		if (left.IsInt() && right.IsInt()) return;
		m_errorHandler->Error(token->Filename(), token->Line(), "Operands must be integers.");
	}

	void CheckNumberOrStringOperand(Token* token, const Literal& left)
	{
		if (!(left.IsNumeric() || left.IsString()))
		{
			m_errorHandler->Error(token->Filename(), token->Line(), "Operand must be number or string.");
		}
		return;
	}

	void CheckNumberOrStringOperand(Token* token, const Literal& left, const Literal& right)
	{
		if (!((left.IsNumeric() && right.IsNumeric()) || (left.IsString() && right.IsString())))
		{
			m_errorHandler->Error(token->Filename(), token->Line(), "Operands must both be numbers or strings.");
		}
		return;
	}

	bool IsTruthy(const Literal& literal)
	{
		if (literal.IsInt()) return literal.IntValue() != 0;
		if (literal.IsBool()) return literal.BoolValue();
		return true;
	}

	bool IsEqual(const Literal& left, const Literal& right)
	{
		return left.Equals(right);
	}


private:

	ErrorHandler* m_errorHandler;
	Environment* m_environment;
	Environment* m_globals;

};


#endif // INTERPRETER_H