#include "TValue.h"
#include "Environment.h"
#include "TVec.h"
#include "TStruct.h"
#include "TFunction.h"

#include "llvm/Support/Casting.h"


//-----------------------------------------------------------------------------
TValue TValue::FromLiteral(Token* token, const Literal& literal)
{
	switch (literal.GetType())
	{
	case LITERAL_TYPE_INTEGER:
	{
		TValue ret = MakeInt(token, 32, m_builder->getInt32(literal.IntValue()));
		ret.m_ttype.SetConstant();
		return ret;
	}
	case LITERAL_TYPE_FLOAT:
		return MakeFloat(token, 64, llvm::ConstantFP::get(m_builder->getContext(), llvm::APFloat(literal.DoubleValue())));

	case LITERAL_TYPE_ENUM:
		return MakeEnum(token, m_builder->getInt32(Environment::GetEnumAsInt(literal.EnumValue().enumValue)));

	case LITERAL_TYPE_BOOL:
		if (literal.BoolValue())
			return MakeBool(token, m_builder->getTrue());
		else
			return MakeBool(token, m_builder->getFalse());

	case LITERAL_TYPE_STRING:
	{
		llvm::Value* g = m_builder->CreateGlobalStringPtr(literal.StringValue(), "str_literal");
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_from_literal"), { g }, "strptr");
		//printf("from literal %s\n", literal.StringValue().c_str());
		return MakeString(token, s);
	}

	default:
		Error(token, "Invalid type in literal constructor.");
	}

	return NullInvalid();
}



//-----------------------------------------------------------------------------
TValue TValue::Construct(TType ttype, std::string lexeme, bool global, TValueList* targs /* = nullptr */)
{
	llvm::Type* defty = ttype.GetTy();
	llvm::Value* defval = nullptr;
	llvm::Constant* nullval = llvm::Constant::getNullValue(defty);

	if (global)
	{
		llvm::Constant* nullval = llvm::Constant::getNullValue(defty);
		defval = new llvm::GlobalVariable(*m_module, defty, false, llvm::GlobalValue::InternalLinkage, nullval, lexeme);
		if (!defval) Error(ttype.GetToken(), "Failed to create global variable.");
	}
	else
	{
		defval = CreateEntryAlloca(m_builder, defty, nullptr, lexeme);
		if (defval)
			m_builder->CreateStore(nullval, defval);
		else
			Error(ttype.GetToken(), "Failed to create stack variable.");
	}

	if (defval)
	{
		TValue ret;
		ret.m_is_storage = true;
		ret.m_is_valid = true;
		ret.m_ttype = ttype;
		ret.m_value = defval;
		ret.m_token = ttype.GetToken();

		if (ret.IsString())
		{
			llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_void"), { }, "calltmp_construct");
			m_builder->CreateStore(s, defval);
			Environment::AddToCleanup(ret);
		}
		else if (ret.IsVecFixed())
		{
			TType i0 = ttype.GetInternal(0);
			int len = ttype.GetFixedVecLen();
			ret.m_is_storage = false;

			// default construction for vector of strings
			if (i0.IsString())
			{
				// initialize strings
				for (size_t i = 0; i < len; ++i)
				{
					llvm::Value* gep = m_builder->CreateGEP(i0.GetTy(), defval, m_builder->getInt32(i), "geptmp");
					llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_void"), {}, "calltmp_vecfixed_string");
					m_builder->CreateStore(s, gep);
				}
			}

			// construct strings inside vec of UDTs - todo make this recursive
			if (i0.IsUDT())
			{
				// construct internal strings
				TStruct tstruc = Environment::GetStruct(ret.m_token, i0.GetLexeme());

				std::vector<std::string>& member_names = tstruc.GetMemberNames();
				std::vector<TType>& members = tstruc.GetMemberVec();

				for (size_t j = 0; j < ttype.GetFixedVecLen(); ++j)
				{
					llvm::Value* gep_idx = m_builder->getInt32(j);

					for (size_t i = 0; i < members.size(); ++i)
					{
						// todo - add support initialization of complex containers
						// initialize empty strings
						if (members[i].IsString())
						{
							llvm::Value* gep = m_builder->CreateGEP(i0.GetTy(), ret.m_value, { gep_idx, m_builder->getInt32(i) }, member_names[i] + "_init");
							llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_void"), { }, "calltmp_vecfixed_udt");
							m_builder->CreateStore(s, gep);
						}
					}
				}
			}

			if (targs)
			{
				int i = 0;
				for (auto& v : *targs)
				{
					if (i == len)
					{
						Error(ret.m_token, "Initializer length mismatch.");
						return NullInvalid();
					}
					TValue idx = TValue::MakeInt(ret.m_token, 32, m_builder->getInt32(i));
					v = v.CastToMatchImplicit(ret.m_ttype.GetInternal(0));
					ret.StoreAtIndex(idx, v);
					i++;
				}
			}

			Environment::AddToCleanup(ret);
		}
		else if (ret.IsVecDynamic())
		{
			TType i0 = ttype.GetInternal(0);
			llvm::Value* vtype = m_builder->getInt64(i0.GetLiteralType());
			llvm::Value* sz_bytes = i0.GetSzBytes();
			llvm::Value* ptr = m_builder->CreateCall(m_module->getFunction("__new_dyn_vec"), { vtype, sz_bytes }, "calltmp");
			//ret.m_is_storage = false;
			m_builder->CreateStore(ptr, defval);
			Environment::AddToCleanup(ret);

			if (targs)
			{
				for (auto& v : *targs)
				{
					v = v.CastToMatchImplicit(ret.m_ttype.GetInternal(0));
					ret.EmitAppend(v);
				}
			}
		}
		else if (ret.IsMap())
		{
			TType i0 = ttype.GetInternal(0);
			TType i1 = ttype.GetInternal(1);
			llvm::Value* vtype = m_builder->getInt64(i0.GetLiteralType());
			llvm::Value* bits = m_builder->getInt32(i0.NumBits());
			llvm::Value* span = i1.GetSzBytes();
			llvm::Value* ptr = m_builder->CreateCall(m_module->getFunction("__new_map"), { vtype, bits, span }, "calltmp");
			//ret.m_is_storage = false;
			m_builder->CreateStore(ptr, defval);
			Environment::AddToCleanup(ret);

			if (targs)
			{
				for (auto& v : *targs)
				{
					ret.EmitMapInsert(v);
				}
			}
		}
		else if (ret.IsSet())
		{
			TType i0 = ttype.GetInternal(0);
			llvm::Value* vtype = m_builder->getInt64(i0.GetLiteralType());
			llvm::Value* sz_bits = m_builder->getInt32(i0.NumBits());
			llvm::Value* ptr = m_builder->CreateCall(m_module->getFunction("__new_set"), { vtype, sz_bits }, "calltmp");
			//ret.m_is_storage = false;
			m_builder->CreateStore(ptr, defval);
			Environment::AddToCleanup(ret);

			if (targs)
			{
				for (auto& v : *targs)
				{
					ret.EmitSetInsert(v);
				}
			}
		}
		else if (ret.IsTuple())
		{
			ret.m_is_storage = false;
			if (targs)
			{
				int i = 0;
				for (auto& v : *targs)
				{
					Token* token = new Token(TOKEN_INTEGER, "tmp", i, i, ret.m_token->Line(), ret.m_token->Filename());
					TValue idx = MakeInt(token, 32, m_builder->getInt32(i));
					idx.m_ttype.SetConstant();
					ret.StoreAtIndex(idx, v);
					delete token;
					i++;
				}
			}
			Environment::AddToCleanup(ret);
		}
		else if (ret.IsUDT())
		{
			// construct internal strings
			TStruct tstruc = Environment::GetStruct(ret.m_token, ttype.GetLexeme());

			std::vector<std::string>& member_names = tstruc.GetMemberNames();
			std::vector<TType>& members = tstruc.GetMemberVec();

			llvm::Value* gep_zero = m_builder->getInt32(0);

			for (size_t i = 0; i < members.size(); ++i)
			{
				// todo - add support initialization of complex containers
				// initialize empty strings
				if (members[i].IsString())
				{
					llvm::Value* gep = m_builder->CreateGEP(ttype.GetTy(), ret.m_value, { gep_zero, m_builder->getInt32(i) }, member_names[i] + "_init");
					llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_void"), { }, "calltmp_udt");
					m_builder->CreateStore(s, gep);
				}
			}
			Environment::AddToCleanup(ret);
			ret.m_is_storage = false;
		}

		return ret;
	}

	Error(ttype.GetToken(), "Invalid type in variable constructor.");

	return TValue::NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Construct_Brace(Token* token, TValueList* targs)
{
	TValue ret;

	ret.m_ttype = TType::Construct_Brace(token);
	ret.m_token = token;
	ret.m_brace_args = *targs;
	ret.m_is_valid = true;
	
	return ret;
}



//-----------------------------------------------------------------------------
TValue TValue::Construct_ConstInt(Token* token, std::string lexeme, Literal lit)
{
	TType ttype = TType::Construct(token, nullptr);
	llvm::Type* defty = ttype.GetTy();
	int bits = ttype.NumBits();

	llvm::Constant* init = llvm::ConstantInt::get(defty, llvm::APInt(bits, lit.IntValue(), true));
	llvm::Value* defval = nullptr;

	defval = new llvm::GlobalVariable(*m_module, defty, false, llvm::GlobalValue::InternalLinkage, init, lexeme);
	if (!defval) Error(token, "Failed to create global constant variable.");

	if (defval)
	{
		TValue ret = MakeInt(token, bits, defval);
		ret.m_ttype.SetConstant();
		ret.m_is_storage = true;
		return ret;
	}

	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Construct_Functor(TType ttype, const std::string& anon_sig, llvm::Function* func)
{
	// make a wrapper around an llvm pointer to a function
	TValue ret;
	ret.m_is_storage = true;
	ret.m_ttype = ttype;
	ret.m_token = ttype.GetToken();
	llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
	m_builder->CreateStore(func, a);
	ret.m_value = a;
	ret.m_functor_sig = anon_sig;
	ret.m_is_valid = true;

	//m_builder->CreateStore(s, defval);

	return ret;
}

//-----------------------------------------------------------------------------
TValue TValue::Construct_Reference(TType ttype, std::string lexeme, llvm::Value* ptr)
{
	// make a wrapper around an llvm pointer to another variable
	TValue ret;
	ret.m_is_storage = true;
	ret.m_ttype = ttype;
	ret.m_token = ttype.GetToken();
	ret.m_value = ptr;
	ret.m_is_valid = true;

	return ret;
}


//-----------------------------------------------------------------------------
TValue TValue::Construct_Pair(TValue lhs, TValue rhs)
{
	// create an anonymous pair for use in map types
	// make sure we have the real values, not the storage locations
	GetFromStorage(lhs, rhs);
	TValue ret;
	ret.m_is_valid = true;
	ret.m_is_map_pair = true;
	ret.m_pair_lhs = new TValue(lhs);
	ret.m_pair_rhs = new TValue(rhs);
	return ret;
}

/****

//-----------------------------------------------------------------------------
TValue TValue::Construct_Explicit(
	Token* token,
	LiteralTypeEnum type,
	llvm::Value* value,
	llvm::Type* ty,
	int bits,
	bool is_storage,
	int fixed_vec_len,
	LiteralTypeEnum vec_type)
{
	TValue ret;
	ret.m_token = token;
	ret.m_type = type;
	ret.m_value = value;
	ret.m_ty = ty;
	ret.m_bits = bits;
	ret.m_is_storage = is_storage;
	ret.m_fixed_vec_len = fixed_vec_len;
	ret.m_vec_type = vec_type;
	return ret;
}


//-----------------------------------------------------------------------------
TValue TValue::Construct_Pointer(Token* token, std::string lexeme, bool global)
{
	llvm::Type* defty = m_builder->getPtrTy();
	llvm::Value* defval = nullptr;
	llvm::Constant* nullval = llvm::Constant::getNullValue(defty);

	if (global)
	{
		defval = new llvm::GlobalVariable(*m_module, defty, false, llvm::GlobalValue::InternalLinkage, nullval, lexeme);
		if (!defval) Error(token, "Failed to create global variable.");
	}
	else
	{
		defval = CreateEntryAlloca(m_builder, defty, nullptr, lexeme);
		if (defval)
			m_builder->CreateStore(nullval, defval);
		else
			Error(token, "Failed to create stack variable.");
	}

	if (defval)
	{
		TValue ret;
		ret.m_type = LITERAL_TYPE_POINTER;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		ret.m_is_storage = true;
		return ret;
	}

	return NullInvalid();
}


TValue TValue::Construct_Struct(Token* token, std::string lexeme, bool global)
{
	std::string udtname = token->Lexeme();
	
	TStruct struc = Environment::GetStruct(token, udtname);
	if (!struc.IsValid()) return NullInvalid();

	llvm::Type* defty = struc.GetLLVMStruct();
	llvm::Value* defval = nullptr;
	llvm::Constant* nullval = llvm::Constant::getNullValue(defty);

	if (global)
	{
		defval = new llvm::GlobalVariable(*m_module, defty, false, llvm::GlobalValue::InternalLinkage, nullval, lexeme);
		if (!defval) Error(token, "Failed to create global variable.");
	}
	else
	{
		defval = CreateEntryAlloca(m_builder, defty, nullptr, lexeme);
		if (defval)
			m_builder->CreateStore(nullval, defval);
		else
			Error(token, "Failed to create stack variable.");
	}

	if (defval)
	{
		TValue ret;
		ret.m_type = LITERAL_TYPE_UDT;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		return ret;
	}

	return NullInvalid();
}




//-----------------------------------------------------------------------------
TValue TValue::Construct_ReturnValue(Token* token, LiteralTypeEnum type, int bits, llvm::Value* value)
{
	TValue ret;
	ret.m_token = token;
	ret.m_type = type;
	ret.m_bits = bits;
	ret.m_value = value;
	ret.m_ty = MakeTy(type, bits);

	if (!ret.m_ty)
	{
		Error(token, "Invalid type in return constructor.");
		return NullInvalid();
	}

	if (LITERAL_TYPE_STRING == type || LITERAL_TYPE_VEC_DYNAMIC == type)
	{
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		m_builder->CreateStore(value, a);
		ret.m_value = a;
		ret.m_is_storage = true;
		Environment::AddToCleanup(ret);
	}

	return ret;
}

*/

//-----------------------------------------------------------------------------
TValue TValue::MakeBool(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_ttype = TType::Construct_Bool(token);
	ret.m_token = token;
	ret.m_value = value;
	ret.m_is_valid = true;
	return ret;
}


//-----------------------------------------------------------------------------
TValue TValue::MakeEnum(Token* token, llvm::Value* value)
{
	TValue ret = MakeInt(token, 32, value);
	ret.m_ttype = TType::Construct_Enum(token);
	ret.m_is_valid = true;
	return ret;
}


//-----------------------------------------------------------------------------
TValue TValue::MakeFloat(Token* token, int bits, llvm::Value* value)
{
	if (32 != bits && 64 != bits)
	{
		Error(token, "Invalid bitsize for float type.");
		return NullInvalid();
	}

	TValue ret;
	ret.m_ttype = TType::Construct_Float(token, bits);
	ret.m_token = token;
	ret.m_value = value;
	ret.m_is_valid = true;
	return ret;
}

//-----------------------------------------------------------------------------
TValue TValue::MakeInt(Token* token, int bits, llvm::Value* value)
{
	if (16 != bits && 32 != bits && 64 != bits)
	{
		Error(token, "Invalid bitsize for integer type.");
		return NullInvalid();
	}

	TValue ret;
	ret.m_ttype = TType::Construct_Int(token, bits);
	ret.m_token = token;	
	ret.m_value = value;
	ret.m_is_valid = true;
	return ret;
}

//-----------------------------------------------------------------------------
TValue TValue::MakeString(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_ttype = TType::Construct_String(token);
	ret.m_token = token;
	llvm::Value* a = CreateEntryAlloca(m_builder, ret.GetTy(), nullptr, "alloctmp");
	m_builder->CreateStore(value, a);
	ret.m_value = a;
	ret.m_is_storage = true;
	ret.m_is_valid = true;
	Environment::AddToCleanup(ret);
	return ret;
}

//-----------------------------------------------------------------------------
TValue TValue::MakeDynVec(Token* token, llvm::Value* value, LiteralTypeEnum vec_type, int vec_bits)
{
	TokenPtrList args;
	TokenTypeEnum token_type;
	
	if (LITERAL_TYPE_INTEGER == vec_type)
	{
		if (16 == vec_bits) token_type = TOKEN_VAR_I16;
		else if (32 == vec_bits) token_type = TOKEN_VAR_I32;
		else if (64 == vec_bits) token_type = TOKEN_VAR_I64;
	}
	else if (LITERAL_TYPE_FLOAT == vec_type)
	{
		if (32 == vec_bits) token_type = TOKEN_VAR_F32;
		else if (64 == vec_bits) token_type = TOKEN_VAR_F64;
	}
	else if (LITERAL_TYPE_ENUM == vec_type) token_type = TOKEN_VAR_ENUM;
	else if (LITERAL_TYPE_BOOL == vec_type) token_type = TOKEN_VAR_BOOL;
	else if (LITERAL_TYPE_STRING == vec_type) token_type = TOKEN_VAR_STRING;
	else
	{
		Error(token, "Invalid dynamic type.");
		return NullInvalid();
	}
	
	args.push_back(new Token(token_type, "tmp", token->Line(), token->Filename()));
	args.push_back(nullptr);
	TType ttype = TType::Construct_Vec(token, &args);
	TValue ret = TValue::Construct(ttype, "vectmp", false);
	m_builder->CreateStore(value, ret.Value());
	Environment::AddToCleanup(ret);
	return ret;
}


/****
//-----------------------------------------------------------------------------
llvm::Type* TValue::MakeTy(LiteralTypeEnum type, int bits)
{
	if (LITERAL_TYPE_INTEGER == type)          return MakeIntTy(bits);
	else if (LITERAL_TYPE_FLOAT == type)       return MakeFloatTy(bits);
	else if (LITERAL_TYPE_ENUM == type)        return MakeIntTy(32);
	else if (LITERAL_TYPE_BOOL == type)        return MakeIntTy(1);
	else if (LITERAL_TYPE_STRING == type)      return MakePointerTy();
	else if (LITERAL_TYPE_VEC_DYNAMIC == type) return MakePointerTy();
	else if (LITERAL_TYPE_POINTER == type)     return MakePointerTy();
	else if (LITERAL_TYPE_INVALID == type)     return MakeVoidTy();

	return nullptr;
}



//-----------------------------------------------------------------------------
llvm::Type* TValue::MakeFloatTy(int bits)
{
	if (32 != bits && 64 != bits)
	{
		Error(nullptr, "Invalid float type.");
		return nullptr;
	}

	if (32 == bits)
		return m_builder->getFloatTy();
	else
		return m_builder->getDoubleTy();
}

*/