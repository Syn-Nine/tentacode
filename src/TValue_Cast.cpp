#include "TValue.h"
#include "Environment.h"


//-----------------------------------------------------------------------------
TValue TValue::As(TokenTypeEnum newType)
{
	switch (newType)
	{
	case TOKEN_VAR_I16:    return AsInt(16);
	case TOKEN_VAR_I32:    return AsInt(32);
	case TOKEN_VAR_I64:    return AsInt(64);
	case TOKEN_VAR_F32:    return AsFloat(32);
	case TOKEN_VAR_F64:    return AsFloat(64);
	case TOKEN_VAR_STRING: return AsString();
	default:
		Error(m_token, "Invalid cast operation.");
	}

	return NullInvalid();
}



//-----------------------------------------------------------------------------
TValue TValue::AsInt(int bits)
{
	if (16 != bits && 32 != bits && 64 != bits)
	{
		Error(m_token, "Invalid bitsize in cast operation.");
		return NullInvalid();
	}

	TValue lhs = GetFromStorage();
	LiteralTypeEnum type = lhs.GetLiteralType();

	if (LITERAL_TYPE_INTEGER == type)
	{
		if (NumBits() == bits) return lhs;
		return lhs.CastToInt(bits);
	}
	else if (LITERAL_TYPE_FLOAT == type)
	{
		return lhs.CastToInt(bits);
	}
	else if (LITERAL_TYPE_BOOL == type)
	{
		return lhs.CastToInt(bits);
	}
	else if (LITERAL_TYPE_STRING == type)
	{
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__str_to_int"), { lhs.m_value }, "calltmp");
		return MakeInt(m_token, 64, v).CastToInt(bits);
	}

	Error(m_token, "Invalid cast operation.");
	return NullInvalid();

}


//-----------------------------------------------------------------------------
TValue TValue::AsFloat(int bits)
{
	if (32 != bits && 64 != bits)
	{
		Error(m_token, "Invalid bitsize in cast operation.");
		return NullInvalid();
	}

	TValue lhs = GetFromStorage();
	LiteralTypeEnum type = lhs.GetLiteralType();

	if (LITERAL_TYPE_FLOAT == type)
	{
		if (NumBits() == bits) return lhs;
		return lhs.CastToFloat(bits);
	}
	else if (LITERAL_TYPE_INTEGER == type)
	{
		return lhs.CastToFloat(bits);
	}
	else if (LITERAL_TYPE_STRING == type)
	{
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__str_to_double"), { lhs.m_value }, "calltmp");
		return MakeFloat(m_token, 64, v).CastToFloat(bits);
	}

	Error(m_token, "Invalid cast operation.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::AsString()
{
	if (IsString()) return *this; // clone

	TValue lhs = GetFromStorage();

	llvm::Value* s = nullptr;

	switch (GetLiteralType())
	{
	case LITERAL_TYPE_INTEGER:
	{
		lhs = lhs.CastToInt(64); // upcast
		s = m_builder->CreateCall(m_module->getFunction("__int_to_string"), { lhs.m_value }, "calltmp");
		break;
	}
	case LITERAL_TYPE_FLOAT:
	{
		lhs = lhs.CastToFloat(64); // upcast
		s = m_builder->CreateCall(m_module->getFunction("__float_to_string"), { lhs.m_value }, "calltmp");
		break;
	}
	case LITERAL_TYPE_BOOL:
	{
		s = m_builder->CreateCall(m_module->getFunction("__bool_to_string"), { lhs.m_value }, "calltmp");
		break;
	}
	case LITERAL_TYPE_VEC_DYNAMIC:
	{
		llvm::Value* srcPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		s = m_builder->CreateCall(m_module->getFunction("__dyn_vec_to_string"), { srcPtr }, "calltmp");
		break;
	}
	case LITERAL_TYPE_VEC_FIXED:
	{
		LiteralTypeEnum vectype = lhs.m_ttype.GetInternal(0).GetLiteralType();
		llvm::Value* srcType = m_builder->getInt32(vectype);
		llvm::Value* srcBits = m_builder->getInt32(lhs.m_ttype.GetInternal(0).NumBits());
		llvm::Value* srcLen = m_builder->getInt64(lhs.m_ttype.GetFixedVecLen());
		llvm::Value* srcPtr = m_value;
		if (LITERAL_TYPE_STRING == vectype)
		{
			srcPtr = m_builder->CreateGEP(m_builder->getPtrTy(), m_value, m_builder->getInt32(0), "geptmp");
		}
		s = m_builder->CreateCall(m_module->getFunction("__fixed_vec_to_string"), { srcType, srcBits, srcPtr, srcLen }, "calltmp");
		break;
	}
	default:
		Error(m_token, "Failed to cast to string.");
		return NullInvalid();
	}

	return MakeString(m_token, s);
}


//-----------------------------------------------------------------------------
TValue TValue::CastToFloat(int bits)
{
	llvm::Type* defty = nullptr;
	if (32 == bits) defty = m_builder->getFloatTy();
	else if (64 == bits) defty = m_builder->getDoubleTy();

	if (!defty)
	{
		Error(m_token, "Invalid bit size for float.");
		return NullInvalid();
	}

	if (m_is_storage)
	{
		Error(m_token, "Attempting to cast from storage.");
		return NullInvalid();
	}

	if (IsFloat())
	{
		if (bits == NumBits()) return *this; // clone
		llvm::Value* defval = m_builder->CreateFPCast(m_value, defty, "fp_cast");
		return MakeFloat(m_token, bits, defval);
	}
	else if (IsInteger())
	{
		llvm::Value* defval = m_builder->CreateSIToFP(m_value, defty, "int_to_fp");
		return MakeFloat(m_token, bits, defval);
	}

	Error(m_token, "Failed to cast to float.");
	return NullInvalid();
}



//-----------------------------------------------------------------------------
TValue TValue::CastToInt(int bits)
{
	llvm::Type* defty = nullptr;
	if (16 == bits || 32 == bits || 64 == bits) defty = m_builder->getIntNTy(bits);

	if (!defty)
	{
		Error(m_token, "Invalid bit size for int.");
		return NullInvalid();
	}

	if (m_is_storage)
	{
		Error(m_token, "Attempting to cast from storage.");
		return NullInvalid();
	}

	if (IsInteger())
	{
		if (bits == NumBits()) return *this; // clone
		llvm::Value* defval = m_builder->CreateIntCast(m_value, defty, true, "int_cast");
		return MakeInt(m_token, bits, defval);
	}
	else if (IsBool())
	{
		llvm::Value* defval = m_builder->CreateIntCast(m_value, defty, true, "int_cast");
		return MakeInt(m_token, bits, defval);
	}
	else if (IsFloat())
	{
		llvm::Value* defval = m_builder->CreateFPToSI(m_value, defty, "fp_to_int");
		return MakeInt(m_token, bits, defval);
	}

	Error(m_token, "Failed to cast to int.");
	return NullInvalid();
}



//-----------------------------------------------------------------------------
TValue TValue::CastToMatchImplicit(TValue src)
{
	return CastToMatchImplicit(src.GetTType());
}

//-----------------------------------------------------------------------------
TValue TValue::CastToMatchImplicit(TType src)
{
	LiteralTypeEnum dstType = GetLiteralType();
	LiteralTypeEnum srcType = src.GetLiteralType();
	int numBits = src.NumBits();
	if (src.IsVecAny())
	{
		srcType = src.GetInternal(0).GetLiteralType();
		numBits = src.GetInternal(0).NumBits();
	}

	// types already match, check bitsize
	if (srcType == dstType)
	{
		if (IsInteger())
			return CastToInt(numBits);
		else if (IsFloat())
			return CastToFloat(numBits);
		else
			return *this; // clone
	}

	if (LITERAL_TYPE_INTEGER == dstType)
	{
		if (LITERAL_TYPE_FLOAT == srcType) return CastToFloat(numBits);
	}
	else if (LITERAL_TYPE_FLOAT == dstType)
	{
		if (LITERAL_TYPE_INTEGER == srcType) return CastToInt(numBits);
	}

	Error(m_token, "Unable to implicit cast between types.");
	return NullInvalid();
}



//-----------------------------------------------------------------------------
void TValue::CastToMaxBits(TValue& lhs, TValue& rhs)
{
	LiteralTypeEnum lhs_type = lhs.GetLiteralType();
	LiteralTypeEnum rhs_type = rhs.GetLiteralType();
	int lhs_bits = lhs.NumBits();
	int rhs_bits = rhs.NumBits();

	if (lhs_type != rhs_type)
	{
		Error(lhs.m_token, "Failed to cast to max bitsize.");
	}

	if (LITERAL_TYPE_INTEGER == lhs_type)
	{
		if (lhs_bits < rhs_bits) lhs = lhs.CastToInt(rhs_bits);
		if (rhs_bits < lhs_bits) rhs = rhs.CastToInt(lhs_bits);
	}
	else if (LITERAL_TYPE_FLOAT == lhs_type)
	{
		if (lhs_bits < rhs_bits) lhs = lhs.CastToFloat(rhs_bits);
		if (rhs_bits < lhs_bits) rhs = rhs.CastToFloat(lhs_bits);
	}
}


/*

//-----------------------------------------------------------------------------
TValue TValue::CastFixedToDynVec()
{
	if (LITERAL_TYPE_VEC_FIXED != m_type)
	{
		Error(m_token, "LHS is not a fixed vector.");
		return NullInvalid();
	}
	else if (m_fixed_vec_len == 0)
	{
		Error(m_token, "Fixed vector is empty.");
		return NullInvalid();
	}
	else if (LITERAL_TYPE_INTEGER != m_vec_type && LITERAL_TYPE_FLOAT != m_vec_type)
	{
		Error(m_token, "Vector must be float or integer.");
		return NullInvalid();
	}

	TValue ret;
	llvm::Value* defval = nullptr;
	llvm::Type* defty = m_builder->getPtrTy();
	defval = CreateEntryAlloca(m_builder, defty, nullptr, "alloc_vec_tmp");
	llvm::Value* vtype = m_builder->getInt64(m_vec_type);
	llvm::Value* span = m_builder->getInt64(4);
	llvm::Value* ptr = m_builder->CreateCall(m_module->getFunction("__new_dyn_vec"), { vtype, span }, "calltmp");
	ret.m_is_storage = false;
	ret.m_value = defval;
	ret.m_ty = m_builder->getInt32Ty();
	ret.m_token = m_token;
	ret.m_type = LITERAL_TYPE_VEC_DYNAMIC;
	ret.m_vec_type = m_vec_type;
	ret.m_bits = 32;
	m_builder->CreateStore(ptr, defval);
	Environment::AddToCleanup(ret);


	llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), ret.m_value, "loadtmp");
	llvm::Value* srcLen = m_builder->getInt64(m_fixed_vec_len);
	m_builder->CreateCall(m_module->getFunction("__dyn_vec_clear_presize"), { dstPtr, srcLen }, "calltmp");

	m_builder->CreateCall(m_module->getFunction("__dyn_vec_assert_idx"), { dstPtr, m_builder->getInt32(m_fixed_vec_len - 1) }, "calltmp");
	llvm::Value* dataPtr = m_builder->CreateCall(m_module->getFunction("__dyn_vec_data_ptr"), { dstPtr }, "calltmp");

	for (size_t i = 0; i < m_fixed_vec_len; ++i)
	{
		llvm::Value* lhs_gep = m_builder->CreateGEP(ret.m_ty, dataPtr, { m_builder->getInt32(i) }, "lhs_geptmp");
		llvm::Value* rhs_gep = m_builder->CreateGEP(m_ty, m_value, { m_builder->getInt32(i) }, "rhs_geptmp");
		llvm::Value* tmp = m_builder->CreateLoad(m_ty, rhs_gep);

		if (m_ty != ret.m_ty)
		{
			if (LITERAL_TYPE_INTEGER == ret.m_vec_type && LITERAL_TYPE_INTEGER == m_vec_type)
			{
				// match bitsize
				tmp = m_builder->CreateIntCast(tmp, ret.m_ty, true, "int_cast");
			}
			else if (LITERAL_TYPE_FLOAT == ret.m_vec_type && LITERAL_TYPE_FLOAT == m_vec_type)
			{
				// match bitsize
				tmp = m_builder->CreateFPCast(tmp, ret.m_ty, "fp_cast");
			}
			else if (LITERAL_TYPE_INTEGER == ret.m_vec_type && LITERAL_TYPE_FLOAT == m_vec_type)
			{
				// lhs is an int, but rhs is a float
				tmp = m_builder->CreateFPToSI(tmp, ret.m_ty, "fp_to_int");
			}
			else if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_FLOAT == ret.m_vec_type)
			{
				// lhs is a float, but rhs is an int
				tmp = m_builder->CreateSIToFP(tmp, ret.m_ty, "int_to_fp");
			}
			else
			{
				Error(m_token, "Failed to cast when storing value.");
				return NullInvalid();
			}
		}

		m_builder->CreateStore(tmp, lhs_gep);
	}

	return ret;
}*/