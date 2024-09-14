#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include <string>
#include <cstdarg>

#include "Token.h"
#include "Literal.h"

class Expr
{
public:
	virtual ExpressionTypeEnum GetType() = 0;
};

typedef std::vector<Expr*> ArgList;

class AssignExpr : public Expr
{
public:
	AssignExpr() = delete;
	AssignExpr(Token* name, Expr* right, Expr* vecIndex, std::string fqns)
	{
		m_token = name;
		m_right = right;
		m_vecIndex = vecIndex;
		m_fqns = fqns;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_ASSIGN; }

	Token* Operator() { return m_token; }
	Expr* Right() { return m_right; }
	Expr* VecIndex() { return m_vecIndex; }
	std::string	FQNS() { return m_fqns; }

private:
	Token* m_token;
	Expr* m_right;
	Expr* m_vecIndex;
	std::string m_fqns;
};


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

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};


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

private:
	Expr* m_callee;
	Token* m_token;
	ArgList m_arguments;
};


class FormatExpr : public Expr
{
public:
	FormatExpr() = delete;
	FormatExpr(Token* paren, ArgList arguments, std::string fqns)
	{
		m_token = paren;
		m_arguments = arguments;
		m_fqns = fqns;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_FORMAT; }

	Token* Operator() { return m_token; }
	ArgList GetArguments() { return m_arguments; }
	std::string FQNS() { return m_fqns; }

private:
	Token* m_token;
	ArgList m_arguments;
	std::string m_fqns;
};


class FunctorExpr : public Expr
{
public:
	FunctorExpr() = delete;
	FunctorExpr(Token* token, TokenList params, void* block, std::string fqns)
	{
		m_token = token;
		m_params = params;
		m_body = block;
		m_fqns = fqns;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_FUNCTOR; }

	Token* Operator() { return m_token; }
	TokenList GetParams() { return m_params; }
	void* GetBody() { return m_body; }
	std::string FQNS() { return m_fqns; }

private:
	Token* m_token;
	TokenList m_params;
	void* m_body;
	std::string m_fqns;
};

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

private:
	Expr* m_expression;
};


class BracketExpr : public Expr
{
public:
	BracketExpr() = delete;
	BracketExpr(Token* bracket, ArgList arguments)
	{
		m_token = bracket;
		m_arguments = arguments;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_BRACKET; }

	Token* Operator() { return m_token; }
	ArgList GetArguments() { return m_arguments; }

private:
	Token* m_token;
	ArgList m_arguments;
};


class LiteralExpr : public Expr
{
public:
	LiteralExpr() = delete; // null not allowed
	LiteralExpr(int32_t val) : m_literal(val) {}
	LiteralExpr(int32_t lval, int32_t rval) : m_literal(lval, rval) {}
	LiteralExpr(double val) : m_literal(val) {}
	LiteralExpr(std::string val) : m_literal(val) {}
	LiteralExpr(EnumLiteral val) : m_literal(val) {}
	LiteralExpr(bool val) : m_literal(val) {}

	std::string ToString() { return m_literal.ToString(); }

	ExpressionTypeEnum GetType() { return EXPRESSION_LITERAL; }

	Literal GetLiteral() { return m_literal; }

private:
	Literal m_literal;
};


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

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};


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

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};


class ReplicateExpr : public Expr
{
public:
	ReplicateExpr() = delete;
	ReplicateExpr(Expr* left, Token* name, Expr* right)
	{
		m_left = left;
		m_token = name;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_REPLICATE; }

	Token* Operator() { return m_token; }
	Expr* Left() { return m_left; }
	Expr* Right() { return m_right; }

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};


class DestructExpr : public Expr
{
public:
	DestructExpr() = delete;
	DestructExpr(ArgList lhs, ArgList rhs, Token* token)
	{
		m_lhs = lhs;
		m_rhs = rhs;
		m_token = token;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_DESTRUCTURE; }

	Token* Operator() { return m_token; }
	ArgList GetLhsArguments() { return m_lhs; }
	ArgList GetRhsArguments() { return m_rhs; }

private:
	Token* m_token;
	ArgList m_lhs;
	ArgList m_rhs;
};


class StructExpr : public Expr
{
public:
	StructExpr() = delete;
	StructExpr(ArgList arguments, Token* token)
	{
		m_arguments = arguments;
		m_token = token;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_STRUCTURE; }

	Token* Operator() { return m_token; }
	ArgList GetArguments() { return m_arguments; }

private:
	Token* m_token;
	ArgList m_arguments;
};


class GetExpr : public Expr
{
public:
	GetExpr() = delete;
	GetExpr(Expr* object, Token* name, Expr* right)
	{
		m_token = name;
		m_object = object;
		m_right = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_GET; }

	Token* Name() { return m_token; }
	Expr* Object() { return m_object; }
	Expr* VecIndex() { return m_right; }

private:
	Token* m_token;
	Expr* m_object;
	Expr* m_right;
};


class SetExpr : public Expr
{
public:
	SetExpr() = delete;
	SetExpr(Expr* object, Token* name, Expr* value, Expr* right, std::string fqns)
	{
		m_token = name;
		m_object = object;
		m_value = value;
		m_right = right;
		m_fqns = fqns;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_SET; }
	
	Token* Name() { return m_token; }
	Expr* Object() { return m_object; }
	Expr* Value() { return m_value; }
	Expr* VecIndex() { return m_right; }
	std::string FQNS() { return m_fqns; }

private:
	Token* m_token;
	Expr* m_object;
	Expr* m_value;
	Expr* m_right;
	std::string m_fqns;
};


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

private:
	Token* m_token;
	Expr* m_right;
};

class VariableExpr : public Expr
{
public:
	VariableExpr() = delete;
	VariableExpr(Token* name, Expr* right, std::string fqns)
	{
		m_token = name;
		m_right = right;
		m_fqns = fqns;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_VARIABLE; }

	Token* Operator() { return m_token; }
	Expr* VecIndex() { return m_right; }
	std::string FQNS() { return m_fqns; }

private:
	Token* m_token;
	Expr* m_right;
	std::string m_fqns;
};


#endif // EXPRESSIONS_H