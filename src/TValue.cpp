#include "TValue.h"
#include "Environment.h"

ErrorHandler* TValue::m_errorHandler = nullptr;
llvm::IRBuilder<>* TValue::m_builder = nullptr;
llvm::Module* TValue::m_module = nullptr;


//-----------------------------------------------------------------------------
void TValue::Cleanup()
{
	// get from storage
	if (IsString())
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "tmpload");
		//printf("attempting to delete: %llu\n", ptr);
		m_builder->CreateCall(m_module->getFunction("__del_string"), { ptr }, "delstr");
		//m_builder->CreateStore(llvm::Constant::getNullValue(m_builder->getPtrTy()), m_value, "nullify_ptr");
		m_builder->CreateStore(m_builder->getInt64(-1), m_value, "dirty_ptr");
	}
	/****else if (LITERAL_TYPE_VEC_DYNAMIC == m_type)
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "tmpload");
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_delete"), { ptr }, "delvec");
		m_builder->CreateStore(llvm::Constant::getNullValue(m_builder->getPtrTy()), m_value, "nullify_ptr");
	}*/
}



//-----------------------------------------------------------------------------
TValue TValue::EmitLen()
{
	if (m_ttype.IsVecFixed())
	{
		llvm::Value* v = m_builder->getInt64(m_ttype.GetFixedVecLen());
		return MakeInt(m_token, 64, v);
	}
	else if (m_ttype.IsVecDynamic())
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_len"), { ptr }, "calltmp");
		return MakeInt(m_token, 64, v);
	}
	else if (m_ttype.IsSet())
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__set_len"), { ptr }, "calltmp");
		return MakeInt(m_token, 64, v);
	}
	else if (m_ttype.IsMap())
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__map_len"), { ptr }, "calltmp");
		return MakeInt(m_token, 64, v);
	}
	else if (m_ttype.IsString())
	{
		llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__str_len"), { ptr }, "calltmp");
		return MakeInt(m_token, 64, v);
	}
	else
	{
		Error(m_token, "Cannot get length of value.");
	}

	return NullInvalid();
}



//-----------------------------------------------------------------------------
void TValue::EmitMapInsert(TValue rhs)
{
	if (LITERAL_TYPE_MAP != m_ttype.GetLiteralType())
	{
		Error(m_token, "Cannot insert into type.");
		return;
	}

	// lhs is a map an rhs is a pair
	TValue key = *rhs.m_pair_lhs;
	TValue val = *rhs.m_pair_rhs;

	TType i0 = m_ttype.GetInternal(0);
	TType i1 = m_ttype.GetInternal(1);

	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	if (i0.GetLiteralType() != key.GetLiteralType())
	{
		Error(key.GetToken(), "Key type mismatch.");
		return;
	}
	
	// check for value float to int cast
	if (val.IsFloat() && i1.IsInteger()) val = val.CastToInt(i1.NumBits());
	if (val.IsInteger() && i1.IsFloat()) val = val.CastToFloat(i1.NumBits());

	if (i1.GetLiteralType() != val.GetLiteralType())
	{
		Error(key.GetToken(), "Value type mismatch.");
		return;
	}

	// upcast int key
	if (i0.IsInteger() && key.IsInteger()) key = key.CastToInt(64);

	// match value bitsize
	val = val.CastToMatchImplicit(i1);//.MakeStorage();
	llvm::Value* vptr = nullptr;
	if (val.IsUDT())
	{
		vptr = m_builder->CreateGEP(i1.GetTy(), val.Value(), { m_builder->getInt32(0) }, "geptmp");
	}
	else
	{
		val = val.MakeStorage();
		vptr = val.GetPtrToStorage();
	}

	if (vptr)
	{
		if (key.IsInteger() || key.IsEnum())
		{
			m_builder->CreateCall(m_module->getFunction("__map_insert_int_key"), { ptr, key.Value(), vptr }, "calltmp");
		}
		else if (key.IsString())
		{
			m_builder->CreateCall(m_module->getFunction("__map_insert_string_key"), { ptr, key.Value(), vptr }, "calltmp");
		}
	}
	else
	{
		Error(m_token, "Failed to insert value into map.");
	}
}



//-----------------------------------------------------------------------------
void TValue::EmitSetInsert(TValue rhs)
{
	if (LITERAL_TYPE_SET != m_ttype.GetLiteralType())
	{
		Error(m_token, "Cannot insert into type.");
		return;
	}
	
	TType i0 = m_ttype.GetInternal(0);

	rhs = rhs.GetFromStorage();
	llvm::Value* tmp = rhs.m_value;
	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
	
	if (rhs.IsInteger() && i0.IsInteger())
	{
		// up cast to i64
		tmp = m_builder->CreateIntCast(tmp, m_builder->getInt64Ty(), true, "int_cast");
		m_builder->CreateCall(m_module->getFunction("__set_insert_int"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.IsEnum() && i0.IsEnum())
	{
		tmp = m_builder->CreateIntCast(tmp, m_builder->getInt64Ty(), true, "int_cast");
		m_builder->CreateCall(m_module->getFunction("__set_insert_int"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.IsString() && i0.IsString())
	{
		m_builder->CreateCall(m_module->getFunction("__set_insert_string"), { ptr, tmp }, "calltmp");
	}
	else
	{
		Error(rhs.GetToken(), "Cannot store mismatched type.");
	}
	
}



//-----------------------------------------------------------------------------
void TValue::EmitAppend(TValue rhs)
{
	TType i0 = m_ttype.GetInternal(0);

	if (LITERAL_TYPE_VEC_DYNAMIC != m_ttype.GetLiteralType())
	{
		Error(m_token, "Cannot append to type.");
		return;
	}
	else if (rhs.IsVecAny())
	{
		Error(m_token, "Cannot append vector to vector (yet).");
		return;
	}
	
	// get rhs into same type as lhs
	rhs = rhs.CastToMatchImplicit(i0);
	
	llvm::Value* tmp = rhs.m_value;
	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	// check for float to int cast
	//if (rhs.IsFloat() && i0.IsInteger()) rhs = rhs.CastToInt(i0.NumBits());
	//if (rhs.IsInteger() && i0.IsFloat()) rhs = rhs.CastToFloat(i0.NumBits());
	
	// append
	if (rhs.IsInteger() && i0.IsInteger())
	{
		// match bitsize
		//tmp = m_builder->CreateIntCast(tmp, i0.GetTy(), true, "int_cast");
		int bits = i0.NumBits();
		if (16 == bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_i16"), { ptr, tmp }, "calltmp");
		else if (32 == bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_i32"), { ptr, tmp }, "calltmp");
		else if (64 == bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_i64"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.IsFloat() && i0.IsFloat())
	{
		// match bitsize
		//tmp = m_builder->CreateFPCast(tmp, i0.GetTy(), "fp_cast");
		int bits = i0.NumBits();
		if (32 == bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_f32"), { ptr, tmp }, "calltmp");
		else if (64 == bits) m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_f64"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.IsEnum() && i0.IsEnum())
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_enum"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.IsBool() && i0.IsBool())
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_bool"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.IsString() && i0.IsString())
	{
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_append_string"), { ptr, tmp }, "calltmp");
	}
	else if (rhs.GetLiteralType() != i0.GetLiteralType())
	{
		Error(rhs.GetToken(), "Cannot store mismatched type.");
		return;
	}
	else
	{
		Error(rhs.GetToken(), "Failed to append to dynamic vector.");
		return;
	}
	
}


//-----------------------------------------------------------------------------
TValue TValue::EmitContains(TValue rhs)
{
	TType i0 = m_ttype.GetInternal(0);
	TType i1 = rhs.m_ttype;

	if (!m_ttype.IsVecDynamic())
	{
		Error(m_token, "Cannot check contents of type.");
		return NullInvalid();
	}
	else if (i0.GetLiteralType() != i1.GetLiteralType())
	{
		Error(rhs.GetToken(), "Cannot check contents of mismatched type.");
		return NullInvalid();
	}

	// get rhs into same type as lhs
	rhs = rhs.GetFromStorage();
	llvm::Value* tmp = rhs.m_value;

	if (i0.GetTy() != i1.GetTy())
	{
		if (i0.IsInteger() && i1.IsInteger())
		{
			// match bitsize
			tmp = m_builder->CreateIntCast(tmp, i0.GetTy(), true, "int_cast");
		}
		else if (i0.IsFloat() && i1.IsFloat())
		{
			// match bitsize
			tmp = m_builder->CreateFPCast(tmp, i0.GetTy(), "fp_cast");
		}
		else
		{
			Error(rhs.GetToken(), "Failed to bitcast when checking contents.");
			return NullInvalid();
		}
	}

	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	TValue ret;

	if (i0.IsInteger())
	{
		llvm::Value* v = nullptr;
		int bits = i0.NumBits();
		if (16 == bits) v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_i16"), { ptr, tmp }, "calltmp");
		else if (32 == bits) v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_i32"), { ptr, tmp }, "calltmp");
		else if (64 == bits) v = m_builder->CreateCall(m_module->getFunction("__dyn_vec_contains_i64"), { ptr, tmp }, "calltmp");

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


//-----------------------------------------------------------------------------
void TValue::EmitStartIterator()
{
	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	if (IsSet())
	{
		m_builder->CreateCall(m_module->getFunction("__set_start_iter"), { ptr }, "calltmp");
	}
	else if (IsMap())
	{
		m_builder->CreateCall(m_module->getFunction("__map_start_iter"), { ptr }, "calltmp");
	}
	else
	{
		Error(m_token, "Cannot start iterator on type.");
	}
}



//-----------------------------------------------------------------------------
TValue TValue::GetFromStorage()
{
	TValue ret = *this; // prototype
	if (m_is_storage) ret.m_value = m_builder->CreateLoad(GetTy(), m_value, "tmp_load");
	ret.m_is_storage = false;
	return ret;
}



//-----------------------------------------------------------------------------
void TValue::GetFromStorage(TValue& lhs, TValue& rhs)
{
	if (lhs.m_is_storage)
	{
		lhs.m_value = m_builder->CreateLoad(lhs.GetTy(), lhs.m_value, "lhs_load");
		lhs.m_is_storage = false;
	}
	if (rhs.m_is_storage)
	{
		rhs.m_value = m_builder->CreateLoad(rhs.GetTy(), rhs.m_value, "rhs_load");
		rhs.m_is_storage = false;
	}
}



//-----------------------------------------------------------------------------
TValue TValue::GetIterKey()
{
	// call is only valid for maps
	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	TType i0 = m_ttype.GetInternal(0);
	TType i1 = m_ttype.GetInternal(1);

	if (i0.IsInteger())
	{
		llvm::Value* v = nullptr;
		if (16 == i0.NumBits())
		{
			v = m_builder->CreateCall(m_module->getFunction("__get_map_iter_key_i16"), { ptr }, "calltmp");
		}
		else if (32 == i0.NumBits())
		{
			v = m_builder->CreateCall(m_module->getFunction("__get_map_iter_key_i32"), { ptr }, "calltmp");
		}
		else if (64 == i0.NumBits())
		{
			v = m_builder->CreateCall(m_module->getFunction("__get_map_iter_key_i64"), { ptr }, "calltmp");
		}
		return TValue::MakeInt(m_token, i0.NumBits(), v);
	}
	else if (i0.IsString())
	{
		llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__get_map_iter_key_str"), { ptr }, "calltmp");
		return TValue::MakeString(m_token, v);
	}

	return NullInvalid();
}



//-----------------------------------------------------------------------------
TValue TValue::GetIterValue()
{
	llvm::Value* ptr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

	if (IsSet())
	{
		TType i0 = m_ttype.GetInternal(0);
		if (i0.IsInteger())
		{
			llvm::Value* v = nullptr;
			if (16 == i0.NumBits())
			{
				v = m_builder->CreateCall(m_module->getFunction("__get_set_iter_val_i16"), { ptr }, "calltmp");
			}
			else if (32 == i0.NumBits())
			{
				v = m_builder->CreateCall(m_module->getFunction("__get_set_iter_val_i32"), { ptr }, "calltmp");
			}
			else if (64 == i0.NumBits())
			{
				v = m_builder->CreateCall(m_module->getFunction("__get_set_iter_val_i64"), { ptr }, "calltmp");
			}
			return TValue::MakeInt(m_token, i0.NumBits(), v);
		}
		else
		{
			llvm::Value* v = m_builder->CreateCall(m_module->getFunction("__get_set_iter_val_str"), { ptr }, "calltmp");
			return TValue::MakeString(m_token, v);
		}
	}
	else if (IsMap())
	{
		// make a temporary to hold the return value through a reference
		TType i1 = m_ttype.GetInternal(1);
		TValue temp = TValue::Construct(i1, "temp", false);
		llvm::Value* tmp = temp.Value();
		if (temp.IsUDT())
		{
			tmp = m_builder->CreateGEP(i1.GetTy(), tmp, { m_builder->getInt32(0) }, "geptmp");
		}
		m_builder->CreateCall(m_module->getFunction("__get_map_iter_val"), { ptr, tmp }, "calltmp");
		return temp;
	}

	return NullInvalid();
}



//-----------------------------------------------------------------------------
llvm::Value* TValue::GetPtrToStorage()
{
	if (!m_is_storage)
	{
		Error(m_token, "Trying to get pointer to storage when value is not in storage.");
		return nullptr;
	}

	if (m_ttype.CanGEP())
	{
		return m_builder->CreateGEP(m_ttype.GetTy(), m_value, { m_builder->getInt32(0) }, "geptmp");
	}

	return m_value;
}



//-----------------------------------------------------------------------------
TValue TValue::GetAtIndex(TValue idx)
{
	TType i0 = m_ttype.GetInternal(0);

	if (m_ttype.IsVecFixed())
	{
		/*if (LITERAL_TYPE_UDT == m_vec_type)
		{
			llvm::Value* gep = m_builder->CreateGEP(m_ty, m_value, { idx.m_value }, "geptmp");
			ret.m_value = gep;
			ret.m_ty = m_ty;
			ret.m_bits = m_bits;
			ret.m_is_storage = false;
			ret.m_type = m_vec_type;
			ret.m_token = idx.m_token;
			ret.m_vec_type_name = m_vec_type_name;
		}
		else
		{*/
		llvm::Value* gep = m_builder->CreateGEP(i0.GetTy(), m_value, {idx.m_value}, "geptmp");
		//llvm::Value* gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { idx.m_value }, "geptmp");
		TValue ret = Construct(i0, "tmp", false);
		ret.SetValue(gep);
		return ret;
		//}
	}
	else if (m_ttype.IsVecDynamic())
	{
		llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_assert_idx"), { dstPtr, idx.m_value }, "calltmp");
		llvm::Value* dataPtr = m_builder->CreateCall(m_module->getFunction("__dyn_vec_data_ptr"), { dstPtr }, "calltmp");
		llvm::Value* gep = m_builder->CreateGEP(i0.GetTy(), dataPtr, {idx.m_value}, "geptmp");
		TValue ret = Construct(i0, "tmp", false);
		ret.SetValue(gep);
		return ret;
	}
	else if (m_ttype.IsMap())
	{
		llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		
		TType i1 = m_ttype.GetInternal(1);
		TValue ret = Construct(i1, "tmp", false);
		llvm::Value* tmp = ret.Value();
		if (ret.IsUDT())
		{
			tmp = m_builder->CreateGEP(i1.GetTy(), tmp, { m_builder->getInt32(0) }, "geptmp");
		}

		if (i0.IsInteger())
		{
			if (!idx.IsInteger())
			{
				Error(m_token, "Expected integer index for map.");
				return NullInvalid();
			}
			m_builder->CreateCall(m_module->getFunction("__get_map_val_at_i64"), { dstPtr, idx.CastToInt(64).Value(), tmp }, "calltmp");
		}
		else if (i0.IsString())
		{
			if (!idx.IsString())
			{
				Error(m_token, "Expected string index for map.");
				return NullInvalid();
			}
			m_builder->CreateCall(m_module->getFunction("__get_map_val_at_str"), { dstPtr, idx.m_value, tmp }, "calltmp");
		}

		return ret;
	}
	else if (m_ttype.IsTuple())
	{
		if (!idx.IsInteger())
		{
			Error(m_token, "Expected integer index for tuple.");
			return NullInvalid();
		}
		if (!idx.IsConstant())
		{
			Error(m_token, "Index must be constant.");
			return NullInvalid();
		}

		// determine index
		int index = 0;
		llvm::Value* v = idx.Value();
		if (llvm::isa<llvm::GlobalValue>(v))
		{
			llvm::GlobalVariable* gv = llvm::cast<llvm::GlobalVariable>(v);
			if (gv)
			{
				llvm::Constant* c = gv->getInitializer();
				if (c)
				{
					llvm::ConstantInt* ci = llvm::cast<llvm::ConstantInt>(c);
					if (ci) index = ci->getSExtValue();
				}
			}
		}
		else
		{
			index = idx.GetToken()->IntValue();
		}

		if (index >= m_ttype.GetInternalCount() || index < 0)
		{
			Error(m_token, "Tuple index out of bounds.");
			return NullInvalid();
		}

		llvm::Value* vidx = m_builder->getInt32(index);
		llvm::Value* zero = m_builder->getInt32(0);
		llvm::Value* gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, vidx }, "geptmp");
		
		TValue ret = TValue::Construct(m_ttype.GetInternal(index), "tmp", false);
		ret.SetValue(m_builder->CreateLoad(ret.GetTy(), gep));
		ret.SetStorage(false);
		return ret;
	}
	else
	{
		Error(m_token, "Failed to get from variable index.");
	}

	return NullInvalid();
}



//-----------------------------------------------------------------------------
TValue TValue::GetStructVariable(Token* token, llvm::Value* vec_idx, const std::string& name)
{
	if (!IsValid()) return NullInvalid();

	llvm::Value* zero = m_builder->getInt32(0);

	std::string udt_name = m_token->Lexeme();
	if (IsVecAny()) udt_name = m_ttype.GetInternal(0).GetToken()->Lexeme();
	
	TStruct struc = Environment::GetStruct(m_token, udt_name);
	if (!struc.IsValid()) return TValue::NullInvalid();

	TType ttype = struc.GetMember(token, name);
	if (!ttype.IsValid()) return TValue::NullInvalid();

	llvm::Value* gep_loc = struc.GetGEPLoc(token, name);
	if (!gep_loc) return TValue::NullInvalid();

	TValue ret;
	llvm::Value* gep = nullptr;
	if (vec_idx)
	{
		ret = TValue::Construct(ttype.GetInternal(0), "tmp", false);
		gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, gep_loc, vec_idx }, name);
	}
	else
	{
		ret = TValue::Construct(ttype, "tmp", false);
		gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, gep_loc }, name);
	}

	if (!gep)
	{
		Error(m_token, "Failed to get structure variable.");
	}

	ret.SetValue(gep);

	return ret;
}



//-----------------------------------------------------------------------------
TValue TValue::MakeStorage()
{
	if (!m_is_valid) return *this;
	if (m_is_storage) return *this;

	TValue ret = *this;
	llvm::Value* v = CreateEntryAlloca(m_builder, m_ttype.GetTy(), nullptr, "make_storage");
	m_builder->CreateStore(m_value, v);
	ret.m_value = v;
	ret.m_is_storage = true;

	return ret;
}



//-----------------------------------------------------------------------------
void TValue::Store(TValue rhs)
{
	if (!rhs.IsValid())
	{
		Error(rhs.GetToken(), "Cannot store invalid value.");
		return;
	}
	else if (GetLiteralType() != rhs.GetLiteralType() || GetTy() != rhs.GetTy())
	{
		/****if (IsVecAny() && rhs.IsVecAny())
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
		{*/
			//m_ty->print(llvm::errs());
			//rhs.m_ty->print(llvm::errs());
			rhs = rhs.GetFromStorage().CastToMatchImplicit(*this);
			if (!rhs.IsValid())
			{
				Error(rhs.GetToken(), "Cannot store mismatched type.");
				return;
			}
		//}
	}

	switch (GetLiteralType())
	{
	case LITERAL_TYPE_INTEGER: // intentional fall-through
	case LITERAL_TYPE_FLOAT:
	case LITERAL_TYPE_ENUM:
	case LITERAL_TYPE_POINTER:
	case LITERAL_TYPE_BOOL:
	{
		rhs = rhs.GetFromStorage();//.CastToMatchImplicit(*this);
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
	case LITERAL_TYPE_SET:
	{
		if ((m_ttype.GetInternal(0).IsInteger() && rhs.m_ttype.GetInternal(0).IsInteger()) || (m_ttype.GetInternal(0).IsString() && rhs.m_ttype.GetInternal(0).IsString()))
		{
			TValue lhs = *this; // clone
			GetFromStorage(lhs, rhs);
			m_builder->CreateCall(m_module->getFunction("__set_assign"), { lhs.m_value, rhs.m_value }, "calltmp");
		}
		else
		{
			Error(rhs.m_token, "Cannot store mismatched set types.");
			return;
		}
		break;
	}
	case LITERAL_TYPE_MAP:
	{
		/*if ((m_ttype.GetInternal(0).IsInteger() && rhs.m_ttype.GetInternal(0).IsInteger()) || (m_ttype.GetInternal(0).IsString() && rhs.m_ttype.GetInternal(0).IsString()))
		{
			TValue lhs = *this; // clone
			GetFromStorage(lhs, rhs);
			m_builder->CreateCall(m_module->getFunction("__map_assign"), { lhs.m_value, rhs.m_value }, "calltmp");
		}
		else
		{
			Error(rhs.m_token, "Cannot store mismatched map types.");
		}*/
		break;
	}
	case LITERAL_TYPE_VEC_DYNAMIC:
	{
		if (rhs.m_ttype.IsVecFixed() && rhs.m_ttype.GetFixedVecLen() > 0)
		{
			int a = 0;
			/*llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
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
			}*/
		}
		else if (rhs.IsVecDynamic())
		{
			if (m_ttype.GetInternal(0).GetTy() != rhs.m_ttype.GetInternal(0).GetTy())
			{
				Error(rhs.GetToken(), "Cannot assign to dynamic vector with different type.");
				return;
			}
			TValue lhs = *this; // clone
			GetFromStorage(lhs, rhs);
			m_builder->CreateCall(m_module->getFunction("__dyn_vec_replace"), { lhs.m_value, rhs.m_value }, "calltmp");
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
		if (rhs.IsVecFixed())
		{
			if (m_ttype.GetSzBytes() != rhs.m_ttype.GetSzBytes())
			{
				Error(rhs.GetToken(), "Mismatched fixed vector lengths.");
				return;
			}
			TValue lhs = *this;
			GetFromStorage(lhs, rhs);
			llvm::Value* zero = m_builder->getInt32(0);

			if (m_ttype.GetTy() != rhs.m_ttype.GetTy())
			{
				llvm::Type* lhs_ty0 = m_ttype.GetInternal(0).GetTy();
				llvm::Type* rhs_ty0 = rhs.m_ttype.GetInternal(0).GetTy();

				for (size_t i = 0; i < m_ttype.GetFixedVecLen(); ++i)
				{
					llvm::Value* gep_idx = m_builder->getInt32(i);
					llvm::Value* lhs_gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, gep_idx }, "lhs_geptmp");
					llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ttype.GetTy(), rhs.m_value, { zero, gep_idx }, "rhs_geptmp");
					llvm::Value* tmp = m_builder->CreateLoad(rhs_ty0, rhs_gep);

					if (m_ttype.GetInternal(0).IsInteger() && rhs.m_ttype.GetInternal(0).IsInteger())
					{
						// match bitsize
						tmp = m_builder->CreateIntCast(tmp, lhs_ty0, true, "int_cast");
					}
					else if (m_ttype.GetInternal(0).IsFloat() && rhs.m_ttype.GetInternal(0).IsFloat())
					{
						// match bitsize
						tmp = m_builder->CreateFPCast(tmp, lhs_ty0, "fp_cast");
					}
					else if (m_ttype.GetInternal(0).IsInteger() && rhs.m_ttype.GetInternal(0).IsFloat())
					{
						// lhs is an int, but rhs is a float
						tmp = m_builder->CreateFPToSI(tmp, lhs_ty0, "fp_to_int");
					}
					else if (m_ttype.GetInternal(0).IsFloat() && rhs.m_ttype.GetInternal(0).IsInteger())
					{
						// lhs is a float, but rhs is an int
						tmp = m_builder->CreateSIToFP(tmp, lhs_ty0, "int_to_fp");
					}
					else
					{
						Error(rhs.GetToken(), "Failed to cast when storing value.");
						return;
					}

					m_builder->CreateStore(tmp, lhs_gep);
				}
			}
			else
			{
				llvm::Value* lhs_gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero }, "lhs_geptmp");
				llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ttype.GetTy(), rhs.m_value, { zero }, "rhs_geptmp");
				m_builder->CreateStore(m_builder->CreateLoad(m_ttype.GetTy(), rhs_gep), lhs_gep);
			}
		}
		else if (rhs.m_ttype.IsVecDynamic())
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
	case LITERAL_TYPE_TUPLE:
	{
		if (m_ttype.GetTy() != rhs.m_ttype.GetTy())
		{
			Error(rhs.GetToken(), "Mismatched tuple types.");
			return;
		}

		llvm::Value* zero = m_builder->getInt32(0);
		
		// copy in one go
		llvm::Value* lhs_gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero }, "lhs_geptmp");
		llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ttype.GetTy(), rhs.m_value, { zero }, "rhs_geptmp");
		m_builder->CreateStore(m_builder->CreateLoad(m_ttype.GetTy(), rhs_gep), lhs_gep);

		// deep copy strings
		for (size_t i = 0; i < m_ttype.GetInternalCount(); ++i)
		{
			TType i0 = m_ttype.GetInternal(i);
			if (!i0.IsString()) continue;
			
			llvm::Value* gep_idx = m_builder->getInt32(i);
			llvm::Value* lhs_gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, gep_idx }, "lhs_geptmp");
			llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ttype.GetTy(), rhs.m_value, { zero, gep_idx }, "rhs_geptmp");

			llvm::Value* tmp = m_builder->CreateLoad(i0.GetTy(), rhs_gep);
			tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { tmp }, "calltmp");
			m_builder->CreateStore(tmp, lhs_gep);
		}

		break;
	}
	case LITERAL_TYPE_UDT:
	{
		TStruct tstruc = Environment::GetStruct(m_token, m_token->Lexeme());

		std::vector<std::string>& member_names = tstruc.GetMemberNames();
		std::vector<TType>& members = tstruc.GetMemberVec();

		llvm::Value* gep_zero = m_builder->getInt32(0);

		for (size_t i = 0; i < members.size(); ++i)
		{
			llvm::Value* lhs_gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { gep_zero, m_builder->getInt32(i)}, member_names[i] + "_lhs");
			llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ttype.GetTy(), rhs.m_value, { gep_zero, m_builder->getInt32(i) }, member_names[i] + "_rhs");
			llvm::Value* tmp = m_builder->CreateLoad(members[i].GetTy(), rhs_gep);
			if (!tmp)
			{
				Error(rhs.GetToken(), "Failed to load from member data.");
				return;
			}

			// deep copy string
			if (members[i].IsString()) // todo - add support for deep copy of complex containers
			{
				tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { tmp }, "calltmp");
			}

			m_builder->CreateStore(tmp, lhs_gep);
		}
		break;
	}
	default:
		Error(rhs.GetToken(), "Failed to store value.");
	}
}



//-----------------------------------------------------------------------------
void TValue::StoreAtIndex(TValue idx, TValue rhs)
{
	TType i0 = m_ttype.GetInternal(0);
	GetFromStorage(idx, rhs);

	if (!m_ttype.CanIndex())
	{
		Error(rhs.GetToken(), "Cannot index into this type.");
		return;
	}
	else if (!rhs.IsValid())
	{
		Error(rhs.GetToken(), "Cannot store invalid value.");
		return;
	}
	else if (!idx.IsValid())
	{
		Error(idx.GetToken(), "Cannot store at invalid index.");
		return;
	}

	if (m_ttype.IsVecAny())
	{
		rhs = rhs.CastToMatchImplicit(i0);
		if (i0.GetLiteralType() != rhs.m_ttype.GetLiteralType() ||
			i0.GetTy() != rhs.m_ttype.GetTy())
		{
			Error(rhs.GetToken(), "Cannot store mismatched type.");
			return;
		}
	}
	else if (m_ttype.IsMap())
	{
		TType i1 = m_ttype.GetInternal(1);
		rhs = rhs.CastToMatchImplicit(i1);

		if (i1.GetLiteralType() != rhs.m_ttype.GetLiteralType() ||
			i1.GetTy() != rhs.m_ttype.GetTy())
		{
			Error(rhs.GetToken(), "Cannot store mismatched type.");
			return;
		}
	}

	llvm::Value* zero = m_builder->getInt32(0);

	if (m_ttype.IsVecFixed())
	{
		if (!i0.IsUDT())
		{
			llvm::Value* gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, idx.m_value }, "geptmp");
			if (i0.IsString())
			{
				llvm::Value* tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { rhs.m_value }, "str_clone");
				m_builder->CreateStore(tmp, gep);
			}
			else
			{
				m_builder->CreateStore(rhs.m_value, gep);
			}
		}/*
		else
		{
			TStruct tstruc = Environment::GetStruct(m_token, m_vec_type_name);

			std::vector<std::string>& member_names = tstruc.GetMemberNames();
			std::vector<TValue>& members = tstruc.GetMemberVec();

			for (size_t i = 0; i < members.size(); ++i)
			{
				llvm::Value* lhs_gep = m_builder->CreateGEP(m_ty, m_value, { idx.m_value, m_builder->getInt32(i) }, member_names[i] + "_lhs");
				llvm::Value* rhs_gep = m_builder->CreateGEP(rhs.m_ty, rhs.m_value, { m_builder->getInt32(0), m_builder->getInt32(i) }, member_names[i] + "_rhs");
				llvm::Value* tmp = m_builder->CreateLoad(members[i].GetTy(), rhs_gep);

				// shallow copy
				m_builder->CreateStore(tmp, lhs_gep);
			}
		}*/
	}
	else if (m_ttype.IsVecDynamic())
	{
		llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");
		m_builder->CreateCall(m_module->getFunction("__dyn_vec_assert_idx"), { dstPtr, idx.m_value }, "calltmp");
		llvm::Value* dataPtr = m_builder->CreateCall(m_module->getFunction("__dyn_vec_data_ptr"), { dstPtr }, "calltmp");
		llvm::Value* gep = m_builder->CreateGEP(i0.GetTy(), dataPtr, { idx.m_value }, "geptmp");

		llvm::Value* tmp = rhs.m_value;
		if (i0.IsString())
		{
			tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { tmp }, "calltmp");
		}

		m_builder->CreateStore(tmp, gep);
	}
	else if (m_ttype.IsMap())
	{
		TType i1 = m_ttype.GetInternal(1);
		if (!rhs.IsString()) rhs = rhs.MakeStorage(); // make sure we have a pointer to value for set function

		llvm::Value* tmp = rhs.m_value;
		if (rhs.IsString()) // clone for storage
		{
			tmp = m_builder->CreateCall(m_module->getFunction("__new_string_from_string"), { tmp }, "calltmp");
		}

		llvm::Value* dstPtr = m_builder->CreateLoad(m_builder->getPtrTy(), m_value, "loadtmp");

		if (i0.IsInteger())
		{
			if (!idx.IsInteger())
			{
				Error(m_token, "Expected integer index for map.");
				return;
			}
			m_builder->CreateCall(m_module->getFunction("__set_map_val_at_i64"), { dstPtr, idx.CastToInt(64).Value(), tmp }, "calltmp");
		}
		else if (i0.IsString())
		{
			if (!idx.IsString())
			{
				Error(m_token, "Expected string index for map.");
				return;
			}
			m_builder->CreateCall(m_module->getFunction("__set_map_val_at_str"), { dstPtr, idx.m_value, tmp }, "calltmp");
		}
	}
	else if (m_ttype.IsTuple())
	{
		if (!idx.IsInteger())
		{
			Error(m_token, "Expected integer index for tuple.");
			return;
		}
		if (!idx.IsConstant())
		{
			Error(m_token, "Index must be constant.");
			return;
		}

		// determine index
		int index = 0;
		llvm::Value* v = idx.Value();
		if (llvm::isa<llvm::GlobalValue>(v))
		{
			llvm::GlobalVariable* gv = llvm::cast<llvm::GlobalVariable>(v);
			if (gv)
			{
				llvm::Constant* c = gv->getInitializer();
				if (c)
				{
					llvm::ConstantInt* ci = llvm::cast<llvm::ConstantInt>(c);
					if (ci) index = ci->getSExtValue();
				}
			}
		}
		else
		{
			index = idx.GetToken()->IntValue();
		}

		if (index >= m_ttype.GetInternalCount() || index < 0)
		{
			Error(m_token, "Tuple index out of bounds.");
			return;
		}

		TType i1 = m_ttype.GetInternal(index);
		rhs = rhs.CastToMatchImplicit(i1);

		llvm::Value* vidx = m_builder->getInt32(index);
		llvm::Value* zero = m_builder->getInt32(0);
		llvm::Value* gep = m_builder->CreateGEP(m_ttype.GetTy(), m_value, { zero, vidx }, "geptmp");
		m_builder->CreateStore(rhs.m_value, gep);
	}
	else
	{
		Error(m_token, "Failed to store into vector index.");
	}

}
