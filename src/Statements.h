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

class FunctionStmt;

using namespace llvm;

class Stmt
{
public:
	virtual StatementTypeEnum GetType() = 0;
	virtual Expr* Expression() { return nullptr; }
	virtual TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
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

	StmtList* GetBlock() { return m_block; }

	StatementTypeEnum GetType() { return STATEMENT_BLOCK; }
	
	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("BlockStmt::codegen()\n");

		Function* ftn = builder->GetInsertBlock()->getParent();
		BasicBlock* entry = BasicBlock::Create(*context, "bbtmp", ftn);
		BasicBlock* after = BasicBlock::Create(*context, "aftertmp");
		
		builder->CreateBr(entry);
		builder->SetInsertPoint(entry);

		Environment* sub_env = new Environment(env);
		
		for (auto& statement : *m_block)
		{
			if (STATEMENT_FUNCTION != statement->GetType())
			{
				statement->codegen(context, builder, module, sub_env);
			}
		}

		delete sub_env;

		ftn->insert(ftn->end(), after);

		builder->CreateBr(after);
		builder->SetInsertPoint(after);

		return TValue::NullInvalid();
	}

private:
	StmtList* m_block;
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

	Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_EXPRESSION; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("ExpressionStmt::codegen()\n");
		return m_expr->codegen(context, builder, module, env);
	}

private:
	Expr* m_expr;
};



//-----------------------------------------------------------------------------
class FunctionStmt : public Stmt
{
public:
	FunctionStmt() = delete;

	FunctionStmt(Token* rettype, Token* name, TokenList params, StmtList* body)
	{
		m_rettype = rettype;
		m_name = name;
		m_params = params;
		m_body = body;
	}

	StatementTypeEnum GetType() { return STATEMENT_FUNCTION; }

	Token* Operator() { return m_name; }
	Token* RetType() { return m_rettype; }
	TokenList GetParams() { return m_params; }
	StmtList* GetBody() { return m_body; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("FunctionStmt::codegen()\n");

		std::string name = m_name->Lexeme();

		// check for existing function definition
		if (env->GetFunction(name))
		{
			printf("Function `%s` already defined!\n", name.c_str());
		}

		// build up the return and parameter types
		LiteralTypeEnum retliteral = LITERAL_TYPE_INVALID;
		Type* rettype = nullptr;
		if (!m_rettype)
		{
			rettype = builder->getVoidTy();
		}
		else
		{
			switch (m_rettype->GetType())
			{
			case TOKEN_VAR_I32:
				rettype = builder->getInt32Ty();
				retliteral = LITERAL_TYPE_INTEGER;
				break;
			case TOKEN_VAR_F32:
				rettype = builder->getDoubleTy();
				retliteral = LITERAL_TYPE_DOUBLE;
				break;
			case TOKEN_VAR_BOOL:
				rettype = builder->getInt1Ty();
				retliteral = LITERAL_TYPE_BOOL;
				break;
			}
		}
		
		FunctionType* FT = FunctionType::get(rettype, {}, false);
		Function* ftn = Function::Create(FT, Function::InternalLinkage, name, *module);
		env->DefineFunction(name, ftn, 0, retliteral);

		BasicBlock* body = BasicBlock::Create(*context, "entry", ftn);
		builder->SetInsertPoint(body);

		Environment* sub_env = new Environment(env);

		for (auto& statement : *m_body)
		{
			if (STATEMENT_FUNCTION != statement->GetType())
			{
				statement->codegen(context, builder, module, sub_env);
			}
		}

		delete sub_env;

		builder->CreateRetVoid();
		verifyFunction(*ftn);

		return TValue::NullInvalid();
	}

private:
	Token* m_name;
	Token* m_rettype;
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

	Expr* GetCondition() { return m_condition; }
	Stmt* GetThenBranch() { return m_thenBranch; }
	Stmt* GetElseBranch() { return m_elseBranch; }
	
	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("IfStmt::codegen()\n");
		TValue CondV = m_condition->codegen(context, builder, module, env);
		if (!CondV.value) return TValue::NullInvalid();

		Function* ftn = builder->GetInsertBlock()->getParent();

		BasicBlock* ThenBB = BasicBlock::Create(*context, "then", ftn);
		BasicBlock* ElseBB = BasicBlock::Create(*context, "else");
		BasicBlock* MergeBB = BasicBlock::Create(*context, "ifcont");

		builder->CreateCondBr(CondV.value, ThenBB, ElseBB);
		builder->SetInsertPoint(ThenBB);

		m_thenBranch->codegen(context, builder, module, env);

		builder->CreateBr(MergeBB);

		ftn->insert(ftn->end(), ElseBB);
		builder->SetInsertPoint(ElseBB);

		if (m_elseBranch) m_elseBranch->codegen(context, builder, module, env);
		
		builder->CreateBr(MergeBB);

		ftn->insert(ftn->end(), MergeBB);
		builder->SetInsertPoint(MergeBB);

		return TValue::NullInvalid();
	}

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

	Expr* Expression() { return m_expr; }

	StatementTypeEnum GetType() { return STATEMENT_PRINT; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("PrintStmt::codegen()\n");

		TValue v = m_expr->codegen(context, builder, module, env);
		
		if (!v.IsString())
		{
			AllocaInst* a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(33), "alloctmp");
			TValue b = TValue(LITERAL_TYPE_STRING, builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
			builder->CreateStore(builder->getInt8(0), b.value);

			if (v.IsInteger())
			{
				builder->CreateCall(module->getFunction("itoa2"), { v.value, b.value }, "calltmp");
			}
			else if (v.IsDouble())
			{
				builder->CreateCall(module->getFunction("ftoa2"), { v.value, b.value }, "calltmp");
			}
			else if (v.IsBool())
			{
				builder->CreateCall(module->getFunction("btoa2"), { v.value, b.value }, "calltmp");
			}

			v = b;
		}
		
		if (m_newline)
			builder->CreateCall(module->getFunction("println"), { v.value }, "calltmp");
		else
			builder->CreateCall(module->getFunction("print"), { v.value }, "calltmp");

		return TValue::NullInvalid();
	}

private:
	Expr* m_expr;
	bool m_newline;
};



//-----------------------------------------------------------------------------
class VarStmt : public Stmt
{
public:
	VarStmt() = delete;

	VarStmt(Token* type, Token* name, Expr* expr)
	{
		m_type = type;
		m_token = name;
		m_expr = expr;
	}

	Expr* Expression() { return m_expr; }
	Token* Operator() { return m_token; }
	Token* VarType() { return m_type; }

	StatementTypeEnum GetType() { return STATEMENT_VAR; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("VarStmt::codegen()\n");

		TokenTypeEnum varType = m_type->GetType();

		AllocaInst* a = nullptr;

		if (TOKEN_VAR_I32 == varType)
		{
			a = builder->CreateAlloca(builder->getInt32Ty(), nullptr, "alloc_i32");
			env->DefineVariable(LITERAL_TYPE_INTEGER, m_token->Lexeme(), a);
			if (!m_expr) builder->CreateStore(builder->getInt32(0), a);
		}
		else if (TOKEN_VAR_BOOL == varType)
		{
			a = builder->CreateAlloca(builder->getInt32Ty(), nullptr, "alloc_bool");
			env->DefineVariable(LITERAL_TYPE_BOOL, m_token->Lexeme(), a);
			if (!m_expr) builder->CreateStore(builder->getInt32(0), a);
		}
		else if (TOKEN_VAR_F32 == varType)
		{
			a = builder->CreateAlloca(builder->getDoubleTy(), nullptr, "alloc_f64");
			env->DefineVariable(LITERAL_TYPE_DOUBLE, m_token->Lexeme(), a);
			if (!m_expr) builder->CreateStore(Constant::getNullValue(builder->getDoubleTy()), a);
		}
		else if (TOKEN_VAR_STRING == varType)
		{
			a = builder->CreateAlloca(builder->getInt8Ty(), builder->getInt32(1024), "alloc_str");
			env->DefineVariable(LITERAL_TYPE_STRING, m_token->Lexeme(), a);

			if (!m_expr)
			{
				TValue b = TValue(LITERAL_TYPE_STRING, builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
				builder->CreateStore(builder->getInt8(0), b.value);
			}
		}

		if (a && m_expr)
		{
			TValue rhs = m_expr->codegen(context, builder, module, env);
			if (!rhs.value) return TValue::NullInvalid();

			if (rhs.IsString())
			{
				TValue b = TValue(LITERAL_TYPE_STRING, builder->CreateGEP(a->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
				builder->CreateCall(module->getFunction("strncpy"), { b.value, rhs.value, builder->getInt32(1023) }, "calltmp");
			}
			else
			{
				if (TOKEN_VAR_I32 == varType && rhs.IsDouble())
				{
					rhs = TValue(LITERAL_TYPE_INTEGER, builder->CreateFPToSI(rhs.value, builder->getInt32Ty(), "cast_to_int"));
				}
				else if (TOKEN_VAR_F32 == varType && rhs.IsInteger())
				{
					rhs = TValue(LITERAL_TYPE_DOUBLE, builder->CreateSIToFP(rhs.value, builder->getDoubleTy(), "cast_to_dbl"));
				}

				builder->CreateStore(rhs.value, a);
			}
		}

		return TValue::NullInvalid();
	}

private:
	Token* m_type;
	Token* m_token;
	Expr* m_expr;
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

	Expr* GetCondition() { return m_condition; }
	Expr* GetPost() { return m_post; }
	Stmt* GetBody() { return m_body; }

	TValue codegen(std::unique_ptr<LLVMContext>& context,
		std::unique_ptr<IRBuilder<>>& builder,
		std::unique_ptr<Module>& module,
		Environment* env)
	{
		printf("WhileStmt::codegen()\n");

		Function* ftn = builder->GetInsertBlock()->getParent();
		BasicBlock* LoopBB = BasicBlock::Create(*context, "loop", ftn);
		BasicBlock* BodyBB = BasicBlock::Create(*context, "body");
		BasicBlock* MergeBB = BasicBlock::Create(*context, "loopcont");

		builder->CreateBr(LoopBB);
		builder->SetInsertPoint(LoopBB);

		TValue CondV = m_condition->codegen(context, builder, module, env);
		if (!CondV.value) return TValue::NullInvalid();

		builder->CreateCondBr(CondV.value, BodyBB, MergeBB);
		
		ftn->insert(ftn->end(), BodyBB);
		builder->SetInsertPoint(BodyBB);

		if (m_body) m_body->codegen(context, builder, module, env);
		if (m_post) m_post->codegen(context, builder, module, env);

		builder->CreateBr(LoopBB); // go back to the top

		ftn->insert(ftn->end(), MergeBB);
		builder->SetInsertPoint(MergeBB);

		return TValue::NullInvalid();
	}

private:
	Expr* m_condition;
	Expr* m_post;
	Stmt* m_body;
};

#endif // STATEMENTS_H