#ifndef STATEMENTS_H
#define STATEMENTS_H

#include <assert.h>

#include "TValue.h"
#include "Enums.h"
#include "Token.h"
#include "Expressions.h"
#include "Environment.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
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
	virtual TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
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
	
	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

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

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

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
	
	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

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

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

private:
	Expr* m_expr;
};



//-----------------------------------------------------------------------------
class FunctionStmt : public Stmt
{
public:
	FunctionStmt() = delete;

	FunctionStmt(Token* rettype, Token* name, TokenList types, TokenList params, StmtList* body)
	{
		m_rettype = rettype;
		m_name = name;
		m_types = types;
		m_params = params;
		m_body = body;
	}

	StatementTypeEnum GetType() { return STATEMENT_FUNCTION; }

	/*Token* Operator() { return m_name; }
	Token* RetType() { return m_rettype; }
	TokenList GetParams() { return m_params; }
	StmtList* GetBody() { return m_body; }*/

	TValue codegen_prototype(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

private:
	Token* m_name;
	Token* m_rettype;
	TokenList m_types;
	TokenList m_params;
	StmtList* m_body;
};



//-----------------------------------------------------------------------------
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

	/*Expr* GetCondition() { return m_condition; }
	Stmt* GetThenBranch() { return m_thenBranch; }
	Stmt* GetElseBranch() { return m_elseBranch; }*/
	
	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

private:
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

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

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

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

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

	Token* Operator() { return m_name; }
	StmtList* GetVars() { return m_vars; }
	
	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);
		

private:
	Token* m_name;
	StmtList* m_vars;
};



//-----------------------------------------------------------------------------
class VarStmt : public Stmt
{
public:
	VarStmt() = delete;

	VarStmt(Token* type, Token* name, Expr* expr, LiteralTypeEnum vtype, std::string vtypeid, bool global = false)
	{
		m_type = type;
		m_token = name;
		m_expr = expr;
		m_vecType = vtype;
		m_global = global;
		m_vecTypeId = vtypeid;
	}

	Expr* Expression() { return m_expr; }
	Token* Operator() { return m_token; }
	Token* VarType() { return m_type; }
	LiteralTypeEnum VarVecType() { return m_vecType; }
	std::string VarVecTypeId() { return m_vecTypeId; }

	StatementTypeEnum GetType() { return STATEMENT_VAR; }

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

private:
	Token* m_type;
	LiteralTypeEnum m_vecType;
	std::string m_vecTypeId;
	Token* m_token;
	Expr* m_expr;
	bool m_global;
};



//-----------------------------------------------------------------------------
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

	/*Expr* GetCondition() { return m_condition; }
	Expr* GetPost() { return m_post; }
	Stmt* GetBody() { return m_body; }*/

	TValue codegen(std::unique_ptr<llvm::LLVMContext>& context,
		std::unique_ptr<llvm::IRBuilder<>>& builder,
		std::unique_ptr<llvm::Module>& module,
		Environment* env);

private:
	Expr* m_condition;
	Expr* m_post;
	Stmt* m_body;
	std::string m_label;
};

#endif // STATEMENTS_H