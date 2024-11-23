#include "Expressions.h"
#include "Environment.h"


//-----------------------------------------------------------------------------
bool CallExpr::CheckArgSize(int count)
{
	if (count != m_arguments.size())
	{
		Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
		Environment::Error(callee, "Argument count mismatch.");
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
TValue CallExpr::codegen(llvm::IRBuilder<>* builder, llvm::Module* module, Environment* env)
{
	if (2 <= Environment::GetDebugLevel()) printf("CallExpr::codegen()\n");
	
	if (!m_callee) return TValue::NullInvalid();
	if (EXPRESSION_VARIABLE != m_callee->GetType()) return TValue::NullInvalid();

	Token* callee = (static_cast<VariableExpr*>(m_callee))->Operator();
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

		TValue v = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		llvm::Value* tmp = llvm::ConstantFP::get(builder->getContext(), llvm::APFloat(DEG2RAD));
		return TValue::MakeFloat(callee, 64, builder->CreateFMul(v.Value(), tmp));
	}
	else if (0 == name.compare("rad2deg"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
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

		TValue v = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (v.IsInvalid()) return TValue::NullInvalid();
		v = v.GetFromStorage().CastToFloat(64);
		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction(name), { v.Value()}, "calltmp"));
	}
	else if (0 == name.compare("sgn"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue v = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
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
	else if (0 == name.compare("clock"))
	{
		if (!CheckArgSize(0)) return TValue::NullInvalid();

		return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("__clock_impl"), {}, "calltmp"));
	}
	else if (0 == name.compare("rand"))
	{
		if (m_arguments.empty())
		{
			return TValue::MakeFloat(callee, 64, builder->CreateCall(module->getFunction("__rand_impl"), {}, "calltmp"));
		}
		else if (EXPRESSION_RANGE == m_arguments[0]->GetType())
		{
			if (!CheckArgSize(1)) return TValue::NullInvalid();

			RangeExpr* re = static_cast<RangeExpr*>(m_arguments[0]);
			TValue lhs = re->Left()->codegen(builder, module, env).GetFromStorage().CastToInt(64);
			TValue rhs = re->Right()->codegen(builder, module, env).GetFromStorage().CastToInt(64);
			return TValue::MakeInt(callee, 64, builder->CreateCall(module->getFunction("__rand_range_impl"), { lhs.Value(), rhs.Value()}, "calltmp"));
		}
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("len"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		if (EXPRESSION_VARIABLE == m_arguments[0]->GetType())
		{
			VariableExpr* ve = static_cast<VariableExpr*>(m_arguments[0]);
			std::string var = ve->Operator()->Lexeme();
			TValue tval = env->GetVariable(callee, var);
			if (tval.IsVecAny()) return tval.EmitLen();

			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
		/*else if (EXPRESSION_GET == m_arguments[0]->GetType())
		{
			GetExpr* ge = static_cast<GetExpr*>(m_arguments[0]);
			TValue v = m_arguments[0]->codegen(builder, module, env);

			if (v.IsVec())
			{
				llvm::Value* src = builder->CreateLoad(builder->getPtrTy(), v.value);
				llvm::Value* srcType = builder->getInt32(v.fixed_vec_type);
				return TValue::Integer(builder->CreateCall(module->getFunction("__vec_len"), { srcType, src }, "calltmp"));
			}
			else
			{
				env->Error(callee, "Argument type mismatch.");
				return TValue::NullInvalid();
			}
		}*/
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::readlines"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* v = builder->CreateCall(module->getFunction("__file_readlines"), { arg.Value() }, "calltmp");
			return TValue::MakeDynVec(callee, v, LITERAL_TYPE_STRING, 64);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::readstring"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__file_readstring"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::writelines"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env);
		if (lhs.IsString() && rhs.IsVecAny())
		{
			if (rhs.IsVecDynamic())
			{
				llvm::Value* rhs_load = builder->CreateLoad(builder->getPtrTy(), rhs.Value(), "loadtmp");
				builder->CreateCall(module->getFunction("__file_writelines_dyn_vec"), { lhs.Value(), rhs_load }, "calltmp");
			}
			else
			{
				llvm::Value* gep = builder->CreateGEP(builder->getPtrTy(), rhs.Value(), builder->getInt32(0), "geptmp");
				llvm::Value* len = builder->getInt64(rhs.FixedVecLen());
				builder->CreateCall(module->getFunction("__file_writelines_fixed_vec"), { lhs.Value(), gep, len }, "calltmp");
			}
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("file::writestring"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && rhs.IsString())
		{
			builder->CreateCall(module->getFunction("__file_writestring"), { lhs.Value(), rhs.Value() }, "calltmp");
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::contains"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* b = builder->CreateCall(module->getFunction("__str_contains"), { lhs.Value(), rhs.Value() }, "calltmp");
			return TValue::MakeBool(callee, b);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::replace"))
	{
		if (!CheckArgSize(3)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue mhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[2]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && mhs.IsString() && rhs.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_replace"), { lhs.Value(), mhs.Value(), rhs.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::split"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsString() && rhs.IsString())
		{
			llvm::Value* v = builder->CreateCall(module->getFunction("__str_split"), { lhs.Value(), rhs.Value() }, "calltmp");
			return TValue::MakeDynVec(callee, v, LITERAL_TYPE_STRING, 64);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::join"))
	{
		if (!CheckArgSize(2)) return TValue::NullInvalid();

		TValue lhs = m_arguments[0]->codegen(builder, module, env);
		TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
		if (lhs.IsVecAny() && LITERAL_TYPE_STRING == lhs.GetVecType() && rhs.IsString())
		{
			llvm::Value* s = nullptr;
			if (lhs.IsVecDynamic())
			{
				llvm::Value* lhs_load = builder->CreateLoad(builder->getPtrTy(), lhs.Value(), "loadtmp");
				s = builder->CreateCall(module->getFunction("__str_join_dyn_vec"), { lhs_load, rhs.Value() }, "calltmp");
				return TValue::MakeString(callee, s);
			}
			else
			{
				llvm::Value* gep = builder->CreateGEP(builder->getPtrTy(), lhs.Value(), builder->getInt32(0), "geptmp");
				llvm::Value* len = builder->getInt64(lhs.FixedVecLen());
				s = builder->CreateCall(module->getFunction("__str_join_fixed_vec"), { gep, rhs.Value(), len }, "calltmp");
				return TValue::MakeString(callee, s);
			}
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::substr"))
	{
		if (2 <= m_arguments.size())
		{
			TValue lhs = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
			TValue rhs = m_arguments[1]->codegen(builder, module, env).GetFromStorage();
			if (lhs.IsString() && rhs.IsInteger())
			{
				llvm::Value* len;
				if (2 < m_arguments.size())
				{
					len = m_arguments[2]->codegen(builder, module, env).GetFromStorage().Value();
				}
				else
				{
					len = builder->getInt32(-1);
				}

				if (len->getType()->isIntegerTy())
				{
					llvm::Value* s = builder->CreateCall(module->getFunction("__str_substr"), { lhs.Value(), rhs.Value(), len }, "calltmp");
					return TValue::MakeString(callee, s);
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
		else
		{
			env->Error(callee, "Argument count mismatch.");
			return TValue::NullInvalid();
		}
		}
	else if (0 == name.compare("str::toupper"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_toupper"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::tolower"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_tolower"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::ltrim"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_ltrim"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::rtrim"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_rtrim"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("str::trim"))
	{
		if (!CheckArgSize(1)) return TValue::NullInvalid();

		TValue arg = m_arguments[0]->codegen(builder, module, env).GetFromStorage();
		if (arg.IsString())
		{
			llvm::Value* s = builder->CreateCall(module->getFunction("__str_trim"), { arg.Value() }, "calltmp");
			return TValue::MakeString(callee, s);
		}
		else
		{
			env->Error(callee, "Argument type mismatch.");
			return TValue::NullInvalid();
		}
	}
	else if (0 == name.compare("vec::append"))
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

	/*else if (0 == name.compare("input"))
	{
		llvm::Value* a = CreateEntryAlloca(builder, builder->getInt8Ty(), builder->getInt32(256), "alloctmp");
		TValue b = TValue::String(builder->CreateGEP(static_cast<llvm::AllocaInst*>(a)->getAllocatedType(), a, builder->getInt8(0), "geptmp"));
		builder->CreateStore(builder->getInt8(0), b.value);
		builder->CreateCall(module->getFunction("input"), { b.value }, "calltmp");
		return b;
	}
	else if (0 == name.compare("pow"))
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
	else
	{
		TFunction tfunc = env->GetFunction(callee, name);
		if (tfunc.IsValid())
		{
			llvm::Function* ftn = tfunc.GetLLVMFunc();

			std::vector<TValue> params = tfunc.GetParams();
			TValue ret = tfunc.GetReturn();

			if (!CheckArgSize(params.size())) return TValue::NullInvalid();
			
			std::vector<llvm::Value*> args;
			for (size_t i = 0; i < m_arguments.size(); ++i)
			{
				TValue v = m_arguments[i]->codegen(builder, module, env).GetFromStorage();

				if (params[i].isNumeric())
				{
					v = v.CastToMatchImplicit(params[i]);
				}
				else if (v.IsVecDynamic())
				{
					v.SetValue(builder->CreateLoad(builder->getPtrTy(), v.Value(), "tmp_load"));
				}
				else if (v.IsVecFixed())
				{
					v = v.CastFixedToDynVec();
					v.SetValue(builder->CreateLoad(builder->getPtrTy(), v.Value(), "tmp_load"));
				}
				args.push_back(v.Value());
			}
			llvm::Value* rval = builder->CreateCall(ftn, args, std::string("call_" + name).c_str());
			ret.SetValue(rval);
			return ret;
		}
		else
		{
			env->Error(callee, "Invalid function call.");
			return TValue::NullInvalid();
		}
	}
	
	return TValue::NullInvalid();

}

