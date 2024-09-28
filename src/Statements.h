#ifndef STATEMENTS_H
#define STATEMENTS_H

#include <assert.h>

#include "Enums.h"
#include "Token.h"
#include "Expressions.h"

class FunctionStmt;

class Stmt
{
public:
	virtual StatementTypeEnum GetType() = 0;
	virtual Expr* Expression() { return nullptr; }
};

typedef std::vector<Stmt*> StmtList;
typedef std::vector<FunctionStmt*> FunList;

class BlockStmt : public Stmt
{
public:
	BlockStmt() = delete;

	BlockStmt(StmtList* block)
	{
		m_block = block;
	}

	StmtList* GetBlock() { return m_block; }

	StatementTypeEnum GetType() { return STATEMENT_BLOCK; }

private:
	StmtList* m_block;
};


class ExpressionStmt : public Stmt
{
public:
	ExpressionStmt() = delete;

	ExpressionStmt(Expr* expr)
	{
		m_expr = expr;
	}

	Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_EXPRESSION; }

private:
	Expr* m_expr;
};


class FunctionStmt : public Stmt
{
public:
	FunctionStmt() = delete;

	FunctionStmt(Token* name, TokenList params, StmtList* body, std::string fqns, bool internal = true)
	{
		m_name = name;
		m_params = params;
		m_body = body;
		m_fqns = fqns;
		m_internal = internal;
	}

	StatementTypeEnum GetType() { return STATEMENT_FUNCTION; }

	Token* Operator() { return m_name; }
	TokenList GetParams() { return m_params; }
	StmtList* GetBody() { return m_body; }
	std::string FQNS() { return m_fqns; }
	bool Internal() { return m_internal; }

private:
	Token* m_name;
	TokenList m_params;
	StmtList* m_body;
	std::string m_fqns;
	bool m_internal;
};


class IfStmt : public Stmt
{
public:
	IfStmt() = delete;

	IfStmt(Expr* condition, Stmt* thenBranch, Stmt* elseBranch)
	{
		m_condition = condition;
		m_thenBranch = thenBranch;
		m_elseBranch = elseBranch;
	}

	StatementTypeEnum GetType() { return STATEMENT_IF; }

	Expr* GetCondition() { return m_condition; }
	Stmt* GetThenBranch() { return m_thenBranch; }
	Stmt* GetElseBranch() { return m_elseBranch; }

private:
	Expr* m_condition;
	Stmt* m_thenBranch;
	Stmt* m_elseBranch;
};


class ClearEnvStmt : public Stmt
{
public:
	StatementTypeEnum GetType() { return STATEMENT_CLEARENV; }
};


class PrintStmt : public Stmt
{
public:
	PrintStmt() = delete;

	PrintStmt(Expr* expr)
	{
		m_expr = expr;
	}

	Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_PRINT; }

private:
	Expr* m_expr;
};

class PrintLnStmt : public Stmt
{
public:
	PrintLnStmt() = delete;

	PrintLnStmt(Expr* expr)
	{
		m_expr = expr;
	}

	Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_PRINTLN; }

private:
	Expr* m_expr;
};


class BreakStmt : public Stmt
{
public:
	BreakStmt() = delete;

	BreakStmt(Token* keyword)
	{
		m_keyword = keyword;
	}

	StatementTypeEnum GetType() { return STATEMENT_BREAK; }

	//Expr* GetValueExpr() { return m_value; }

private:
	Token* m_keyword;
};


class ContinueStmt : public Stmt
{
public:
	ContinueStmt() = delete;

	ContinueStmt(Token* keyword)
	{
		m_keyword = keyword;
	}

	StatementTypeEnum GetType() { return STATEMENT_CONTINUE; }

	//Expr* GetValueExpr() { return m_value; }

private:
	Token* m_keyword;
};


class DestructStmt : public Stmt
{
public:
	DestructStmt() = delete;

	DestructStmt(std::vector<Token*> types, std::vector<Token*> names, Expr* expr, std::vector<LiteralTypeEnum> vtypes, std::vector<LiteralTypeEnum> mapKeyTypes, std::vector<LiteralTypeEnum> mapValueTypes, std::string fqns, bool internal = true)
	{
		assert(types.size() == names.size());
		assert(types.size() == vtypes.size());
		m_types = types;
		m_tokens = names;
		m_expr = expr;
		m_vecTypes = vtypes;
		m_mapKeyTypes = mapKeyTypes;
		m_mapValueTypes = mapValueTypes;
		m_fqns = fqns;
		m_internal = internal;
	}

	Expr* Expression() { return m_expr; }
	const std::vector<Token*>& Operators() { return m_tokens; }
	const std::vector<Token*>& VarTypes() { return m_types; }
	const std::vector<LiteralTypeEnum>& VarVecTypes() { return m_vecTypes; }
	const std::vector<LiteralTypeEnum>& MapKeyTypes() { return m_mapKeyTypes; }
	const std::vector<LiteralTypeEnum>& MapValueTypes() { return m_mapValueTypes; }
	std::string FQNS() { return m_fqns; }
	bool Internal() { return m_internal; }

	StatementTypeEnum GetType() { return STATEMENT_DESTRUCT; }

private:
	std::vector<Token*> m_types;
	std::vector<Token*> m_tokens;
	Expr* m_expr;
	std::vector<LiteralTypeEnum> m_vecTypes;
	std::vector<LiteralTypeEnum> m_mapKeyTypes;
	std::vector<LiteralTypeEnum> m_mapValueTypes;
	std::string m_fqns;
	bool m_internal;
};


class ReturnStmt : public Stmt
{
public:
	ReturnStmt() = delete;

	ReturnStmt(Token* keyword, Expr* value)
	{
		m_keyword = keyword;
		m_value = value;
	}

	StatementTypeEnum GetType() { return STATEMENT_RETURN; }

	Expr* GetValueExpr() { return m_value; }

private:
	Token* m_keyword;
	Expr* m_value;
};


class StructStmt : public Stmt
{
public:
	StructStmt() = delete;

	StructStmt(Token* name, StmtList* vars, std::string fqns, bool internal = true)
	{
		m_name = name;
		m_vars = vars;
		m_fqns = fqns;
		m_internal = internal;
	}

	StatementTypeEnum GetType() { return STATEMENT_STRUCT; }

	Token* Operator() { return m_name; }
	StmtList* GetVars() { return m_vars; }
	std::string FQNS() { return m_fqns; }
	bool Internal() { return m_internal; }

private:
	Token* m_name;
	StmtList* m_vars;
	std::string m_fqns;
	bool m_internal;
};



class VarStmt : public Stmt
{
public:
	VarStmt() = delete;

	VarStmt(Token* type, Token* name, Expr* expr, LiteralTypeEnum vtype, LiteralTypeEnum mapKeyType, LiteralTypeEnum mapValueType, std::string fqns, bool internal = true)
	{
		m_type = type;
		m_token = name;
		m_expr = expr;
		m_vecType = vtype;
		m_mapKeyType = mapKeyType;
		m_mapValueType = mapValueType;
		m_fqns = fqns;
		m_internal = internal;
	}

	Expr* Expression() { return m_expr; }
	Token* Operator() { return m_token; }
	Token* VarType() { return m_type; }
	LiteralTypeEnum VarVecType() { return m_vecType; }
	LiteralTypeEnum MapKeyType() { return m_mapKeyType; }
	LiteralTypeEnum MapValueType() { return m_mapValueType; }
	std::string FQNS() { return m_fqns; }
	bool Internal() { return m_internal; }

	StatementTypeEnum GetType() { return STATEMENT_VAR; }

private:
	Token* m_type;
	Token* m_token;
	Expr* m_expr;
	LiteralTypeEnum m_vecType;
	LiteralTypeEnum m_mapKeyType;
	LiteralTypeEnum m_mapValueType;
	std::string m_fqns;
	bool m_internal;
};


class WhileStmt : public Stmt
{
public:
	WhileStmt() = delete;

	WhileStmt(Expr* condition, Stmt* body, Expr* post, std::string label)
	{
		m_condition = condition;
		m_body = body;
		m_post = post;
		m_label = label;
	}

	StatementTypeEnum GetType() { return STATEMENT_WHILE; }

	Expr* GetCondition() { return m_condition; }
	Expr* GetPost() { return m_post; }
	Stmt* GetBody() { return m_body; }
	std::string GetLabel() { return m_label; }

private:
	Expr* m_condition;
	Expr* m_post;
	Stmt* m_body;
	std::string m_label;
};

#endif // STATEMENTS_H