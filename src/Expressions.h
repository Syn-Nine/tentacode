#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include <string>
#include <cstdarg>

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
	virtual Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID) = 0;
	virtual LiteralTypeEnum GetLiteralType(Environment* env) = 0;
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

	LiteralTypeEnum GetLiteralType(Environment* env) { return env->GetVariableType(m_token->Lexeme()); }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("VariableExpr::codegen()\n");
		AllocaInst* a = env->GetVariable(m_token->Lexeme());
		LiteralTypeEnum type = env->GetVariableType(m_token->Lexeme());

		if (LITERAL_TYPE_INTEGER == type || LITERAL_TYPE_BOOL == type || LITERAL_TYPE_DOUBLE == type)
		{
			return builder->CreateLoad(a->getAllocatedType(), a, "loadtmp");
		}
		else if (LITERAL_TYPE_STRING == type)
		{
			return builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp");
		}

		return nullptr;
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

	LiteralTypeEnum GetLiteralType(Environment* env) { return m_right->GetLiteralType(env); }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("AssignExpr::codegen()\n");
		Value* rhs = m_right->codegen(context, builder, module, env, typeHint);

		AllocaInst* a = env->GetVariable(m_token->Lexeme());
		LiteralTypeEnum type = env->GetVariableType(m_token->Lexeme());

		if (LITERAL_TYPE_INTEGER == type || LITERAL_TYPE_BOOL == type || LITERAL_TYPE_DOUBLE == type)
		{
			return builder->CreateStore(rhs, a);
		}
		else if (LITERAL_TYPE_STRING == type)
		{
			Value* b = builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp");
			builder->CreateStore(builder->getInt8(0), b);
			return builder->CreateCall(module->getFunction("strncpy"), { b, rhs, builder->getInt32(1023) }, "calltmp");
		}

		return nullptr;
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
	LiteralTypeEnum GetLiteralType(Environment* env)
	{
		LiteralTypeEnum ltype = m_left->GetLiteralType(env);
		LiteralTypeEnum rtype = m_right->GetLiteralType(env);

		switch (m_token->GetType())
		{
		case TOKEN_MINUS: // intentional fall-through
		case TOKEN_PLUS:
		case TOKEN_STAR:
		case TOKEN_GREATER:
		case TOKEN_GREATER_EQUAL:
		case TOKEN_LESS:
		case TOKEN_LESS_EQUAL:
		case TOKEN_BANG_EQUAL:
		case TOKEN_EQUAL_EQUAL:
			if (ltype == rtype) return ltype;
			if ((LITERAL_TYPE_INTEGER == ltype && LITERAL_TYPE_DOUBLE == rtype) ||
				(LITERAL_TYPE_DOUBLE == ltype && LITERAL_TYPE_INTEGER == rtype))
				return LITERAL_TYPE_DOUBLE;
			
			break;
		
		case TOKEN_SLASH:
			return LITERAL_TYPE_DOUBLE;
			break;

		case TOKEN_AS:
			return rtype;
			break;
		}
		
		return LITERAL_TYPE_INVALID;
	}

	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("BinaryExpr::codegen()\n");

		LiteralTypeEnum lt = this->GetLiteralType(env);
		if (LITERAL_TYPE_INVALID != typeHint) lt = typeHint;

		if (!m_left || !m_right) return nullptr;
		Value* lhs = m_left->codegen(context, builder, module, env, lt);
		if (!lhs) return nullptr;

		if (TOKEN_AS == m_token->GetType() && EXPRESSION_VARIABLE == m_right->GetType())
		{
			Type* ltype = lhs->getType();
			TokenTypeEnum new_type = (static_cast<VariableExpr*>(m_right))->Operator()->GetType();

			if (TOKEN_VAR_I32 == new_type)
			{
				if (ltype->isIntegerTy()) return lhs;
				if (ltype->isDoubleTy())
				{
					// convert lhs to int
					return builder->CreateFPToSI(lhs, builder->getInt32Ty(), "int_cast_tmp");
				}
				if (ltype->isPointerTy())
				{
					// else string
					return builder->CreateCall(module->getFunction("atoi"), { lhs }, "calltmp");
				}
			}
			else if (TOKEN_VAR_F32 == new_type)
			{
				if (ltype->isDoubleTy()) return lhs;

				if (ltype->isIntegerTy())
				{
					// convert lhs to double
					lhs = builder->CreateSIToFP(lhs, builder->getDoubleTy(), "cast_to_dbl");
					lt = LITERAL_TYPE_DOUBLE;
				}
				if (ltype->isPointerTy())
				{
					// else string
					return builder->CreateCall(module->getFunction("atof"), { lhs }, "calltmp");
				}
			}
			else if (TOKEN_VAR_STRING == new_type)
			{
				if (ltype->isIntegerTy() || ltype->isDoubleTy() || LITERAL_TYPE_BOOL == lt)
				{
					AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(33), "alloctmp");
					Value* b = builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp");
					builder->CreateStore(builder->getInt8(0), b);

					if (ltype->isIntegerTy())
					{
						builder->CreateCall(module->getFunction("itoa2"), { lhs, b }, "calltmp");
					}
					else if (ltype->isDoubleTy())
					{
						builder->CreateCall(module->getFunction("ftoa2"), { lhs, b }, "calltmp");
					}
					else if (LITERAL_TYPE_BOOL == lt)
					{
						builder->CreateCall(module->getFunction("btoa2"), { lhs, b }, "calltmp");
					}
					return b;
				}
			}
		}


		Value* rhs = m_right->codegen(context, builder, module, env, lt);
		if (!lhs || !rhs) return nullptr;

		if (lhs->getType()->isIntegerTy() && rhs->getType()->isDoubleTy())
		{
			// convert lhs to double
			lhs = builder->CreateSIToFP(lhs, builder->getDoubleTy(), "cast_to_dbl");
			lt = LITERAL_TYPE_DOUBLE;
		}
		else if (rhs->getType()->isIntegerTy() && lhs->getType()->isDoubleTy())
		{
			// convert rhs to double
			rhs = builder->CreateSIToFP(rhs, builder->getDoubleTy(), "cast_to_int");
			lt = LITERAL_TYPE_DOUBLE;
		}

		switch (m_token->GetType())
		{
		case TOKEN_MINUS:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFSub(lhs, rhs, "subtmp");
			if (LITERAL_TYPE_INTEGER == lt) return builder->CreateSub(lhs, rhs, "subtmp");
			break;

		case TOKEN_PLUS:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFAdd(lhs, rhs, "addtmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateAdd(lhs, rhs, "addtmp");
			if (lhs->getType()->isPointerTy())
			{
				AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(1024), "alloctmp");
				Value* b = builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp");
				builder->CreateStore(builder->getInt8(0), b);
				builder->CreateCall(module->getFunction("strcat"), { b, lhs }, "calltmp");
				builder->CreateCall(module->getFunction("strcat"), { b, rhs }, "calltmp");
				return b;
			}
			break;

		case TOKEN_STAR:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFMul(lhs, rhs, "multmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateMul(lhs, rhs, "multmp");
			break;

		case TOKEN_SLASH:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFDiv(lhs, rhs, "divtmp");
			break;

		case TOKEN_GREATER:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFCmpUGT(lhs, rhs, "cmptmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateICmpSGT(lhs, rhs, "cmptmp");
			break;

		case TOKEN_GREATER_EQUAL:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFCmpUGE(lhs, rhs, "cmptmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateICmpSGE(lhs, rhs, "cmptmp");
			break;

		case TOKEN_LESS:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFCmpULT(lhs, rhs, "cmptmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateICmpSLT(lhs, rhs, "cmptmp");
			break;

		case TOKEN_LESS_EQUAL:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFCmpULE(lhs, rhs, "cmptmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateICmpSLE(lhs, rhs, "cmptmp");
			break;

		case TOKEN_BANG_EQUAL:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFCmpUNE(lhs, rhs, "cmptmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateICmpNE(lhs, rhs, "cmptmp");
			if (lhs->getType()->isPointerTy())
			{
				std::vector<Value*> args;
				args.push_back(lhs);
				args.push_back(rhs);
				Value* v = builder->CreateCall(module->getFunction("strcmp"), args, "calltmp");
				return builder->CreateICmpNE(v, Constant::getNullValue(builder->getInt32Ty()), "boolcmp");
			}
			break;

		case TOKEN_EQUAL_EQUAL:
			if (lhs->getType()->isDoubleTy()) return builder->CreateFCmpUEQ(lhs, rhs, "cmptmp");
			if (lhs->getType()->isIntegerTy()) return builder->CreateICmpEQ(lhs, rhs, "cmptmp");
			if (lhs->getType()->isPointerTy())
			{
				std::vector<Value*> args;
				args.push_back(lhs);
				args.push_back(rhs);
				Value* v = builder->CreateCall(module->getFunction("strcmp"), args, "calltmp");
				return builder->CreateICmpEQ(v, Constant::getNullValue(builder->getInt32Ty()), "boolcmp");
			}
			break;
		}

		return nullptr;
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

	LiteralTypeEnum GetLiteralType(Environment* env)
	{
		if (!m_callee || EXPRESSION_VARIABLE != m_callee->GetType()) return LITERAL_TYPE_INVALID;
		std::string name = (static_cast<VariableExpr*>(m_callee))->Operator()->Lexeme();
		return env->GetFunctionReturnType(name);
	}

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("CallExpr::codegen()\n");
		if (!m_callee) return nullptr;
		if (EXPRESSION_VARIABLE != m_callee->GetType()) return nullptr;

		std::string name = (static_cast<VariableExpr*>(m_callee))->Operator()->Lexeme();
		Function* callee = env->GetFunction(name);

		if (0 == name.compare("input"))
		{
			AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(256), "alloctmp");
			Value* b = builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp");
			builder->CreateStore(builder->getInt8(0), b);
			builder->CreateCall(module->getFunction("input"), { b }, "calltmp");
			return b;
		}
		else
		{
			return builder->CreateCall(callee, {}, "calltmp");
		}

		/*LiteralTypeEnum lt = this->GetLiteralType(env);
		if (LITERAL_TYPE_INVALID != typeHint && typeHint != lt)
		{
			// return type conversion
			if (LITERAL_TYPE_DOUBLE == typeHint)
			{
				return builder->CreateSIToFP(v, builder->getDoubleTy(), "dbl_cast_tmp");
			}
			else if (LITERAL_TYPE_INTEGER == typeHint)
			{
				return builder->CreateFPToSI(v, builder->getInt32Ty(), "int_cast_tmp");
			}
		}*/

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
	LiteralTypeEnum GetLiteralType(Environment* env) { return m_expression->GetLiteralType(env); }

	Expr* Expression() { return m_expression; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("GroupExpr::codegen()\n");
		return m_expression->codegen(context, builder, module, env, typeHint);
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
	LiteralTypeEnum GetLiteralType(Environment* env) { return m_literal.GetType(); }

	Literal GetLiteral() { return m_literal; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("LiteralExpr::codegen()\n");

		LiteralTypeEnum lt = m_literal.GetType();
		if (LITERAL_TYPE_INVALID != typeHint) lt = typeHint;

		if (LITERAL_TYPE_INTEGER == lt)
			return ConstantInt::get(*context, APInt(32, m_literal.IntValue(), true));
		else if (LITERAL_TYPE_BOOL == lt)
			return ConstantInt::get(*context, APInt(32, int(m_literal.BoolValue()), true));
		else if (LITERAL_TYPE_DOUBLE == lt)
			return ConstantFP::get(*context, APFloat(m_literal.DoubleValue()));
		else if (LITERAL_TYPE_STRING == lt)
			return builder->CreateGlobalStringPtr(m_literal.ToString(), "strtmp");

		return nullptr;
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
	LiteralTypeEnum GetLiteralType(Environment* env) { return LITERAL_TYPE_BOOL; }

	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }
	
	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("LogicalExpr::codegen()\n");

		if (!m_left || !m_right) return nullptr;
		Value* lhs = m_left->codegen(context, builder, module, env);
		Value* rhs = m_right->codegen(context, builder, module, env);
		if (!lhs || !rhs) return nullptr;

		switch (m_token->GetType())
		{
		case TOKEN_AND:
			return builder->CreateAnd(lhs, rhs, "logical_and_cmp");
			break;
		
		case TOKEN_OR:
			return builder->CreateOr(lhs, rhs, "logical_or_cmp");
			break;
		}

		return nullptr;
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
	LiteralTypeEnum GetLiteralType(Environment* env) { return LITERAL_TYPE_INTEGER; }
	
	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("RangeExpr::codegen()\n");

		if (!m_left || !m_right) return nullptr;
		Value* lhs = m_left->codegen(context, builder, module, env);
		Value* rhs = m_right->codegen(context, builder, module, env);
		if (!lhs || !rhs) return nullptr;

		printf("test\n");

		return nullptr;
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
	LiteralTypeEnum GetLiteralType(Environment* env) { return m_right->GetLiteralType(env); }

	Token* Operator() { return m_token; }
	Expr* Right() { return m_right; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("UnaryExpr::codegen()\n");
		
		Value* rhs = m_right->codegen(context, builder, module, env, typeHint);
		
		LiteralTypeEnum lt = m_right->GetLiteralType(env);
		if (LITERAL_TYPE_INVALID != typeHint) lt = typeHint;

		switch (m_token->GetType())
		{
		case TOKEN_MINUS:
			if (LITERAL_TYPE_INTEGER == lt)
				return builder->CreateNeg(rhs, "negtmp");
			else if (LITERAL_TYPE_DOUBLE == lt)
				return builder->CreateFNeg(rhs, "negtmp");
			break;

		case TOKEN_BANG:
			if (LITERAL_TYPE_INTEGER == lt || LITERAL_TYPE_BOOL == lt)
			{
				printf("IfStmt::codegen()\n");
				rhs = builder->CreateIntCast(rhs, builder->getInt1Ty(), true, "bool_cast");
				return builder->CreateICmpEQ(rhs, Constant::getNullValue(builder->getInt1Ty()), "zero_check");
			}
			break;
		}

		
		return nullptr;
	}

private:
	Token* m_token;
	Expr* m_right;
};





#endif // EXPRESSIONS_H