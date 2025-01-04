#ifndef STATEMENTS_H
#define STATEMENTS_H

#include <assert.h>

#include "TValue.h"
#include "Enums.h"
#include "Token.h"
#include "Expressions.h"
#include "Environment.h"
#include "Utility.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/DerivedTypes.h"

class FunctionStmt;


class Stmt
{
public:
	virtual StatementTypeEnum GetType() = 0;
	virtual Expr* Expression() { return nullptr; }
	virtual TValue codegen(
		llvm::IRBuilder<>* builder,
		llvm::Module* module,
		Environment* env) = 0;
};

typedef std::vector<Stmt*> StmtList;
typedef std::vector<FunctionStmt*> FunList;



//-----------------------------------------------------------------------------
class BlockStmt : public Stmt
{
public:
	BlockStmt() = delete;

	BlockStmt(StmtList* block)
	{
		m_block = block;
	}

	//StmtList* GetBlock() { return m_block; }

	StatementTypeEnum GetType() { return STATEMENT_BLOCK; }
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	StmtList* m_block;
};



//-----------------------------------------------------------------------------
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

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_keyword;
};



//-----------------------------------------------------------------------------
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
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_keyword;
};



//-----------------------------------------------------------------------------
class ExpressionStmt : public Stmt
{
public:
	ExpressionStmt() = delete;

	ExpressionStmt(Expr* expr)
	{
		m_expr = expr;
	}

	//Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_EXPRESSION; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Expr* m_expr;
};



//-----------------------------------------------------------------------------
class ForEachStmt : public Stmt
{
public:
	ForEachStmt() = delete;

	ForEachStmt(Token* key, Token* val, Expr* expr, Stmt* body)
	{
		m_key = key;
		m_val = val;
		m_expr = expr;
		m_body = body;
	}

	StatementTypeEnum GetType() { return STATEMENT_FOREACH; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_key;
	Token* m_val;
	Expr* m_expr;
	Stmt* m_body;
};



//-----------------------------------------------------------------------------
class FunctionStmt : public Stmt
{
public:
	FunctionStmt() = delete;

	FunctionStmt(Token* rettype, Token* name, TokenList types, TokenList params, std::vector<bool> mut, StmtList* body)
	{
		m_rettype = rettype;
		m_name = name;
		m_types = types;
		m_params = params;
		m_mutable = mut;
		m_body = body;
	}

	StatementTypeEnum GetType() { return STATEMENT_FUNCTION; }

	/*Token* Operator() { return m_name; }
	Token* RetType() { return m_rettype; }
	TokenList GetParams() { return m_params; }
	StmtList* GetBody() { return m_body; }*/

	TValue codegen_prototype(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_name;
	Token* m_rettype;
	TokenList m_types;
	TokenList m_params;
	StmtList* m_body;
	std::vector<bool> m_mutable;
};



//-----------------------------------------------------------------------------
class IfStmt : public Stmt
{
public:
	IfStmt() = delete;

	IfStmt(Token* token, Expr* condition, Stmt* thenBranch, Stmt* elseBranch)
	{
		m_token = token;
		m_condition = condition;
		m_thenBranch = thenBranch;
		m_elseBranch = elseBranch;
	}

	StatementTypeEnum GetType() { return STATEMENT_IF; }

	Token* Operator() { return m_token; }
	/*Expr* GetCondition() { return m_condition; }
	Stmt* GetThenBranch() { return m_thenBranch; }
	Stmt* GetElseBranch() { return m_elseBranch; }*/
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_condition;
	Stmt* m_thenBranch;
	Stmt* m_elseBranch;
};



//-----------------------------------------------------------------------------
class PrintStmt : public Stmt
{
public:
	PrintStmt() = delete;

	PrintStmt(Expr* expr, bool newline)
	{
		m_expr = expr;
		m_newline = newline;
	}

	//Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_PRINT; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Expr* m_expr;
	bool m_newline;
};



//-----------------------------------------------------------------------------
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

	// Expr* GetValueExpr() { return m_value; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_keyword;
	Expr* m_value;
};



//-----------------------------------------------------------------------------
class StructStmt : public Stmt
{
public:
	StructStmt() = delete;

	StructStmt(Token* name, StmtList* vars)
	{
		m_name = name;
		m_vars = vars;
	}

	StatementTypeEnum GetType() { return STATEMENT_STRUCT; }

	//Token* Operator() { return m_name; }
	//StmtList* GetVars() { return m_vars; }
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);
		

private:
	Token* m_name;
	StmtList* m_vars;
};



//-----------------------------------------------------------------------------
class VarStmt : public Stmt
{
public:
	VarStmt() = delete;

	VarStmt(TypeToken type, TokenPtrList ids, ArgList exprs)
	{
		m_type_token = type;
		m_ids = ids;
		m_exprs = exprs;
	}

	TokenPtrList IDs() { return m_ids; }
	ArgList Expressions() { return m_exprs; }

	Token* VarType() { return m_type_token.type; }
	TokenPtrList* TypeArgs() { return m_type_token.args; }
	
	StatementTypeEnum GetType() { return STATEMENT_VAR; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	TypeToken m_type_token;
	TokenPtrList m_ids;
	ArgList m_exprs;
};


//-----------------------------------------------------------------------------
class VarConstStmt : public Stmt
{
public:
	VarConstStmt() = delete;

	VarConstStmt(Token* type, TokenPtrList ids, ArgList exprs)
	{
		m_type = type;
		m_ids = ids;
		m_exprs = exprs;
	}

	//Token* Id() { return m_id; }

	//Expr* Expression() { return m_expr; }
	Token* VarType() { return m_type; }

	/*LiteralTypeEnum VarVecType() { return m_vecType; }
	std::string VarVecTypeId() { return m_vecTypeId; }*/

	StatementTypeEnum GetType() { return STATEMENT_VAR_CONST; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_type;
	TokenPtrList m_ids;
	ArgList m_exprs;
};



//-----------------------------------------------------------------------------
class WhileStmt : public Stmt
{
public:
	WhileStmt() = delete;

	WhileStmt(Expr* condition, Stmt* body, Expr* post)
	{
		m_condition = condition;
		m_body = body;
		m_post = post;
	}

	StatementTypeEnum GetType() { return STATEMENT_WHILE; }

	/*Expr* GetCondition() { return m_condition; }
	Expr* GetPost() { return m_post; }
	Stmt* GetBody() { return m_body; }*/

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Expr* m_condition;
	Expr* m_post;
	Stmt* m_body;
};

#endif // STATEMENTS_H