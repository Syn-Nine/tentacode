#include "TType.h"
#include "TFunction.h"

#include "Environment.h"

llvm::IRBuilder<>* TType::m_builder = nullptr;
llvm::Module* TType::m_module = nullptr;

TType TType::Construct(Token* token, TokenPtrList* args)
{
	if (!token) return TType(); // invalid

	// token to type
	TokenTypeEnum token_type = token->GetType();

	switch (token_type)
	{
	case TOKEN_VAR_I16:
		return Construct_Int(token, 16);

	case TOKEN_VAR_I32:
		return Construct_Int(token, 32);

	case TOKEN_VAR_I64:
		return Construct_Int(token, 64);

	case TOKEN_VAR_F32:
		return Construct_Float(token, 32);

	case TOKEN_VAR_F64:
		return Construct_Float(token, 64);

	case TOKEN_VAR_ENUM:
		return Construct_Enum(token);

	case TOKEN_VAR_BOOL:
		return Construct_Bool(token);

	case TOKEN_VAR_STRING:
		return Construct_String(token);

	case TOKEN_IDENTIFIER:
		return Construct_Struct(token);

	case TOKEN_VAR_MAP:
		return Construct_Map(token, args);

	case TOKEN_VAR_SET:
		return Construct_Set(token, args);

	case TOKEN_VAR_TUPLE:
		return Construct_Tuple(token, args);

	case TOKEN_VAR_VEC:
		return Construct_Vec(token, args);

	case TOKEN_AT:
		return Construct_Functor(token);

	
	case TOKEN_VAR_IMAGE: // intentional fallthrough
	case TOKEN_VAR_TEXTURE:
	case TOKEN_VAR_FONT:
	case TOKEN_VAR_SOUND:
	case TOKEN_VAR_SHADER:
	case TOKEN_VAR_RENDER_TEXTURE_2D:
		return Construct_Ptr(token);

	default:
		Environment::Error(token, "Unable to convert token to type.");
	}

	return TType();
}



//-----------------------------------------------------------------------------
TType TType::Construct_Bool(Token* token)
{
	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 1;
	ret.m_literal_type = LITERAL_TYPE_BOOL;
	ret.m_lexeme = "bool";
	ret.m_ty = m_builder->getIntNTy(1);
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Brace(Token* token)
{
	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 0;
	ret.m_lexeme = "brace";
	ret.m_literal_type = LITERAL_TYPE_BRACE;
	ret.m_ty = nullptr;
	ret.m_ty_sz_bytes = 0;

	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Enum(Token* token)
{
	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 32;
	ret.m_literal_type = LITERAL_TYPE_ENUM;
	ret.m_lexeme = "enum";
	ret.m_ty = m_builder->getIntNTy(32);
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Float(Token* token, int bits)
{
	TType ret;
	if (32 != bits && 64 != bits)
	{
		Environment::Error(token, "Invalid bitsize for float type.");
		return ret;
	}

	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = bits;
	ret.m_literal_type = LITERAL_TYPE_FLOAT;
	if (32 == bits)
	{
		ret.m_lexeme = "f32";
		ret.m_ty = m_builder->getFloatTy();
	}
	else if (64 == bits)
	{
		ret.m_lexeme = "f64";
		ret.m_ty = m_builder->getDoubleTy();
	}
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);

	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Functor(Token* token)
{
	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_literal_type = LITERAL_TYPE_FUNCTOR;
	ret.m_lexeme = "functor";
	ret.m_ty = m_builder->getPtrTy();
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Int(Token* token, int bits)
{
	TType ret;
	if (16 != bits && 32 != bits && 64 != bits)
	{
		Environment::Error(token, "Invalid bitsize for integer type.");
		return ret;
	}

	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = bits;
	ret.m_literal_type = LITERAL_TYPE_INTEGER;
	ret.m_ty = m_builder->getIntNTy(ret.m_bits);

	if (16 == bits) ret.m_lexeme = "i16";
	else if (32 == bits) ret.m_lexeme = "i32";
	else if (64 == bits) ret.m_lexeme = "i64";

	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);

	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_String(Token* token)
{
	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_literal_type = LITERAL_TYPE_STRING;
	ret.m_ty = m_builder->getPtrTy();
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	ret.m_lexeme = "string";
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Map(Token* token, TokenPtrList* args)
{
	TType ret;
	if (2 != args->size()) return ret;

	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_ty = m_builder->getPtrTy();
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	ret.m_lexeme = "map";
	ret.m_literal_type = LITERAL_TYPE_MAP;
	ret.m_inner_types.push_back(TType::Construct(args->at(0), nullptr));
	ret.m_inner_types.push_back(TType::Construct(args->at(1), nullptr));

	// type checking
	TType t0 = ret.m_inner_types[0];
	if (!t0.IsInteger() && !t0.IsString() && !t0.IsEnum())
	{
		Environment::Error(token, "Map index must be enum, integer, or string type.");
		return TType();
	}

	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Set(Token* token, TokenPtrList* args)
{
	TType ret;
	if (1 != args->size()) return ret;

	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_ty = m_builder->getPtrTy();
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	ret.m_lexeme = "set";
	ret.m_literal_type = LITERAL_TYPE_SET;
	ret.m_inner_types.push_back(TType::Construct(args->at(0), nullptr));

	// type checking
	TType t0 = ret.m_inner_types[0];
	if (!t0.IsInteger() && !t0.IsString() && !t0.IsEnum())
	{
		Environment::Error(token, "Set must be enum, integer, or string type.");
		return TType();
	}

	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Tuple(Token* token, TokenPtrList* args)
{
	TType ret;
	if (args->empty()) return ret;

	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_lexeme = "tuple";
	ret.m_literal_type = LITERAL_TYPE_TUPLE;

	std::vector<llvm::Type*> arg_ty;
	for (size_t i = 0; i < args->size(); ++i)
	{
		TType tt = TType::Construct(args->at(i), nullptr);
		ret.m_inner_types.push_back(tt);
		arg_ty.push_back(tt.GetTy());
	}

	ret.m_ty = llvm::StructType::create(m_module->getContext(), arg_ty, "tuple");
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);

	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Vec(Token* token, TokenPtrList* args)
{
	TType ret;
	if (2 != args->size()) return ret;

	TType i0 = TType::Construct(args->at(0), nullptr);
	if (!i0.IsValid()) return TType();

	ret.m_inner_types.push_back(i0);

	if (!args->at(1))
	{
		ret.m_ty = m_builder->getPtrTy();
		ret.m_literal_type = LITERAL_TYPE_VEC_DYNAMIC;
	}
	else
	{
		int vsize = 0;
		Token* vtoken = args->at(1);

		if (TOKEN_INTEGER == vtoken->GetType())
		{
			vsize = vtoken->IntValue();
		}
		else if (TOKEN_IDENTIFIER == vtoken->GetType())
		{
			TValue vv = Environment::GetVariable(vtoken, vtoken->Lexeme());//.GetFromStorage();
			if (!vv.IsInteger())
			{
				Environment::Error(vtoken, "Size variable must be an integer.");
				return TType();
			}
			if (!vv.IsConstant())
			{
				Environment::Error(vtoken, "Size variable must be constant.");
				return TType();
			}

			llvm::Value* v = vv.Value();
			if (llvm::isa<llvm::GlobalValue>(v))
			{
				llvm::GlobalVariable* gv = llvm::cast<llvm::GlobalVariable>(v);
				if (gv)
				{
					llvm::Constant* c = gv->getInitializer();
					if (c)
					{
						llvm::ConstantInt* ci = llvm::cast<llvm::ConstantInt>(c);
						if (ci) vsize = ci->getSExtValue();
					}
				}
			}
		}

		if (1 > vsize)
		{
			Environment::Error(vtoken, "Vector size must be > 0.");
		}

		ret.m_ty = llvm::ArrayType::get(ret.m_inner_types[0].GetTy(), vsize);
		ret.m_literal_type = LITERAL_TYPE_VEC_FIXED;
		ret.m_vec_sz = vsize;
	}

	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_lexeme = "vec";
	
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Ptr(Token* token)
{
	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_literal_type = LITERAL_TYPE_POINTER;
	ret.m_lexeme = "ptr";
	ret.m_ty = m_builder->getPtrTy();
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::Construct_Struct(Token* token)
{
	TStruct tstruc = Environment::GetStruct(token, token->Lexeme());
	if (!tstruc.IsValid()) return TType();

	TType ret;
	ret.m_token = token;
	ret.m_is_valid = true;
	ret.m_bits = 64;
	ret.m_literal_type = LITERAL_TYPE_UDT;
	ret.m_lexeme = tstruc.GetFQNS() + ":" + token->Lexeme();
	ret.m_ty = tstruc.GetLLVMStruct();
	ret.m_ty_sz_bytes = llvm::ConstantExpr::getSizeOf(ret.m_ty);
	return ret;
}


//-----------------------------------------------------------------------------
TType TType::GetInternal(size_t idx)
{
	if (idx < m_inner_types.size()) return m_inner_types[idx];
	Environment::Error(m_token, "Attempting to get non-existant inner type.");
	return TType();
}


//-----------------------------------------------------------------------------
bool TType::CanCast(TType val)
{
	LiteralTypeEnum lt0 = GetLiteralType();
	LiteralTypeEnum lt1 = val.GetLiteralType();

	if (LITERAL_TYPE_INTEGER == lt0 || LITERAL_TYPE_FLOAT == lt0)
	{
		if (LITERAL_TYPE_INTEGER == lt1 || LITERAL_TYPE_FLOAT == lt1) return true;
	}
	if (lt0 == lt1)
	{
		if (LITERAL_TYPE_BOOL == lt1 || LITERAL_TYPE_ENUM == lt1 || LITERAL_TYPE_STRING == lt1) return true;
	}
	
	if (IsVecAny() && val.IsVecAny()) return GetInternal(0).CanCast(val.GetInternal(0));
	if (IsSet() && val.IsSet()) return GetInternal(0).CanCast(val.GetInternal(0));
	
	if (IsUDT() && val.IsUDT()) return GetTy() == val.GetTy();
	
	if (IsTuple() && val.IsTuple() && GetInternalCount() == val.GetInternalCount())
	{
		for (size_t i = 0; i < GetInternalCount(); ++i)
		{
			// if using tuples internal types must be identical for now
			if (!GetInternal(i).IsTypeMatched(val.GetInternal(i))) return false;
		}
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
bool TType::IsTypeMatched(TType val)
{
	bool ret = GetLiteralType() == val.GetLiteralType();
	ret = ret && GetInternalCount() == val.GetInternalCount();
	if (!IsTuple()) ret = ret && GetTy() == val.GetTy();

	// check for internal type equivalent for vectors
	if (ret && IsVecAny())
	{
		ret = ret && GetInternal(0).IsTypeMatched(val.GetInternal(0));
	}

	// check for type equivalence of tuples
	if (ret && IsTuple() && GetInternalCount() == val.GetInternalCount())
	{
		for (size_t i = 0; i < GetInternalCount(); ++i)
		{
			if (!GetInternal(i).IsTypeMatched(val.GetInternal(i))) return false;
		}
	}

	return ret;
}
