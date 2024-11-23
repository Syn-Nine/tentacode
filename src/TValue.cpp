#include "TValue.h"
#include "Environment.h"

ErrorHandler* TValue::m_errorHandler = nullptr;
llvm::IRBuilder<>* TValue::m_builder = nullptr;
llvm::Module* TValue::m_module = nullptr;


void TValue::Cleanup()
{
	// get from storage
	if (m_is_storage) m_value = m_builder->CreateLoad(m_ty, m_value, "tmp_load");

	if (LITERAL_TYPE_STRING == m_type)
	{
		m_builder->CreateCall(m_module->getFunction("__del_string"), { m_value }, "delstr");
	}
	else if (LITERAL_TYPE_VEC_DYNAMIC == m_type)
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "tmpload");
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_delete"), { ptr }, "delvec");
	}
}

void TValue::EmitAppend(TValue rhs)
{
	if (LITERAL_TYPE_VEC_DYNAMIC != m_type)
	{
		Error(m_token, "Cannot append to type.");
		return;
	}
	else if (LITERAL_TYPE_VEC_DYNAMIC == rhs.m_type || LITERAL_TYPE_VEC_FIXED == rhs.m_type)
	{
		Error(m_token, "Cannot append vector to vector.");
		return;
	}
	else if (m_vec_type != rhs.m_type)
	{
		if ((LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_type) ||
			(LITERAL_TYPE_INTEGER == rhs.m_type && LITERAL_TYPE_FLOAT == m_vec_type))
		{
			// ok, will cast down below if necessary
		}
		else
		{
			Error(rhs.GetToken(), "Cannot store mismatched type.");
			return;
		}
	}

	// get rhs into same type as lhs
	rhs = rhs.GetFromStorage();
	llvm::Value* tmp = rhs.m_value;

	if (m_ty != rhs.m_ty)
	{
		if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_INTEGER == rhs.m_type)
		{
			// match bitsize
			tmp = m_builder->CreateIntCast(tmp, m_ty, true, "int_cast");
		}
		else if (LITERAL_TYPE_FLOAT == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_type)
		{
			// match bitsize
			tmp = m_builder->CreateFPCast(tmp, m_ty, "fp_cast");
		}
		else if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_type)
		{
			// lhs is an int, but rhs is a float
			tmp = m_builder->CreateFPToSI(tmp, m_ty, "fp_to_int");
		}
		else if (LITERAL_TYPE_INTEGER == rhs.m_vec_type && LITERAL_TYPE_FLOAT == m_type)
		{
			// lhs is a float, but rhs is an int
			tmp = m_builder->CreateSIToFP(tmp, m_ty, "int_to_fp");
		}
		else
		{
			Error(rhs.GetToken(), "Failed to cast when storing value.");
			return;
		}
	}

	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	if (LITERAL_TYPE_INTEGER == m_vec_type)
	{
		if (16 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_i16"), { ptr, tmp }, "calltmp");
		else if (32 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_i32"), { ptr, tmp }, "calltmp");
		else if (64 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_i64"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_FLOAT == m_vec_type)
	{
		if (32 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_f32"), { ptr, tmp }, "calltmp");
		else if (64 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_f64"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_ENUM == m_vec_type)
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_enum"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_BOOL == m_vec_type)
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_bool"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_STRING == m_vec_type)
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_string"), { ptr, tmp }, "calltmp");
	}
	else
	{
		Error(rhs.GetToken(), "Failed to append to dynamic vector.");
		return;
	}
	
}

TValue TValue::EmitContains(TValue rhs)
{
	if (LITERAL_TYPE_VEC_DYNAMIC != m_type)
	{
		Error(m_token, "Cannot check contents of type.");
		return NullInvalid();
	}
	else if (m_vec_type != rhs.m_type)
	{
		Error(rhs.GetToken(), "Cannot check contents of mismatched type.");
		return NullInvalid();
	}

	// get rhs into same type as lhs
	rhs = rhs.GetFromStorage();
	llvm::Value* tmp = rhs.m_value;

	if (m_ty != rhs.m_ty)
	{
		if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_INTEGER == rhs.m_type)
		{
			// match bitsize
			tmp = m_builder->CreateIntCast(tmp, m_ty, true, "int_cast");
		}
		else if (LITERAL_TYPE_FLOAT == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_type)
		{
			// match bitsize
			tmp = m_builder->CreateFPCast(tmp, m_ty, "fp_cast");
		}
		else
		{
			Error(rhs.GetToken(), "Failed to bitcast when checking contents.");
			return NullInvalid();
		}
	}

	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
	
	TValue ret;

	if (LITERAL_TYPE_INTEGER == m_vec_type)
	{
		llvm::Value* v = nullptr;
		if (16 == m_bits) v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_i16"), { ptr, tmp }, "calltmp");
		else if (32 == m_bits) v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_i32"), { ptr, tmp }, "calltmp");
		else if (64 == m_bits) v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_i64"), { ptr, tmp }, "calltmp");

		if (v) ret = TValue::MakeBool(m_token, v);
	}
	/*else if (LITERAL_TYPE_FLOAT == m_vec_type)
	{
		if (32 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_f32"), { ptr, tmp }, "calltmp");
		else if (64 == m_bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_f64"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_ENUM == m_vec_type)
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_enum"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_BOOL == m_vec_type)
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_bool"), { ptr, tmp }, "calltmp");
	}
	else if (LITERAL_TYPE_STRING == m_vec_type)
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_string"), { ptr, tmp }, "calltmp");
	}*/
	else
	{
		Error(rhs.GetToken(), "Failed to check contents of dynamic vector.");
		return NullInvalid();
	}

	return ret;
}

TValue TValue::EmitLen()
{
	TValue ret;

	if (LITERAL_TYPE_VEC_FIXED == m_type)
	{
		llvm::Value* v = m_builder->getInt64(m_fixed_vec_len);
		ret = TValue(m_token, LITERAL_TYPE_INTEGER, v);
		ret.m_bits = 64;
	}
	else if (LITERAL_TYPE_VEC_DYNAMIC == m_type)
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_len"), { ptr }, "calltmp");
		ret = TValue(m_token, LITERAL_TYPE_INTEGER, v);
		ret.m_is_storage = false;
		ret.m_bits = 64;
	}
	else
	{
		Error(m_token, "Cannot get length of value.");
	}

	return ret;
}


TValue TValue::GetFromStorage()
{
	TValue ret = *this; // prototype
	if (m_is_storage) ret.m_value = m_builder->CreateLoad(m_ty, m_value, "tmp_load");
	ret.m_is_storage = false;
	return ret;
}


void TValue::GetFromStorage(TValue& lhs, TValue& rhs)
{
	if (lhs.m_is_storage)
	{
		lhs.m_value = m_builder->CreateLoad(lhs.m_ty, lhs.m_value, "lhs_load");
		lhs.m_is_storage = false;
	}
	if (rhs.m_is_storage)
	{
		rhs.m_value = m_builder->CreateLoad(rhs.m_ty, rhs.m_value, "rhs_load");
		rhs.m_is_storage = false;
	}
}



TValue TValue::GetAtVectorIndex(TValue idx)
{
	TValue ret;

	if (LITERAL_TYPE_VEC_FIXED == m_type)
	{
		llvm::Value* gep = m_builder->CreateGEP(m_ty, m_value, { idx.m_value }, "geptmp");
		ret.m_value = m_builder->CreateLoad(m_ty, gep);
		ret.m_ty = m_ty;
		ret.m_bits = m_bits;
		ret.m_is_storage = false;
		ret.m_type = m_vec_type;
		ret.m_token = idx.m_token;
	}
	else if (LITERAL_TYPE_VEC_DYNAMIC == m_type)
	{
		llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_assert_idx"), { dstPtr, idx.m_value }, "calltmp");
		llvm::Value* dataPtr = m_builder->CreateCall(m_module->getFunction("__dyn_vec_data_ptr"), { dstPtr }, "calltmp");
		llvm::Value* gep = m_builder->CreateGEP(m_ty, dataPtr, { idx.m_value }, "geptmp");
		ret.m_value = m_builder->CreateLoad(m_ty, gep);
		ret.m_ty = m_ty;
		ret.m_bits = m_bits;
		ret.m_is_storage = false;
		ret.m_type = m_vec_type;
		ret.m_token = idx.m_token;
	}
	else
	{
		Error(m_token, "Failed to get from vector index.");
	}

	return ret;
}


void TValue::Store(TValue rhs)
{
	if (rhs.IsInvalid())
	{
		Error(rhs.GetToken(), "Cannot store invalid value.");
		return;
	}
	else if (m_type != rhs.m_type || m_ty != rhs.m_ty)
	{
		if (IsVecAny() && rhs.IsVecAny())
		{
			if ((LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_vec_type) ||
				(LITERAL_TYPE_INTEGER == rhs.m_vec_type && LITERAL_TYPE_FLOAT == m_vec_type) ||
				m_vec_type == rhs.m_vec_type)
			{
				// ok, will cast down below if necessary
			}
			else
			{
				Error(rhs.GetToken(), "Cannot store mismatched type.");
				return;
			}
		}
		else
		{
			Error(rhs.GetToken(), "Cannot store mismatched type.");
			return;
		}
	}

	switch (m_type)
	{
	case LITERAL_TYPE_INTEGER: // intentional fall-through
	case LITERAL_TYPE_FLOAT:
	case LITERAL_TYPE_ENUM:
	case LITERAL_TYPE_POINTER:
	case LITERAL_TYPE_BOOL:
	{
		rhs = rhs.GetFromStorage();
		m_builder->CreateStore(rhs.m_value, m_value);
		break;
	}
	case LITERAL_TYPE_STRING:
	{
		TValue lhs = *this; // clone
		GetFromStorage(lhs, rhs);
		m_builder->CreateCall(m_module->getFunction("__str_assign"), { lhs.m_value, rhs.m_value }, "calltmp");
		break;
	}
	case LITERAL_TYPE_VEC_DYNAMIC:
	{
		if (LITERAL_TYPE_VEC_FIXED == rhs.m_type && rhs.m_fixed_vec_len > 0)
		{
			llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
			llvm::Value* srcLen = m_builder->getInt64(rhs.m_fixed_vec_len);
			m_builder->CreateCall(m_module->getFunction("__dyn_vec_clear_presize"), { dstPtr, srcLen }, "calltmp");

			m_builder->CreateCall(m_module->getFunction("__dyn_vec_assert_idx"), { dstPtr, m_builder->getInt32(rhs.m_fixed_vec_len - 1) }, "calltmp");
			llvm::Value* dataPtr = m_builder->CreateCall(m_module->getFunction("__dyn_vec_data_ptr"), { dstPtr }, "calltmp");
			
			for (size_t i = 0; i < rhs.m_fixed_vec_len; ++i)
			{
				llvm::Value* lhs_gep = m_builder->CreateGEP(m_ty, dataPtr, { m_builder->getInt32(i) }, "lhs_geptmp");
				llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ty, rhs.m_value, { m_builder->getInt32(i) }, "rhs_geptmp");
				llvm::Value* tmp = m_builder->CreateLoad(rhs.m_ty, rhs_gep);

				if (m_ty != rhs.m_ty)
				{
					if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_INTEGER == rhs.m_vec_type)
					{
						// match bitsize
						tmp = m_builder->CreateIntCast(tmp, m_ty, true, "int_cast");
					}
					else if (LITERAL_TYPE_FLOAT == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_vec_type)
					{
						// match bitsize
						tmp = m_builder->CreateFPCast(tmp, m_ty, "fp_cast");
					}
					else if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_vec_type)
					{
						// lhs is an int, but rhs is a float
						tmp = m_builder->CreateFPToSI(tmp, m_ty, "fp_to_int");
					}
					else if (LITERAL_TYPE_INTEGER == rhs.m_vec_type && LITERAL_TYPE_FLOAT == m_vec_type)
					{
						// lhs is a float, but rhs is an int
						tmp = m_builder->CreateSIToFP(tmp, m_ty, "int_to_fp");
					}
					else
					{
						Error(rhs.GetToken(), "Failed to cast when storing value.");
						return;
					}
				}

				if (LITERAL_TYPE_STRING == m_vec_type)
				{
					tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { tmp }, "calltmp");
				}
				
				m_builder->CreateStore(tmp, lhs_gep);
			}
		}
		else if (LITERAL_TYPE_VEC_DYNAMIC == rhs.m_type)
		{
			if (m_ty != rhs.m_ty)
			{
				Error(rhs.GetToken(), "Cannot assign to dynamic vector with different type.");
				return;
			}
			llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "dst_loadtmp");
			llvm::Value* srcPtr = m_builder->CreateLoad(m_builder->getPtrTy(), rhs.m_value, "src_loadtmp");
			m_builder->CreateCall(m_module->getFunction("__dyn_vec_replace"), { dstPtr, srcPtr }, "calltmp");
		}
		else
		{
			Error(rhs.GetToken(), "Cannot assign to dynamic vector.");
			return;
		}
		break;
	}
	case LITERAL_TYPE_VEC_FIXED:
	{
		if (LITERAL_TYPE_VEC_FIXED == rhs.m_type)
		{
			if (m_fixed_vec_len != rhs.m_fixed_vec_len)
			{
				Error(rhs.GetToken(), "Mismatched fixed vector lengths.");
				return;
			}

			for (size_t i = 0; i < m_fixed_vec_len; ++i)
			{
				llvm::Value* lhs_gep = m_builder->CreateGEP(m_ty, m_value, { m_builder->getInt32(i) }, "lhs_geptmp");
				llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ty, rhs.m_value, { m_builder->getInt32(i) }, "rhs_geptmp");
				llvm::Value* tmp = m_builder->CreateLoad(rhs.m_ty, rhs_gep);

				if (m_ty != rhs.m_ty)
				{
					if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_INTEGER == rhs.m_vec_type)
					{
						// match bitsize
						tmp = m_builder->CreateIntCast(tmp, m_ty, true, "int_cast");
					}
					else if (LITERAL_TYPE_FLOAT == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_vec_type)
					{
						// match bitsize
						tmp = m_builder->CreateFPCast(tmp, m_ty, "fp_cast");
					}
					else if (LITERAL_TYPE_INTEGER == m_vec_type && LITERAL_TYPE_FLOAT == rhs.m_vec_type)
					{
						// lhs is an int, but rhs is a float
						tmp = m_builder->CreateFPToSI(tmp, m_ty, "fp_to_int");
					}
					else if (LITERAL_TYPE_INTEGER == rhs.m_vec_type && LITERAL_TYPE_FLOAT == m_vec_type)
					{
						// lhs is a float, but rhs is an int
						tmp = m_builder->CreateSIToFP(tmp, m_ty, "int_to_fp");
					}
					else
					{
						Error(rhs.GetToken(), "Failed to cast when storing value.");
						return;
					}
				}

				m_builder->CreateStore(tmp, lhs_gep);
			}
		}
		else if (LITERAL_TYPE_VEC_DYNAMIC == rhs.m_type)
		{
			Error(rhs.GetToken(), "Cannot assign dynamic vector to fixed vector.");
			return;
		}
		else
		{
			Error(rhs.GetToken(), "Cannot assign to fixed vector.");
			return;
		}
		
		break;
	}
	default:
		Error(rhs.GetToken(), "Failed to store value.");
	}
}


void TValue::StoreAtIndex(TValue idx, TValue rhs)
{
	if (!IsVecAny())
	{
		Error(rhs.GetToken(), "Cannot store into this type.");
		return;
	}
	else if (rhs.IsInvalid())
	{
		Error(rhs.GetToken(), "Cannot store invalid value.");
		return;
	}
	else if (idx.IsInvalid())
	{
		Error(idx.GetToken(), "Cannot store at invalid index.");
		return;
	}
	else if (m_vec_type != rhs.m_type || m_ty != rhs.m_ty)
	{
		Error(rhs.GetToken(), "Cannot store mismatched type.");
		return;
	}

	if (LITERAL_TYPE_VEC_FIXED == m_type)
	{
		llvm::Value* gep = m_builder->CreateGEP(m_ty, m_value, { idx.m_value }, "geptmp");
		m_builder->CreateStore(rhs.m_value, gep);
	}
	else if (LITERAL_TYPE_VEC_DYNAMIC == m_type)
	{
		llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_assert_idx"), { dstPtr, idx.m_value }, "calltmp");
		llvm::Value* dataPtr = m_builder->CreateCall(m_module->getFunction("__dyn_vec_data_ptr"), { dstPtr }, "calltmp");
		llvm::Value* gep = m_builder->CreateGEP(m_ty, dataPtr, { idx.m_value }, "geptmp");

		llvm::Value* tmp = rhs.m_value;
		if (LITERAL_TYPE_STRING == m_vec_type)
		{
			tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { tmp }, "calltmp");
		}

		m_builder->CreateStore(tmp, gep);
	}
	else
	{
		Error(m_token, "Failed to store into vector index.");
	}

}