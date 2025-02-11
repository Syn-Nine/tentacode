#include "Expressions.h"

//-----------------------------------------------------------------------------
TValue CallExpr::codegen_map(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee)
{
	std::string name = callee->Lexeme();

	if (0 == name.compare("map::insert"))
	{
		if (2 > m_arguments.size() || 3 < m_arguments.size())
		{
			Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
			Environment::Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}

		if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			TValue tval = env->GetVariable(callee, var);

			TValue mhs = m_arguments[1]->codegen(builder, module, env);
			TValue rhs;
			if (3 == m_arguments.size())
			{
				rhs = m_arguments[2]->codegen(builder, module, env);
				mhs = TValue::Construct_Pair(mhs, rhs);
			}
			
			if (tval.IsMap() && mhs.IsMapPair())
			{
				tval.EmitMapInsert(mhs);
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
			}
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("map::contains"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();

			TValue tval = env->GetVariable(callee, var);
			if (tval.IsMap())
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
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}	
	}
	/*else if (0 == name.compare("vec::fill"))
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
	}*/

	return TValue::NullInvalid();
}

