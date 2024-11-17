#include "TValue.h"
#include "Environment.h"


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


TValue TValue::AsInt(int bits)
{
	if (16 != bits && 32 != bits && 64 != bits)
	{
		Error(m_token, "Invalid bitsize in cast operation.");
		return NullInvalid();
	}

	TValue lhs = GetFromStorage();

	if (LITERAL_TYPE_INTEGER == m_type)
	{
		if (m_bits == bits) return lhs;
		return lhs.CastToInt(bits);
	}
	else if (LITERAL_TYPE_FLOAT == m_type)
	{
		return lhs.CastToInt(bits);
	}
	else if (LITERAL_TYPE_STRING == m_type)
	{
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__str_to_int"), { lhs.m_value }, "calltmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_INTEGER, v);
		ret.m_ty = v->getType();
		ret.m_bits = 64;
		ret.m_is_storage = false;
		return ret.CastToInt(bits);
	}

	Error(m_token, "Invalid cast operation.");
	return NullInvalid();

	/*

		else if (lhs.IsBool())
		{
			return TValue::Integer(builder->CreateIntCast(lhs.value, builder->getInt32Ty(), false, "int_cast_tmp"));
		}
		*/
}

TValue TValue::AsFloat(int bits)
{
	if (32 != bits && 64 != bits)
	{
		Error(m_token, "Invalid bitsize in cast operation.");
		return NullInvalid();
	}

	TValue lhs = GetFromStorage();

	if (LITERAL_TYPE_FLOAT == m_type)
	{
		if (m_bits == bits) return lhs;
		return lhs.CastToFloat(bits);
	}
	else if (LITERAL_TYPE_INTEGER == m_type)
	{
		return lhs.CastToFloat(bits);
	}
	else if (LITERAL_TYPE_STRING == m_type)
	{
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__str_to_double"), { lhs.m_value }, "calltmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_FLOAT, v);
		ret.m_ty = v->getType();
		ret.m_bits = 64;
		ret.m_is_storage = false;
		return ret.CastToFloat(bits);
	}

	Error(m_token, "Invalid cast operation.");
	return NullInvalid();
}


TValue TValue::AsString()
{
	if (LITERAL_TYPE_STRING == m_type) return *this; // clone

	TValue lhs = GetFromStorage();

	switch (m_type)
	{
	case LITERAL_TYPE_INTEGER:
	{
		lhs = lhs.CastToInt(64); // upcast
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_STRING, a);
		ret.m_ty = m_builder->getPtrTy();
		ret.m_is_storage = true;
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__int_to_string"), { lhs.m_value }, "calltmp");
		m_builder->CreateStore(s, ret.m_value);
		Environment::AddToCleanup(ret);
		return ret;
	}
	case LITERAL_TYPE_FLOAT:
	{
		lhs = lhs.CastToFloat(64); // upcast
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_STRING, a);
		ret.m_ty = m_builder->getPtrTy();
		ret.m_is_storage = true;
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__float_to_string"), { lhs.m_value }, "calltmp");
		m_builder->CreateStore(s, ret.m_value);
		Environment::AddToCleanup(ret);
		return ret;
	}
	case LITERAL_TYPE_BOOL:
	{
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_STRING, a);
		ret.m_ty = m_builder->getPtrTy();
		ret.m_is_storage = true;
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__bool_to_string"), { lhs.m_value }, "calltmp");
		m_builder->CreateStore(s, ret.m_value);
		Environment::AddToCleanup(ret);
		return ret;
	}
	case LITERAL_TYPE_VEC_DYNAMIC:
	{
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_STRING, a);
		ret.m_ty = m_builder->getPtrTy();
		ret.m_is_storage = true;

		llvm::Value* srcPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__dyn_vec_to_string"), { srcPtr }, "calltmp");
		m_builder->CreateStore(s, ret.m_value);
		Environment::AddToCleanup(ret);
		return ret;
	}
	case LITERAL_TYPE_VEC_FIXED:
	{
		llvm::Value* a = CreateEntryAlloca(m_builder, m_builder->getPtrTy(), nullptr, "alloctmp");
		TValue ret = TValue(m_token, LITERAL_TYPE_STRING, a);
		ret.m_ty = m_builder->getPtrTy();
		ret.m_is_storage = true;
		llvm::Value* srcType = m_builder->getInt32(lhs.m_vec_type);
		llvm::Value* srcBits = m_builder->getInt32(lhs.m_bits);
		llvm::Value* srcLen = m_builder->getInt64(lhs.m_fixed_vec_len);
		llvm::Value* srcPtr = m_value;
		if (LITERAL_TYPE_STRING == lhs.m_vec_type)
		{
			srcPtr = m_builder->CreateGEP(m_builder->getPtrTy(), m_value, m_builder->getInt32(0), "geptmp");
		}
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__fixed_vec_to_string"), { srcType, srcBits, srcPtr, srcLen }, "calltmp");
		m_builder->CreateStore(s, ret.m_value);
		Environment::AddToCleanup(ret);
		return ret;
	}
	default:
		Error(m_token, "Failed to cast to string.");
	}

	return NullInvalid();
}


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

	if (LITERAL_TYPE_FLOAT == m_type)
	{
		if (bits == m_bits) return *this; // clone
		llvm::Value* defval = m_builder->CreateFPCast(m_value, defty, "fp_cast");
		TValue ret = *this;
		ret.m_bits = bits;
		ret.m_value = defval;
		ret.m_ty = defty;
		return ret;
	}
	else if (LITERAL_TYPE_INTEGER == m_type)
	{
		llvm::Value* defval = m_builder->CreateSIToFP(m_value, defty, "int_to_fp");
		TValue ret;
		ret.m_type = LITERAL_TYPE_FLOAT;
		ret.m_token = m_token;
		ret.m_bits = bits;
		ret.m_value = defval;
		ret.m_ty = defty;
		return ret;
	}

	Error(m_token, "Failed to cast to float.");
	return NullInvalid();
}


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

	if (LITERAL_TYPE_INTEGER == m_type)
	{
		if (bits == m_bits) return *this; // clone
		llvm::Value* defval = m_builder->CreateIntCast(m_value, defty, true, "int_cast");
		TValue ret = *this;
		ret.m_bits = bits;
		ret.m_value = defval;
		ret.m_ty = defty;
		return ret;
	}
	else if (LITERAL_TYPE_FLOAT == m_type)
	{
		llvm::Value* defval = m_builder->CreateFPToSI(m_value, defty, "fp_to_int");
		TValue ret;
		ret.m_type = LITERAL_TYPE_INTEGER;
		ret.m_bits = bits;
		ret.m_value = defval;
		ret.m_token = m_token;
		ret.m_ty = defty;
		return ret;
	}

	Error(m_token, "Failed to cast to int.");
	return NullInvalid();
}

void TValue::CastToMaxBits(TValue& lhs, TValue& rhs)
{
	if (lhs.m_type != rhs.m_type)
	{
		Error(lhs.m_token, "Failed to cast to max bitsize.");
	}

	if (LITERAL_TYPE_INTEGER == lhs.m_type)
	{
		if (lhs.m_bits < rhs.m_bits) lhs = lhs.CastToInt(rhs.m_bits);
		if (rhs.m_bits < lhs.m_bits) rhs = rhs.CastToInt(lhs.m_bits);
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type)
	{
		if (lhs.m_bits < rhs.m_bits) lhs = lhs.CastToFloat(rhs.m_bits);
		if (rhs.m_bits < lhs.m_bits) rhs = rhs.CastToFloat(lhs.m_bits);
	}
}

TValue TValue::CastToMatchImplicit(TValue src)
{
	LiteralTypeEnum srcType = src.m_type;
	if (src.IsVecAny()) srcType = src.m_vec_type;

	// types already match, check bitsize
	if (srcType == m_type)
	{
		if (LITERAL_TYPE_INTEGER == m_type)
			return CastToInt(src.m_bits);
		else if (m_type == LITERAL_TYPE_FLOAT)
			return CastToFloat(src.m_bits);
		else
			return *this; // clone
	}

	if (LITERAL_TYPE_INTEGER == m_type)
	{
		if (LITERAL_TYPE_FLOAT == srcType) return CastToFloat(src.m_bits);
	}
	else if (LITERAL_TYPE_FLOAT == m_type)
	{
		if (LITERAL_TYPE_INTEGER == srcType) return CastToInt(src.m_bits);
	}

	Error(m_token, "Unable to implicit cast between types.");
	return NullInvalid();
}