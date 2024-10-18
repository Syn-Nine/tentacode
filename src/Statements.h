#ifndef STATEMENTS_H
#define STATEMENTS_H

#include <assert.h>

#include "Enums.h"
#include "Token.h"
#include "Expressions.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/SmallVector.h"

class FunctionStmt;

using namespace llvm;

class Stmt
{
public:
	virtual StatementTypeEnum GetType() = 0;
	virtual Expr* Expression() { return nullptr; }
	virtual Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module) = 0;
};

typedef std::vector<Stmt*> StmtList;
typedef std::vector<FunctionStmt*> FunList;


//-----------------------------------------------------------------------------
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

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module)
	{
		printf("ExpressionStmt::codegen()\n");
		return m_expr->codegen(context, builder);
	}

private:
	Expr* m_expr;
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

	Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_PRINT; }

	Value* codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module)
	{
		printf("ExpressionStmt::PrintStmt()\n");

		LiteralTypeEnum lt = m_expr->GetLiteralType();
		
		std::vector<Value*> args;
		
		Value* v = m_expr->codegen(context, builder);
		if (LITERAL_TYPE_STRING != lt)
		{
			SmallVector<char, 32> s;
			if (LITERAL_TYPE_DOUBLE == lt) dyn_cast<ConstantFP>(v)->getValue().toString(s, 8);
			if (LITERAL_TYPE_INTEGER == lt) dyn_cast<ConstantInt>(v)->getValue().toStringSigned(s);
			
			v = builder->CreateGlobalStringPtr(StringRef(s.data(), s.size()), "strtmp");
		}
		args.push_back(v);
		
		if (m_newline)
			return builder->CreateCall(module->getFunction("println"), args, "calltmp");
		else
			return builder->CreateCall(module->getFunction("print"), args, "calltmp");
	}

private:
	Expr* m_expr;
	bool m_newline;
};



#endif // STATEMENTS_H