#include "Expressions.h"


//-----------------------------------------------------------------------------
TValue CallExpr::codegen_math(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee)
{
	std::string name = callee->Lexeme();

	if (0 == name.compare("abs"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage();
		if (v.IsInteger())
		{
			v = v.CastToInt(64);
			return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("abs"), { v.Value() }, "calltmp"));
		}
		else if (v.IsFloat())
		{
			v = v.CastToFloat(64);
			return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("fabs"), { v.Value() }, "calltmp"));
		}
		else
		{
			env->Error(callee, "Invalid parameter type.");
		}
	}
	else if (0 == name.compare("deg2rad"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		llvm::Value* tmp = llvm::ConstantFP::get(builder->getContext(), llvm::APFloat(DEG2RAD));
		return TValue::MakeFloat(callee, 64, builder->CreateFMul(v.Value(), tmp));
	}
	else if (0 == name.compare("rad2deg"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		llvm::Value* tmp = llvm::ConstantFP::get(builder->getContext(), llvm::APFloat(RAD2DEG));
		return TValue::MakeFloat(callee, 64, builder->CreateFMul(v.Value(), tmp));
	}
	else if (0 == name.compare("cos") ||
		0 == name.compare("sin") ||
		0 == name.compare("tan") ||
		0 == name.compare("acos") ||
		0 == name.compare("asin") ||
		0 == name.compare("atan") ||
		0 == name.compare("floor") ||
		0 == name.compare("sqrt"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { v.Value()}, "calltmp"));
	}
	else if (0 == name.compare("sgn"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("__sgn"), { v.Value() }, "calltmp"));
	}
	else if (0 == name.compare("atan2"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { lhs.Value(), rhs.Value() }, "calltmp"));
	}
	/*else if (0 == name.compare("pow"))
		{
		if (!CheckArgSize(2)) return TValue::NullInvalid();
			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(builder, module, env);
				if (LITERAL_TYPE_INTEGER == v.type)
				{
					// convert rhs to double
					v = TValue::Double(builder->CreateSIToFP(v.value, builder->getDoubleTy(), "cast_to_dbl"));
				}
				args.push_back(v.value);
			}
			llvm::Value* rval = builder->CreateCall(module->getFunction("pow_impl"), args, std::string("call_" + name).c_str());
			return TValue::Double(rval);
	}*/
	
	
	return TValue::NullInvalid();

}

