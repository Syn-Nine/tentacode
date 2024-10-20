#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include <string>
#include <cstdarg>

#include "TValue.h"
#include "Token.h"
#include "Literal.h"
#include "Environment.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class Expr
{
public:
	virtual ExpressionTypeEnum GetType() = 0;
	virtual TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env) = 0;
};

typedef std::vector<Expr*> ArgList;


//-----------------------------------------------------------------------------
class VariableExpr : public Expr
{
public:
	VariableExpr() = delete;
	VariableExpr(Token* name, Expr* right)
	{
		m_token = name;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_VARIABLE; }

	Token* Operator() { return m_token; }
	Expr* VecIndex() { return m_right; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("VariableExpr::codegen()\n");
		AllocaInst* a = env->GetVariable(m_token->Lexeme());
		LiteralTypeEnum type = env->GetVariableType(m_token->Lexeme());

		if (LITERAL_TYPE_INTEGER == type || LITERAL_TYPE_BOOL == type || LITERAL_TYPE_DOUBLE == type)
		{
			return TValue(type, builder->CreateLoad(a->getAllocatedType(), a, "loadtmp"));
		}
		else if (LITERAL_TYPE_STRING == type)
		{
			return TValue(type, builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
		}

		return TValue::NullInvalid();
	}

private:
	Token* m_token;
	Expr* m_right;
};


//-----------------------------------------------------------------------------
class AssignExpr : public Expr
{
public:
	AssignExpr() = delete;
	AssignExpr(Token* name, Expr* right)
	{
		m_token = name;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_ASSIGN; }

	Token* Operator() { return m_token; }
	Expr* Right() { return m_right; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("AssignExpr::codegen()\n");
		TValue rhs = m_right->codegen(context, builder, module, env);

		AllocaInst* a = env->GetVariable(m_token->Lexeme());
		LiteralTypeEnum type = env->GetVariableType(m_token->Lexeme());

		if (LITERAL_TYPE_INTEGER == type || LITERAL_TYPE_BOOL == type || LITERAL_TYPE_DOUBLE == type)
		{
			builder->CreateStore(rhs.value, a);
		}
		else if (LITERAL_TYPE_STRING == type)
		{
			TValue b = TValue::String(builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
			builder->CreateStore(builder->getInt8(0), b.value);
			builder->CreateCall(module->getFunction("strncpy"), { b.value, rhs.value, builder->getInt32(1023) }, "calltmp");
		}

		return TValue::NullInvalid();
	}

private:
	Token* m_token;
	Expr* m_right;
};




//-----------------------------------------------------------------------------
class BinaryExpr : public Expr
{
public:
	BinaryExpr() = delete;
	BinaryExpr(Expr* left, Token* name, Expr* right)
	{
		m_left = left;
		m_token = name;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_BINARY; }
	
	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("BinaryExpr::codegen()\n");

		if (!m_left || !m_right) return TValue::NullInvalid();
		TValue lhs = m_left->codegen(context, builder, module, env);
		if (!lhs.value) return TValue::NullInvalid();

		if (TOKEN_AS == m_token->GetType() && EXPRESSION_VARIABLE == m_right->GetType())
		{
			TokenTypeEnum new_type = (static_cast<VariableExpr*>(m_right))->Operator()->GetType();

			if (TOKEN_VAR_I32 == new_type)
			{
				if (lhs.IsInteger()) return lhs;
				if (lhs.IsDouble())
				{
					return TValue::Integer(builder->CreateFPToSI(lhs.value, builder->getInt32Ty(), "int_cast_tmp"));
				}
				if (lhs.IsString())
				{
					return TValue::Integer(builder->CreateCall(module->getFunction("atoi"), { lhs.value }, "calltmp"));
				}
			}
			else if (TOKEN_VAR_F32 == new_type)
			{
				if (lhs.IsDouble()) return lhs;
				if (lhs.IsInteger())
				{
					return TValue::Double(builder->CreateSIToFP(lhs.value, builder->getDoubleTy(), "cast_to_dbl"));
				}
				if (lhs.IsString())
				{
					return TValue::Double(builder->CreateCall(module->getFunction("atof"), { lhs.value }, "calltmp"));
				}
			}
			else if (TOKEN_VAR_STRING == new_type)
			{
				if (lhs.IsString()) return lhs;
				
				AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(33), "alloctmp");
				TValue b = TValue::String(builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
				builder->CreateStore(builder->getInt8(0), b.value);

				if (lhs.IsInteger())
				{
					builder->CreateCall(module->getFunction("itoa2"), { lhs.value, b.value }, "calltmp");
				}
				else if (lhs.IsDouble())
				{
					builder->CreateCall(module->getFunction("ftoa2"), { lhs.value, b.value }, "calltmp");
				}
				else if (lhs.IsBool())
				{
					builder->CreateCall(module->getFunction("btoa2"), { lhs.value, b.value }, "calltmp");
				}
				return b;
			}
		}


		TValue rhs = m_right->codegen(context, builder, module, env);
		if (!lhs.value || !rhs.value) return TValue::NullInvalid();

		if (lhs.IsInteger() && rhs.IsDouble())
		{
			// convert lhs to double
			lhs = TValue::Double(builder->CreateSIToFP(lhs.value, builder->getDoubleTy(), "cast_to_dbl"));
		}
		if (rhs.IsInteger() && lhs.IsDouble())
		{
			// convert rhs to double
			rhs = TValue::Double(builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_int"));
		}

		switch (m_token->GetType())
		{
		case TOKEN_MINUS:
			if (lhs.IsDouble()) return TValue::Double(builder->CreateFSub(lhs.value, rhs.value, "subtmp"));
			if (lhs.IsInteger()) return TValue::Integer(builder->CreateSub(lhs.value, rhs.value, "subtmp"));
			break;

		case TOKEN_PLUS:
			if (lhs.IsDouble()) return TValue::Double(builder->CreateFAdd(lhs.value, rhs.value, "addtmp"));
			if (lhs.IsInteger()) return TValue::Integer(builder->CreateAdd(lhs.value, rhs.value, "addtmp"));
			if (lhs.IsString() && rhs.IsString())
			{
				AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(1024), "alloctmp");
				TValue b = TValue::String(builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
				builder->CreateStore(builder->getInt8(0), b.value);
				builder->CreateCall(module->getFunction("strcat"), { b.value, lhs.value }, "calltmp");
				builder->CreateCall(module->getFunction("strcat"), { b.value, rhs.value }, "calltmp");
				return b;
			}
			break;

		case TOKEN_STAR:
			if (lhs.IsDouble()) return TValue::Double(builder->CreateFMul(lhs.value, rhs.value, "multmp"));
			if (lhs.IsInteger()) return TValue::Integer(builder->CreateMul(lhs.value, rhs.value, "multmp"));
			break;

		case TOKEN_SLASH:
			if (lhs.IsDouble()) return TValue::Double(builder->CreateFDiv(lhs.value, rhs.value, "divtmp"));
			break;

		case TOKEN_GREATER:
			if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpUGT(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSGT(lhs.value, rhs.value, "cmptmp"));
			break;

		case TOKEN_GREATER_EQUAL:
			if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpUGE(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSGE(lhs.value, rhs.value, "cmptmp"));
			break;

		case TOKEN_LESS:
			if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpULT(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSLT(lhs.value, rhs.value, "cmptmp"));
			break;

		case TOKEN_LESS_EQUAL:
			if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpULE(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpSLE(lhs.value, rhs.value, "cmptmp"));
			break;

		case TOKEN_BANG_EQUAL:
			if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpUNE(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpNE(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsString() && rhs.IsString())
			{
				std::vector<Value*> args;
				args.push_back(lhs.value);
				args.push_back(rhs.value);
				Value* v = builder->CreateCall(module->getFunction("strcmp"), args, "calltmp");
				return TValue::Bool(builder->CreateICmpNE(v, Constant::getNullValue(builder->getInt32Ty()), "boolcmp"));
			}
			break;

		case TOKEN_EQUAL_EQUAL:
			if (lhs.IsDouble()) return TValue::Bool(builder->CreateFCmpUEQ(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsInteger()) return TValue::Bool(builder->CreateICmpEQ(lhs.value, rhs.value, "cmptmp"));
			if (lhs.IsString() && rhs.IsString())
			{
				std::vector<Value*> args;
				args.push_back(lhs.value);
				args.push_back(rhs.value);
				Value* v = builder->CreateCall(module->getFunction("strcmp"), args, "calltmp");
				return TValue::Bool(builder->CreateICmpEQ(v, Constant::getNullValue(builder->getInt32Ty()), "boolcmp"));
			}
			break;
		}

		return TValue::NullInvalid();
	}

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};



//-----------------------------------------------------------------------------
class CallExpr : public Expr
{
public:
	CallExpr() = delete;
	CallExpr(Expr* callee, Token* paren, ArgList arguments)
	{
		m_callee = callee;
		m_token = paren;
		m_arguments = arguments;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_CALL; }

	Expr* GetCallee() { return m_callee; }
	Token* Operator() { return m_token; }
	ArgList GetArguments() { return m_arguments; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("CallExpr::codegen()\n");
		if (!m_callee) return TValue::NullInvalid();
		if (EXPRESSION_VARIABLE != m_callee->GetType()) return TValue::NullInvalid();

		std::string name = (static_cast<VariableExpr*>(m_callee))->Operator()->Lexeme();
		Function* callee = env->GetFunction(name);

		if (0 == name.compare("input"))
		{
			AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(256), "alloctmp");
			TValue b = TValue::String(builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
			builder->CreateStore(builder->getInt8(0), b.value);
			builder->CreateCall(module->getFunction("input"), { b.value }, "calltmp");
			return b;
		}
		else
		{
			return TValue::Double(builder->CreateCall(callee, {}, "calltmp"));
		}

		return TValue::NullInvalid();

	}

private:
	Expr* m_callee;
	Token* m_token;
	ArgList m_arguments;
};



//-----------------------------------------------------------------------------
class GroupExpr : public Expr
{
public:
	GroupExpr() = delete;
	GroupExpr(Expr* expression)
	{
		m_expression = expression;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_GROUP; }
	
	Expr* Expression() { return m_expression; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("GroupExpr::codegen()\n");
		return m_expression->codegen(context, builder, module, env);
	}

private:
	Expr* m_expression;
};



//-----------------------------------------------------------------------------
class LiteralExpr : public Expr
{
public:
	LiteralExpr() = delete; // null not allowed
	LiteralExpr(int32_t val) : m_literal(val) {}
	LiteralExpr(double val) : m_literal(val) {}
	LiteralExpr(std::string val) : m_literal(val) {}
	LiteralExpr(bool val) : m_literal(val) {}

	std::string ToString() { return m_literal.ToString(); }

	ExpressionTypeEnum GetType() { return EXPRESSION_LITERAL; }
	
	Literal GetLiteral() { return m_literal; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("LiteralExpr::codegen()\n");

		LiteralTypeEnum lt = m_literal.GetType();

		if (LITERAL_TYPE_INTEGER == lt)
		{
			return TValue(lt, ConstantInt::get(*context, APInt(32, m_literal.IntValue(), true)));
		}
		else if (LITERAL_TYPE_BOOL == lt)
		{
			if (m_literal.BoolValue())
				return TValue(lt, builder->getTrue());
			else
				return TValue(lt, builder->getFalse());
		}
		else if (LITERAL_TYPE_DOUBLE == lt)
		{
			return TValue(lt, ConstantFP::get(*context, APFloat(m_literal.DoubleValue())));
		}
		else if (LITERAL_TYPE_STRING == lt)
		{
			return TValue(lt, builder->CreateGlobalStringPtr(m_literal.ToString(), "strtmp"));
		}

		return TValue::NullInvalid();
	}
	

private:
	Literal m_literal;
};



//-----------------------------------------------------------------------------
class LogicalExpr : public Expr
{
public:
	LogicalExpr() = delete;
	LogicalExpr(Expr* left, Token* token, Expr* right)
	{
		m_token = token;
		m_left = left;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_LOGICAL; }
	
	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }
	
	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("LogicalExpr::codegen()\n");

		if (!m_left || !m_right) return TValue::NullInvalid();
		TValue lhs = m_left->codegen(context, builder, module, env);
		TValue rhs = m_right->codegen(context, builder, module, env);
		if (!lhs.value || !rhs.value) return TValue::NullInvalid();

		switch (m_token->GetType())
		{
		case TOKEN_AND:
			return TValue::Bool(builder->CreateAnd(lhs.value, rhs.value, "logical_and_cmp"));
			break;
		
		case TOKEN_OR:
			return TValue::Bool(builder->CreateOr(lhs.value, rhs.value, "logical_or_cmp"));
			break;
		}

		return TValue::NullInvalid();
	}

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};




//-----------------------------------------------------------------------------
class RangeExpr : public Expr
{
public:
	RangeExpr() = delete;
	RangeExpr(Expr* left, Token* name, Expr* right)
	{
		m_left = left;
		m_token = name;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_RANGE; }
	
	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("RangeExpr::codegen()\n");

		if (!m_left || !m_right) return TValue::NullInvalid();
		TValue lhs = m_left->codegen(context, builder, module, env);
		TValue rhs = m_right->codegen(context, builder, module, env);
		if (!lhs.value || !rhs.value) return TValue::NullInvalid();

		printf("test\n");

		return TValue::NullInvalid();
	}

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};



//-----------------------------------------------------------------------------
class UnaryExpr : public Expr
{
public:
	UnaryExpr() = delete;
	UnaryExpr(Token* token, Expr* right)
	{
		m_token = token;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_UNARY; }
	
	Token* Operator() { return m_token; }
	Expr* Right() { return m_right; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("UnaryExpr::codegen()\n");
		
		TValue rhs = m_right->codegen(context, builder, module, env);
		
		switch (m_token->GetType())
		{
		case TOKEN_MINUS:
			if (rhs.IsInteger())
				return TValue::Integer(builder->CreateNeg(rhs.value, "negtmp"));
			else if (rhs.IsDouble())
				return TValue::Double(builder->CreateFNeg(rhs.value, "negtmp"));
			break;

		case TOKEN_BANG:
			if (rhs.IsInteger())
			{
				return TValue::Integer(builder->CreateICmpEQ(rhs.value, Constant::getNullValue(builder->getInt32Ty()), "zero_check"));
			}
			else if (rhs.IsBool())
			{
				return TValue::Bool(builder->CreateNot(rhs.value, "nottmp"));
			}
			break;
		}
		
		return TValue::NullInvalid();
	}

private:
	Token* m_token;
	Expr* m_right;
};





#endif // EXPRESSIONS_H