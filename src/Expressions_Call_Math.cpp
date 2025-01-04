#include "Expressions.h"

//-----------------------------------------------------------------------------
TValue CallExpr::codegen_math(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env, Token* callee)
{
	std::string name = callee->Lexeme();

	if (0 == name.compare("abs"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (!v.IsValid()) return TValue::NullInvalid();
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
		if (!v.IsValid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		llvm::Value* tmp = llvm::ConstantFP::get(builder->getContext(), llvm::APFloat(DEG2RAD));
		return TValue::MakeFloat(callee, 64, builder->CreateFMul(v.Value(), tmp));
	}
	else if (0 == name.compare("rad2deg"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (!v.IsValid()) return TValue::NullInvalid();
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
		0 == name.compare("ceil") ||
		0 == name.compare("round") ||
		0 == name.compare("fract") ||
		0 == name.compare("sqrt"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (!v.IsValid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { v.Value()}, "calltmp"));
	}
	else if (0 == name.compare("sgn"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env);
		if (!v.IsValid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("__sgn"), { v.Value() }, "calltmp"));
	}
	else if (0 == name.compare("atan2"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		if (!lhs.IsValid()) return TValue::NullInvalid();
		if (!rhs.IsValid()) return TValue::NullInvalid();
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { lhs.Value(), rhs.Value() }, "calltmp"));
	}
	else if (0 == name.compare("mix") ||
		0 == name.compare("smoothstep") ||
		0 == name.compare("smootherstep") ||
		0 == name.compare("clamp"))
	{
		if (!CheckArgSize(3)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		TValue mhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		TValue rhs = m_arguments[2]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		if (!lhs.IsValid()) return TValue::NullInvalid();
		if (!rhs.IsValid()) return TValue::NullInvalid();
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { lhs.Value(), mhs.Value(), rhs.Value() }, "calltmp"));
	}
	else if (0 == name.compare("pow"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage().CastToFloat(64);
		if (!lhs.IsValid()) return TValue::NullInvalid();
		if (!rhs.IsValid()) return TValue::NullInvalid();
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { lhs.Value(), rhs.Value() }, "calltmp"));
	}	
	if (0 == name.compare("min") ||
		0 == name.compare("max"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (!lhs.IsValid()) return TValue::NullInvalid();
		if (!rhs.IsValid()) return TValue::NullInvalid();

		if (lhs.IsFloat() && rhs.IsInteger())
		{
			rhs = rhs.CastToMatchImplicit(lhs);
		}
		else if (lhs.IsInteger() && rhs.IsFloat())
		{
			lhs = lhs.CastToMatchImplicit(rhs);
		}

		if (lhs.IsInteger())
		{
			lhs = lhs.CastToInt(64);
			rhs = rhs.CastToInt(64);
			if (0 == name.compare("min"))
				return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("__min_impl"), { lhs.Value(), rhs.Value()}, "calltmp"));
			else
				return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("__max_impl"), { lhs.Value(), rhs.Value() }, "calltmp"));
		}
		else if (lhs.IsFloat())
		{
			lhs = lhs.CastToFloat(64);
			rhs = rhs.CastToFloat(64);
			if (0 == name.compare("min"))
				return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("__minf_impl"), { lhs.Value(), rhs.Value() }, "calltmp"));
			else
				return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("__maxf_impl"), { lhs.Value(), rhs.Value() }, "calltmp"));
		}
		else
		{
			env->Error(callee, "Invalid parameter type.");
		}
	}

	
	return TValue::NullInvalid();

}

