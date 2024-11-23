#include "TValue.h"
#include "Environment.h"


//-----------------------------------------------------------------------------
TValue TValue::Add(TValue rhs)
{
	TValue lhs = *this;	// clone

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// type specific addition
	if (LITERAL_TYPE_STRING == lhs.m_type && matched)
	{
		llvm::Value* s = m_builder->CreateCall(m_module->getFunction("__string_cat"), { lhs.m_value, rhs.m_value }, "calltmp");
		return MakeString(lhs.m_token, s);
	}
	else if (LITERAL_TYPE_INTEGER == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateAdd(lhs.m_value, rhs.m_value, "addtmp");
			return lhs;
		}
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateFAdd(lhs.m_value, rhs.m_value, "addtmp");
			return lhs;
		}
	}

	Error(m_token, "Failed to add values.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Divide(TValue rhs)
{
	TValue lhs = *this;	// clone

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	if (LITERAL_TYPE_FLOAT == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateFDiv(lhs.m_value, rhs.m_value, "divtmp");
			return lhs;
		}
	}

	Error(m_token, "Failed to divide values.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Modulus(TValue rhs)
{
	TValue lhs = *this;	// clone

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// type specific addition
	if (LITERAL_TYPE_INTEGER == lhs.m_type && matched)
	{
		lhs = lhs.CastToInt(64);
		rhs = rhs.CastToInt(64);

		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateCall(m_module->getFunction("__mod_impl"), { lhs.m_value, rhs.m_value }, "calltmp");
			return lhs;
		}
	}

	Error(m_token, "Failed to take modulus.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Multiply(TValue rhs)
{
	TValue lhs = *this;	// clone

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// type specific addition
	if (LITERAL_TYPE_INTEGER == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateMul(lhs.m_value, rhs.m_value, "multmp");
			return lhs;
		}
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateFMul(lhs.m_value, rhs.m_value, "multmp");
			return lhs;
		}
	}

	Error(m_token, "Failed to multiply values.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Subtract(TValue rhs)
{
	TValue lhs = *this; // clone

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// type specific subtraction
	if (LITERAL_TYPE_INTEGER == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateSub(lhs.m_value, rhs.m_value, "subtmp");
			return lhs;
		}
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type && matched)
	{
		CastToMaxBits(lhs, rhs);
		if (!lhs.IsInvalid() && !rhs.IsInvalid())
		{
			lhs.m_value = m_builder->CreateFSub(lhs.m_value, rhs.m_value, "subtmp");
			return lhs;
		}
	}

	Error(m_token, "Failed to subtract values.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::IsNotEqual(TValue rhs)
{
	TValue lhs = *this;

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// hard type mismatch
	if (!matched)
	{
		Error(m_token, "Type mismatch in comparison.");
		return NullInvalid();
	}

	// type specific comparison
	if (LITERAL_TYPE_INTEGER == lhs.m_type)
	{
		CastToMaxBits(lhs, rhs);

		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpNE(lhs.m_value, rhs.m_value, "cmptmp"));
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type)
	{
		Error(m_token, "Cannot compare inequality of floats.");
		return NullInvalid();
		// upcast to double for subtraction and fabs()
		/*if (lhs.m_bits < 64) lhs = lhs.CastToFloat(64);
		if (rhs.m_bits < 64) rhs = rhs.CastToFloat(64);

		llvm::Value* tmp = m_builder->CreateCall(m_module->getFunction("fabs"), { m_builder->CreateFSub(lhs.m_value, rhs.m_value) }, "calltmp");
		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateFCmpUGT(tmp, llvm::ConstantFP::get(m_module->getContext(), llvm::APFloat(1.0e-12)), "cmptmp"));*/
	}
	else if (LITERAL_TYPE_BOOL == lhs.m_type || LITERAL_TYPE_ENUM == lhs.m_type)
	{
		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpNE(lhs.m_value, rhs.m_value, "cmptmp"));
	}
	else if (LITERAL_TYPE_STRING == lhs.m_type)
	{
		llvm::Value* tmp = m_builder->CreateCall(m_module->getFunction("__str_cmp"), { lhs.m_value, rhs.m_value }, "calltmp");
		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpNE(tmp, m_builder->getTrue(), "boolcmp"));
	}

	Error(m_token, "Failed to create != comparison.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::IsEqual(TValue rhs)
{
	TValue lhs = *this;

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// hard type mismatch
	if (!matched)
	{
		Error(m_token, "Type mismatch in comparison.");
		return NullInvalid();
	}

	// type specific comparison
	if (LITERAL_TYPE_INTEGER == lhs.m_type)
	{
		CastToMaxBits(lhs, rhs);

		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpEQ(lhs.m_value, rhs.m_value, "cmptmp"));
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type)
	{
		Error(m_token, "Cannot compare equality of floats.");
		return NullInvalid();
		// upcast to double for subtraction and fabs()
		/*if (lhs.m_bits < 64) lhs = lhs.CastToFloat(64);
		if (rhs.m_bits < 64) rhs = rhs.CastToFloat(64);

		llvm::Value* tmp = m_builder->CreateCall(m_module->getFunction("fabs"), { m_builder->CreateFSub(lhs.m_value, rhs.m_value) }, "calltmp");
		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateFCmpULT(tmp, llvm::ConstantFP::get(m_module->getContext(), llvm::APFloat(1.0e-12)), "cmptmp"));*/
	}
	else if (LITERAL_TYPE_BOOL == lhs.m_type || LITERAL_TYPE_ENUM == lhs.m_type)
	{
		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpEQ(lhs.m_value, rhs.m_value, "cmptmp"));
	}
	else if (LITERAL_TYPE_STRING == lhs.m_type)
	{
		llvm::Value* tmp = m_builder->CreateCall(m_module->getFunction("__str_cmp"), { lhs.m_value, rhs.m_value }, "calltmp");
		return TValue(m_token, LITERAL_TYPE_BOOL, tmp);
	}

	Error(m_token, "Failed to create != comparison.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::IsGreaterThan(TValue rhs, bool or_equal)
{
	TValue lhs = *this;

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// hard type mismatch
	if (!matched)
	{
		Error(m_token, "Type mismatch in comparison.");
		return NullInvalid();
	}

	// type specific comparison
	if (LITERAL_TYPE_INTEGER == lhs.m_type)
	{
		CastToMaxBits(lhs, rhs);

		if (or_equal)
			return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpSGE(lhs.m_value, rhs.m_value, "cmptmp"));
		else
			return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpSGT(lhs.m_value, rhs.m_value, "cmptmp"));
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type)
	{
		CastToMaxBits(lhs, rhs);

		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateFCmpUGT(lhs.m_value, rhs.m_value, "cmptmp"));
	}

	Error(m_token, "Failed to create > comparison.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::IsLessThan(TValue rhs, bool or_equal)
{
	TValue lhs = *this;

	GetFromStorage(lhs, rhs);

	bool matched = lhs.m_type == rhs.m_type;

	// hard type mismatch
	if (!matched)
	{
		Error(m_token, "Type mismatch in comparison.");
		return NullInvalid();
	}

	// type specific comparison
	if (LITERAL_TYPE_INTEGER == lhs.m_type)
	{
		CastToMaxBits(lhs, rhs);

		if (or_equal)
			return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpSLE(lhs.m_value, rhs.m_value, "cmptmp"));
		else
			return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateICmpSLT(lhs.m_value, rhs.m_value, "cmptmp"));
	}
	else if (LITERAL_TYPE_FLOAT == lhs.m_type)
	{
		CastToMaxBits(lhs, rhs);

		return TValue(m_token, LITERAL_TYPE_BOOL, m_builder->CreateFCmpULT(lhs.m_value, rhs.m_value, "cmptmp"));
	}

	Error(m_token, "Failed to create < comparison.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Negate()
{
	TValue ret = GetFromStorage();

	if (LITERAL_TYPE_INTEGER == m_type)
	{
		ret.m_value = m_builder->CreateNeg(ret.m_value, "negtmp");
		return ret;
	}
	else if (LITERAL_TYPE_FLOAT == m_type)
	{
		ret.m_value = m_builder->CreateFNeg(ret.m_value, "negtmp");
		return ret;
	}

	Error(m_token, "Failed to negate value.");
	return NullInvalid();
}


//-----------------------------------------------------------------------------
TValue TValue::Not()
{
	TValue ret = GetFromStorage();

	if (LITERAL_TYPE_BOOL == m_type)
	{
		ret.m_value = m_builder->CreateNot(ret.m_value, "nottmp");
		return ret;
	}

	Error(m_token, "Failed to perform Not operation.");
	return NullInvalid();
}

