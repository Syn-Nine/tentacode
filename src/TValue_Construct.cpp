#include "TValue.h"
#include "Environment.h"
#include "TVec.h"

TValue TValue::FromLiteral(Token* token, const Literal& literal)
{
	TValue ret;
	ret.m_token = token;

	LiteralTypeEnum type = literal.GetType();

	switch (type)
	{
	case LITERAL_TYPE_INTEGER:
		ret.m_type = type;
		ret.m_bits = 32;
		ret.m_value = m_builder->getInt32(literal.IntValue());
		ret.m_ty = m_builder->getInt32Ty();
		break;

	case LITERAL_TYPE_FLOAT:
		ret.m_type = type;
		ret.m_bits = 64;
		ret.m_value = llvm::ConstantFP::get(m_builder->getContext(), llvm::APFloat(literal.DoubleValue()));
		ret.m_ty = m_builder->getDoubleTy();
		break;

	case LITERAL_TYPE_ENUM:
		ret.m_type = type;
		ret.m_bits = 32;
		ret.m_value = m_builder->getInt32(Environment::GetEnumAsInt(literal.EnumValue().enumValue));
		ret.m_ty = m_builder->getInt32Ty();
		break;

	case LITERAL_TYPE_BOOL:
		ret.m_type = type;
		ret.m_bits = 1;
		if (literal.BoolValue())
			ret.m_value = m_builder->getTrue();
		else
			ret.m_value = m_builder->getFalse();

		ret.m_ty = m_builder->getInt1Ty();
		break;

	case LITERAL_TYPE_STRING:
	{
		ret.m_type = type;
		llvm::Value* g = m_builder->CreateGlobalStringPtr(literal.StringValue(), "str_literal");
		llvm::Value* p = m_builder->CreateCall(m_module->getFunction("__new_string_from_literal"), { g }, "strptr");
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		m_builder->CreateStore(p, a);
		ret.m_value = a;
		ret.m_is_storage = true;
		ret.m_ty = m_builder->getPtrTy();
		Environment::AddToCleanup(ret);
		break;
	}

	default:
		Error(token, "Invalid type in literal constructor.");
	}

	return ret;
}


TValue TValue::Construct(Token* token, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs /* = nullptr */)
{
	switch (token->GetType())
	{
	case TOKEN_VAR_I16:    return Construct_Int(token, 16, lexeme, global);
	case TOKEN_VAR_I32:    return Construct_Int(token, 32, lexeme, global);
	case TOKEN_VAR_I64:    return Construct_Int(token, 64, lexeme, global);
	case TOKEN_VAR_F32:    return Construct_Float(token, 32, lexeme, global);
	case TOKEN_VAR_F64:    return Construct_Float(token, 64, lexeme, global);
	case TOKEN_VAR_BOOL:   return Construct_Bool(token, lexeme, global);
	case TOKEN_VAR_ENUM:   return Construct_Enum(token, lexeme, global);
	case TOKEN_VAR_STRING: return Construct_String(token, lexeme, global);
	case TOKEN_VAR_VEC:
		if (args && 2 == args->size())
		{
			if (!args->at(1))
				return Construct_Vec_Dynamic(token, args, lexeme, global, targs);
			else
				return Construct_Vec_Fixed(token, args, lexeme, global, targs);
		}
		else
		{
			Error(token, "Expected vector arguments.");
		}

	case TOKEN_VAR_IMAGE: // intentional fall-through
	case TOKEN_VAR_TEXTURE:
	case TOKEN_VAR_FONT:
	case TOKEN_VAR_SOUND:
	case TOKEN_VAR_SHADER:
	case TOKEN_VAR_RENDER_TEXTURE_2D:
		return Construct_Pointer(token, lexeme, global);
		
	default:
		Error(token, "Invalid type in variable constructor.");
	}

	return TValue::NullInvalid();
}


TValue TValue::Construct_Int(Token* token, int bits, std::string lexeme, bool global)
{
	if (16 != bits && 32 != bits && 64 != bits)
	{
		Error(token, "Invalid number of bits in Int constructor.");
		return NullInvalid();
	}

	llvm::Type* defty = m_builder->getIntNTy(bits);
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
		ret.m_type = LITERAL_TYPE_INTEGER;
		ret.m_bits = bits;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		ret.m_is_storage = true;
		return ret;
	}

	return NullInvalid();
}


TValue TValue::Construct_Float(Token* token, int bits, std::string lexeme, bool global)
{
	if (32 != bits && 64 != bits)
	{
		Error(token, "Invalid number of bits in Float constructor.");
		return NullInvalid();
	}

	llvm::Type* defty = nullptr;
	if (32 == bits) defty = m_builder->getFloatTy();
	else if (64 == bits) defty = m_builder->getDoubleTy();

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
		ret.m_type = LITERAL_TYPE_FLOAT;
		ret.m_bits = bits;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		ret.m_is_storage = true;
		return ret;
	}

	return NullInvalid();
}


TValue TValue::Construct_Bool(Token* token, std::string lexeme, bool global)
{
	llvm::Type* defty = m_builder->getInt1Ty();
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
		ret.m_type = LITERAL_TYPE_BOOL;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		ret.m_is_storage = true;
		return ret;
	}

	return NullInvalid();
}


TValue TValue::Construct_Enum(Token* token, std::string lexeme, bool global)
{
	llvm::Type* defty = m_builder->getInt32Ty();
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
		ret.m_type = LITERAL_TYPE_ENUM;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		ret.m_is_storage = true;
		return ret;
	}

	return NullInvalid();
}


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


TValue TValue::Construct_String(Token* token, std::string lexeme, bool global)
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
		ret.m_type = LITERAL_TYPE_STRING;
		ret.m_value = defval;
		ret.m_ty = defty;
		ret.m_token = token;
		ret.m_is_storage = true;
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_void"), { }, "calltmp");
		m_builder->CreateStore(s, defval);
		Environment::AddToCleanup(ret);
		return ret;
	}

	return NullInvalid();
}


TValue TValue::Construct_Vec_Dynamic(Token* token, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs)
{
	TValue ret;

	TokenTypeEnum vtype = args->at(0)->GetType();

	llvm::Type* ty = nullptr;

	switch (vtype)
	{
	case TOKEN_VAR_I16:
		ret.m_bits = 16;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_INTEGER;
		break;

	case TOKEN_VAR_I32:
		ret.m_bits = 32;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_INTEGER;
		break;

	case TOKEN_VAR_I64:
		ret.m_bits = 64;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_INTEGER;
		break;

	case TOKEN_VAR_F32:
		ret.m_bits = 32;
		ty = m_builder->getFloatTy();
		ret.m_vec_type = LITERAL_TYPE_FLOAT;
		break;

	case TOKEN_VAR_F64:
		ret.m_bits = 64;
		ty = m_builder->getDoubleTy();
		ret.m_vec_type = LITERAL_TYPE_FLOAT;
		break;

	case TOKEN_VAR_ENUM:
		ret.m_bits = 32;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_ENUM;
		break;

	case TOKEN_VAR_BOOL:
		ret.m_bits = 1;
		ty = m_builder->getIntNTy(1);
		ret.m_vec_type = LITERAL_TYPE_BOOL;
		break;

	case TOKEN_VAR_STRING:
		ret.m_bits = 64;
		ty = m_builder->getPtrTy();
		ret.m_vec_type = LITERAL_TYPE_STRING;
		break;

	default:
		Error(token, "Invalid vector type.");
		return NullInvalid();
	}

	if (ty)
	{
		llvm::Value* defval = nullptr;
		llvm::Type* defty = m_builder->getPtrTy();
		llvm::Constant* init = llvm::Constant::getNullValue(defty);

		// allocate space for the pointer to this vector
		if (global)
		{
			defval = new llvm::GlobalVariable(*m_module, defty, false, llvm::GlobalValue::InternalLinkage, init, "global_vec_tmp");
			if (!defval) Error(token, "Failed to create global variable.");
		}
		else
		{
			defval = CreateEntryAlloca(m_builder, defty, nullptr, "alloc_vec_tmp");
			m_builder->CreateStore(init, defval);
		}

		if (defval)
		{
			// get a pointer to an empty vector
			llvm::Value* vtype = m_builder->getInt64(ret.m_vec_type);
			int nbytes = std::max(1, ret.m_bits / 8);
			llvm::Value* span = m_builder->getInt64(nbytes);
			llvm::Value* ptr = m_builder->CreateCall(m_module->getFunction("__new_dyn_vec"), { vtype, span }, "calltmp");
			ret.m_is_storage = false;
			ret.m_value = defval;
			ret.m_ty = ty;
			ret.m_token = token;
			ret.m_type = LITERAL_TYPE_VEC_DYNAMIC;
			m_builder->CreateStore(ptr, defval);
			Environment::AddToCleanup(ret);
			return ret;
		}
	}

	return NullInvalid();
}

TValue TValue::Construct_Vec_Fixed(Token* token, TokenPtrList* args, std::string lexeme, bool global, TValueList* targs)
{
	TValue ret;

	TokenTypeEnum vtype = args->at(0)->GetType();
	int vsize = args->at(1)->IntValue();

	if (1 > vsize)
	{
		Error(token, "Vector size must be > 0.");
		return NullInvalid();
	}
	else if (targs && targs->size() != vsize)
	{
		Error(token, "Not enough default values passed to vector constructor.");
		return NullInvalid();
	}

	llvm::Value* count = m_builder->getInt64(vsize);

	llvm::Type* ty = nullptr;
	switch (vtype)
	{
	case TOKEN_VAR_I16:
		ret.m_bits = 16;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_INTEGER;
		break;

	case TOKEN_VAR_I32:
		ret.m_bits = 32;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_INTEGER;
		break;

	case TOKEN_VAR_I64:
		ret.m_bits = 64;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_INTEGER;
		break;

	case TOKEN_VAR_F32:
		ret.m_bits = 32;
		ty = m_builder->getFloatTy();
		ret.m_vec_type = LITERAL_TYPE_FLOAT;
		break;

	case TOKEN_VAR_F64:
		ret.m_bits = 64;
		ty = m_builder->getDoubleTy();
		ret.m_vec_type = LITERAL_TYPE_FLOAT;
		break;

	case TOKEN_VAR_ENUM:
		ret.m_bits = 32;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_ENUM;
		break;

	case TOKEN_VAR_BOOL:
		ret.m_bits = 1;
		ty = m_builder->getIntNTy(ret.m_bits);
		ret.m_vec_type = LITERAL_TYPE_BOOL;
		break;

	case TOKEN_VAR_STRING:
		ret.m_bits = 64;
		ty = m_builder->getPtrTy();
		ret.m_vec_type = LITERAL_TYPE_STRING;
		break;

	default:
		Error(token, "Invalid vector type.");
		return NullInvalid();
	}

	if (ty)
	{
		llvm::Value* defval = nullptr;
		llvm::ArrayType* defty = llvm::ArrayType::get(ty, vsize);
		llvm::Constant* init = llvm::Constant::getNullValue(defty);

		if (global)
		{
			defval = new llvm::GlobalVariable(*m_module, defty, false, llvm::GlobalValue::InternalLinkage, init, "global_vec_tmp");
			if (!defval) Error(token, "Failed to create global variable.");
		}
		else
		{
			defval = CreateEntryAlloca(m_builder, ty, count, "alloc_vec_tmp");
			m_builder->CreateStore(init, defval);
		}

		if (defval)
		{
			ret.m_value = defval;
			ret.m_is_storage = false;
			ret.m_type = LITERAL_TYPE_VEC_FIXED;
			ret.m_ty = ty;
			ret.m_token = token;
			ret.m_fixed_vec_len = vsize;

			if (targs)
			{
				// initializer list provided
				for (size_t i = 0; i < targs->size(); ++i)
				{
					llvm::Value* gep = m_builder->CreateGEP(ty, defval, m_builder->getInt32(i), "geptmp");
					m_builder->CreateStore(targs->at(i).m_value, gep);
				}
			}
			else
			{
				// default construction for vector of strings
				if (LITERAL_TYPE_STRING == ret.m_vec_type)
				{
					// initialize strings
					for (size_t i = 0; i < vsize; ++i)
					{
						llvm::Value* gep = m_builder->CreateGEP(ty, defval, m_builder->getInt32(i), "geptmp");
						llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__new_string_void"), {}, "calltmp");
						m_builder->CreateStore(s, gep);
						// to do - figure out how to clean up this string allocation
					}
				}
			}
			return ret;
		}
	}

	return NullInvalid();
}


TValue TValue::Construct_Null(Token* token, LiteralTypeEnum type, int bits)
{
	TValue ret;
	ret.m_token = token;
	ret.m_type = type;
	ret.m_bits = bits;
	ret.m_ty = MakeTy(type, bits);
	ret.m_value = nullptr;

	if (!ret.m_ty)
	{
		Error(token, "Invalid type in null constructor.");
		return NullInvalid();
	}

	if (LITERAL_TYPE_INVALID != type) ret.m_value = llvm::Constant::getNullValue(ret.m_ty);

	return ret;
}


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


TValue TValue::MakeBool(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_type = LITERAL_TYPE_BOOL;
	ret.m_ty = m_builder->getInt1Ty();
	ret.m_token = token;
	ret.m_value = value;
	ret.m_bits = 1;
	ret.m_is_storage = false;
	return ret;
}

TValue TValue::MakeInt32(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_type = LITERAL_TYPE_INTEGER;
	ret.m_ty = m_builder->getInt32Ty();
	ret.m_token = token;
	ret.m_value = value;
	ret.m_bits = 32;
	ret.m_is_storage = false;
	return ret;
}

TValue TValue::MakeInt64(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_type = LITERAL_TYPE_INTEGER;
	ret.m_ty = m_builder->getInt64Ty();
	ret.m_token = token;
	ret.m_value = value;
	ret.m_bits = 64;
	ret.m_is_storage = false;
	return ret;
}

TValue TValue::MakeFloat64(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_type = LITERAL_TYPE_FLOAT;
	ret.m_ty = m_builder->getDoubleTy();
	ret.m_token = token;
	ret.m_value = value;
	ret.m_bits = 64;
	ret.m_is_storage = false;
	return ret;
}

TValue TValue::MakeString(Token* token, llvm::Value* value)
{
	TValue ret;
	ret.m_type = LITERAL_TYPE_STRING;
	ret.m_ty = m_builder->getPtrTy();
	ret.m_token = token;
	ret.m_bits = 64;
	llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
	m_builder->CreateStore(value, a);
	ret.m_value = a;
	ret.m_is_storage = true;
	Environment::AddToCleanup(ret);
	return ret;
}

TValue TValue::MakeDynVec(Token* token, llvm::Value* value, LiteralTypeEnum vec_type, int vec_bits)
{
	TValue ret;
	ret.m_type = LITERAL_TYPE_VEC_DYNAMIC;
	ret.m_ty = MakeTy(vec_type, vec_bits);
	ret.m_token = token;
	ret.m_bits = vec_bits;
	llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
	m_builder->CreateStore(value, a);
	ret.m_value = a;
	ret.m_is_storage = false;
	ret.m_vec_type = vec_type;
	Environment::AddToCleanup(ret);
	return ret;
}


llvm::Type* TValue::MakeTy(LiteralTypeEnum type, int bits)
{
	if (LITERAL_TYPE_INTEGER == type)          return m_builder->getIntNTy(bits);
	else if (LITERAL_TYPE_FLOAT == type)
	{
		if (32 == bits)                        return m_builder->getFloatTy();
		else if (64 == bits)                   return m_builder->getDoubleTy();
	}
	else if (LITERAL_TYPE_ENUM == type)        return m_builder->getInt32Ty();
	else if (LITERAL_TYPE_BOOL == type)        return m_builder->getInt1Ty();
	else if (LITERAL_TYPE_STRING == type)      return m_builder->getPtrTy();
	else if (LITERAL_TYPE_VEC_DYNAMIC == type) return m_builder->getPtrTy();
	else if (LITERAL_TYPE_POINTER == type)     return m_builder->getPtrTy();
	else if (LITERAL_TYPE_INVALID == type)     return m_builder->getVoidTy();

	return nullptr;
}

