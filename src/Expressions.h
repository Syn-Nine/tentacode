#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include <string>
#include <cstdarg>

#include "Token.h"
#include "Literal.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

class Expr
{
public:
	virtual ExpressionTypeEnum GetType() = 0;
	virtual Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID) = 0;
	virtual LiteralTypeEnum GetLiteralType() = 0;
};

typedef std::vector<Expr*> ArgList;


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
	LiteralTypeEnum GetLiteralType()
	{
		LiteralTypeEnum ltype = m_left->GetLiteralType();
		LiteralTypeEnum rtype = m_right->GetLiteralType();

		switch (m_token->GetType())
		{
		case TOKEN_MINUS: // intentional fall-through
		case TOKEN_PLUS:
		case TOKEN_STAR:
			if (ltype == rtype) return ltype;
			if ((LITERAL_TYPE_INTEGER == ltype && LITERAL_TYPE_DOUBLE == rtype) ||
				(LITERAL_TYPE_DOUBLE == ltype && LITERAL_TYPE_INTEGER == rtype))
				return LITERAL_TYPE_DOUBLE;
			
			break;
		
		case TOKEN_SLASH:
			return LITERAL_TYPE_DOUBLE;
			break;
		}
		
		return LITERAL_TYPE_INVALID;
	}

	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("BinaryExpr::codegen()\n");

		LiteralTypeEnum lt = this->GetLiteralType();
		if (LITERAL_TYPE_INVALID != typeHint) lt = typeHint;

		Value* lhs = m_left->codegen(context, builder, lt);
		Value* rhs = m_right->codegen(context, builder, lt);
		if (!lhs || !rhs) return nullptr;

		switch (m_token->GetType())
		{
		case TOKEN_MINUS:
			if (LITERAL_TYPE_DOUBLE == lt) return builder->CreateFSub(lhs, rhs, "subtmp");
			if (LITERAL_TYPE_INTEGER == lt) return builder->CreateSub(lhs, rhs, "subtmp");
			break;
		case TOKEN_PLUS:
			if (LITERAL_TYPE_DOUBLE == lt) return builder->CreateFAdd(lhs, rhs, "addtmp");
			if (LITERAL_TYPE_INTEGER == lt) return builder->CreateAdd(lhs, rhs, "addtmp");
			break;
		case TOKEN_STAR:
			if (LITERAL_TYPE_DOUBLE == lt) return builder->CreateFMul(lhs, rhs, "multmp");
			if (LITERAL_TYPE_INTEGER == lt) return builder->CreateMul(lhs, rhs, "multmp");
			break;
		case TOKEN_SLASH:
			if (LITERAL_TYPE_DOUBLE == lt) return builder->CreateFDiv(lhs, rhs, "divtmp");
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
class GroupExpr : public Expr
{
public:
	GroupExpr() = delete;
	GroupExpr(Expr* expression)
	{
		m_expression = expression;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_GROUP; }
	LiteralTypeEnum GetLiteralType() { return m_expression->GetLiteralType(); }

	Expr* Expression() { return m_expression; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("GroupExpr::codegen()\n");
		return m_expression->codegen(context, builder, typeHint);
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
	LiteralTypeEnum GetLiteralType() { return m_literal.GetType(); }

	Literal GetLiteral() { return m_literal; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("LiteralExpr::codegen()\n");

		LiteralTypeEnum lt = m_literal.GetType();
		if (LITERAL_TYPE_INVALID != typeHint) lt = typeHint;

		if (LITERAL_TYPE_INTEGER == lt)
			return ConstantInt::get(*context, APInt(32, m_literal.IntValue(), true));
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
	LiteralTypeEnum GetLiteralType() { return LITERAL_TYPE_BOOL; }

	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("LogicalExpr::codegen()\n");
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
	LiteralTypeEnum GetLiteralType() { return m_right->GetLiteralType(); }

	Token* Operator() { return m_token; }
	Expr* Right() { return m_right; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		LiteralTypeEnum typeHint = LITERAL_TYPE_INVALID)
	{
		printf("UnaryExpr::codegen()\n");
		
		Value* rhs = m_right->codegen(context, builder, typeHint);
		
		LiteralTypeEnum lt = m_right->GetLiteralType();
		if (LITERAL_TYPE_INVALID != typeHint) lt = typeHint;

		if (LITERAL_TYPE_INTEGER == lt)
			return builder->CreateNeg(rhs, "negtmp");
		else if (LITERAL_TYPE_DOUBLE == lt)
			return builder->CreateFNeg(rhs, "negtmp");
	
		return nullptr;
	}

private:
	Token* m_token;
	Expr* m_right;
};

#endif // EXPRESSIONS_H