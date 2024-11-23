#include "Expressions.h"
#include "Environment.h"



//-----------------------------------------------------------------------------
TValue CallExpr::codegen_vec(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee)
{
	std::string name = callee->Lexeme();

	if (0 == name.compare("vec::append"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			TValue tval = env->GetVariable(callee, var);
			TValue rhs = m_arguments[1]->codegen(builder, module, env);
			if (tval.IsVecDynamic())
			{
				tval.EmitAppend(rhs);
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
			}
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("vec::contains"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();

			TValue tval = env->GetVariable(callee, var);
			if (tval.IsVecAny())
			{
				TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
				return tval.EmitContains(rhs);
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
				return TValue::NullInvalid();
			}
		}
		}
	else if (0 == name.compare("vec::fill"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();

		if (rhs.IsInteger())
		{
			if (lhs.IsInteger())
			{
				llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_i32"), { lhs.Value(), rhs.Value() }, "calltmp");
				return TValue::MakeDynVec(callee, v, LITERAL_TYPE_INTEGER, 32);
			}
			else if (lhs.IsBool())
			{
				llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_bool"), { lhs.Value(), rhs.Value() }, "calltmp");
				return TValue::MakeDynVec(callee, v, LITERAL_TYPE_BOOL, 1);
			}
			if (lhs.IsEnum())
			{
				llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_enum"), { lhs.Value(), rhs.Value() }, "calltmp");
				return TValue::MakeDynVec(callee, v, LITERAL_TYPE_ENUM, 32);
			}
			else if (lhs.IsFloat())
			{
				lhs = lhs.CastToFloat(32);
				llvm::Value* v = builder->CreateCall(module->getFunction("__vec_new_rep_float"), { lhs.Value(), rhs.Value() }, "calltmp");
				return TValue::MakeDynVec(callee, v, LITERAL_TYPE_FLOAT, 32);
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
				return TValue::NullInvalid();
			}
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}

	return TValue::NullInvalid();
}

