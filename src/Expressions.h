#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include <string>
#include <cstdarg>

#include "TValue.h"
#include "Token.h"
#include "Literal.h"
#include "Environment.h"
#include "Utility.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"


#define PI 3.1415926535897932384626433832795028841971693993751058
#define RAD2DEG (180.0 / PI)
#define DEG2RAD (1 / RAD2DEG)


class Expr
{
public:
	virtual ExpressionTypeEnum GetType() = 0;
	virtual TValue codegen(
		llvm::IRBuilder<>* builder,
		llvm::Module* module,
		Environment* env) = 0;
};

typedef std::vector<Expr*> ArgList;



//-----------------------------------------------------------------------------
class AssignExpr : public Expr
{
public:
	AssignExpr() = delete;
	AssignExpr(Token* name, Expr* right, Expr* vecIndex)
	{
		m_token = name;
		m_right = right;
		m_vecIndex = vecIndex;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_ASSIGN; }

	/*Token* Operator() { return m_token; }
	Expr* Right() { return m_right; }
	Expr* VecIndex() { return m_vecIndex; }*/

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_right;
	Expr* m_vecIndex;
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

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};



//-----------------------------------------------------------------------------
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

	//Token* Operator() { return m_token; }
	//ArgList GetArguments() { return m_arguments; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);


private:
	Token* m_token;
	ArgList m_arguments;
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

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);
	
private:

	TValue codegen_file(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee);
	TValue codegen_math(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee);
	TValue codegen_str(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee);
	TValue codegen_vec(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee);
	
	bool CheckArgSize(int count);
	
	Expr* m_callee;
	Token* m_token;
	ArgList m_arguments;
};



//-----------------------------------------------------------------------------
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

	Token* Operator() { return m_token; }
	Expr* Object() { return m_object; }
	Expr* VecIndex() { return m_right; }
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_object;
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
	
	Expr* Expression() { return m_expression; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
	{
		if (2 <= Environment::GetDebugLevel()) printf("GroupExpr::codegen()\n");
		return m_expression->codegen(builder, module, env);
	}

private:
	Expr* m_expression;
};



//-----------------------------------------------------------------------------
class LiteralExpr : public Expr
{
public:
	LiteralExpr() = delete; // null not allowed
	LiteralExpr(Token* token, int32_t val) : m_token(token), m_literal(val) {}
	LiteralExpr(Token* token, double val) : m_token(token), m_literal(val) {}
	LiteralExpr(Token* token, std::string val) : m_token(token), m_literal(val) {}
	LiteralExpr(Token* token, EnumLiteral val) : m_token(token), m_literal(val) {}
	LiteralExpr(Token* token, bool val) : m_token(token), m_literal(val) {}

	//std::string ToString() { return m_literal.ToString(); }

	ExpressionTypeEnum GetType() { return EXPRESSION_LITERAL; }
	
	Literal GetLiteral() { return m_literal; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
	{
		if (2 <= Environment::GetDebugLevel()) printf("LiteralExpr::codegen()\n");
		return TValue::FromLiteral(m_token, m_literal);
	}
	

private:
	Literal m_literal;
	Token* m_token;
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
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

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

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};



//-----------------------------------------------------------------------------
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

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
	{
		if (2 <= Environment::GetDebugLevel()) printf("ReplicateExpr::codegen()\n");

		TValue val = m_left->codegen(builder, module, env);
		TValue qty = m_left->codegen(builder, module, env);

		/*AllocaInst* a = entry_builder.CreateAlloca(builder->getInt64Ty(), nullptr, "alloc_vec_ptr");
		if (LITERAL_TYPE_INTEGER == val.type && LITERAL_TYPE_INTEGER == qty.type)
		{
			Value* addr = builder->CreateCall(module->getFunction("__vec_new_rep_i32"), { val.value, qty.value }, "calltmp");
			Value* v = builder->CreateStore(addr, a);
			TValue::Vec(v, val.type);
		}*/

		/*if (EXPRESSION_LITERAL == m_right->GetType())
		{
			LiteralExpr* le = static_cast<LiteralExpr*>(m_right);
			Literal literal = le->GetLiteral();
			LiteralTypeEnum lt = literal.GetType();

			if (LITERAL_TYPE_INTEGER == lt)
			{
				int qty = literal.IntValue();
				Value* vqty = ConstantInt::get(*context, APInt(32, qty, true));

				if (val.IsInteger())
				{
					AllocaInst* a = entry_builder.CreateAlloca(builder->getInt32Ty(), vqty, "alloc_vec_tmp");

					// build a for loop
					Value* startval = builder->CreateLoad(builder->getInt32Ty(), builder->getInt32(0), "startval");

					Function* ftn = builder->GetInsertBlock()->getParent();
					BasicBlock* HeaderBB = builder->GetInsertBlock();
					BasicBlock* LoopBB = BasicBlock::Create(*context, "loop", ftn);

					builder->CreateBr(LoopBB);
					builder->SetInsertPoint(LoopBB);

					PHINode* phi = builder->CreatePHI(builder->getInt32Ty(), 2, "phivar");
					phi->addIncoming(startval, HeaderBB);

					Value* b = builder->CreateGEP(a->getAllocatedType(), a, phi, "geptmp");
					builder->CreateStore(val.value, b);

					Value* nextval = builder->CreateAdd(phi, builder->getInt32(1), "nextval");

					BasicBlock* endBB = builder->GetInsertBlock();
					BasicBlock* MergeBB = BasicBlock::Create(*context, "loopcont", ftn);

					Value* cond = builder->CreateICmpSLT(startval, nextval, "loopcmp");

					builder->CreateCondBr(cond, LoopBB, MergeBB);

					builder->SetInsertPoint(MergeBB);

					phi->addIncoming(nextval, MergeBB);

					Value* b = builder->CreateGEP(a->getAllocatedType(), a, builder->getInt32(0), "geptmp");
					return TValue::FixedVec(b, LITERAL_TYPE_INTEGER, sz);
				}
			}
		}*/


		return TValue::NullInvalid();
	}
	
private:
	Token* m_token;
	Expr* m_left;
	Expr* m_right;
};



//-----------------------------------------------------------------------------
class SetExpr : public Expr
{
public:
	SetExpr() = delete;
	SetExpr(Expr* object, Expr* value)
	{
		m_object = object;
		m_value = value;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_SET; }
	
	Expr* Object() { return m_object; }
	Expr* Value() { return m_value; }
	
	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Expr* m_object;
	Expr* m_value;
	
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

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_right;
};



//-----------------------------------------------------------------------------
class VariableExpr : public Expr
{
public:
	VariableExpr() = delete;
	VariableExpr(Token* name, Expr* right)
	{
		m_token = name;
		m_vecIndex = right;
	}

	ExpressionTypeEnum GetType() { return EXPRESSION_VARIABLE; }

	Token* Operator() { return m_token; }
	Expr* VecIndex() { return m_vecIndex; }

	TValue codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env);

private:
	Token* m_token;
	Expr* m_vecIndex;
};



#endif // EXPRESSIONS_H