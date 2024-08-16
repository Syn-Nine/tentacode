#include "Literal.h"
#include "Statements.h"
#include "Environment.h"
#include "Interpreter.h"

Literal Literal::Call(Interpreter* interpreter, LiteralList args)
{
	if (m_type != LITERAL_TYPE_FUNCTION &&
		m_type != LITERAL_TYPE_TT_FUNCTION &&
		m_type != LITERAL_TYPE_TT_STRUCT)
		return Literal();

	if (LITERAL_TYPE_FUNCTION == m_type)
	{
		return m_ftn(args);
	}
	else if (LITERAL_TYPE_TT_STRUCT == m_type)
	{
		Literal ret = Literal(*this);
		ret.m_isInstance = true;

		// instantialize internal parameters
		StmtList* vars = ret.m_stuctStmt->GetVars();
		for (auto& v : *vars)
		{
			if (STATEMENT_VAR == v->GetType())
			{
				VarStmt* stmt = (VarStmt*)v;

				Literal value;
				TokenTypeEnum varType = stmt->VarType()->GetType();
				Literal::LiteralTypeEnum vecType = stmt->VarVecType();

				if (TOKEN_VAR_I32 == varType) value = Literal(int32_t(0));
				if (TOKEN_VAR_F32 == varType) value = Literal(0.0);
				if (TOKEN_VAR_STRING == varType) value = Literal(std::string(""));
				if (TOKEN_VAR_ENUM == varType) value = Literal(EnumLiteral());
				if (TOKEN_VAR_BOOL == varType) value = Literal(false);
				if (TOKEN_VAR_VEC == varType)
				{
					if (Literal::LITERAL_TYPE_BOOL == vecType)
						value = Literal(std::vector<bool>());
					else if (Literal::LITERAL_TYPE_INTEGER == vecType)
						value = Literal(std::vector<int32_t>());
					else if (Literal::LITERAL_TYPE_DOUBLE == vecType)
						value = Literal(std::vector<double>());
					else if (Literal::LITERAL_TYPE_STRING == vecType)
						value = Literal(std::vector<std::string>());
					else if (Literal::LITERAL_TYPE_ENUM == vecType)
						value = Literal(std::vector<EnumLiteral>());
					else if (Literal::LITERAL_TYPE_TT_STRUCT == vecType)
						value = Literal(std::vector<Literal>(), vecType);
				}

#ifndef NO_RAYLIB
				if (TOKEN_VAR_FONT == varType) value = Literal(Font());
				if (TOKEN_VAR_IMAGE == varType) value = Literal(Image());
				if (TOKEN_VAR_SOUND == varType) value = Literal(Sound());
				if (TOKEN_VAR_TEXTURE == varType) value = Literal(Texture2D());
#endif

				// user defined type
				if (TOKEN_IDENTIFIER == varType)
				{
					Literal vtype = interpreter->GetGlobals()->Get(stmt->VarType(), m_fqns);

					if (!vtype.IsCallable())
					{
						printf("Invalid user defined type in variable declaration.\n");
					}
					else
					{
						value = vtype.Call(interpreter, LiteralList());
					}
				}

				ret.m_parameters.insert(std::make_pair(stmt->Operator()->Lexeme(), value));
			}
		}
		
		return ret;
	}
	else
	{
		Environment* env = new Environment(interpreter->GetGlobals(), interpreter->GetErrorHandler());

		auto params = m_ftnStmt->GetParams();

		for (size_t i = 0; i < args.size(); ++i)
		{
			env->Define(params.at(i).Lexeme(), args.at(i), m_fqns);
		}

		Literal ret;
		try
		{
			interpreter->ExecuteBlock(m_ftnStmt->GetBody(), env);
		}
		catch (Literal value)
		{
			ret = value;
		}

		return ret;
	}
}

Literal Literal::GetParameter(const std::string& name)
{
	if (0 != m_parameters.count(name)) return m_parameters.at(name);
	return Literal();
}

bool Literal::SetParameter(const std::string& name, Literal value, size_t index)
{
	if (0 == m_parameters.count(name)) return false;
	Literal& v = m_parameters.at(name);

	// check type casting -- DUPLICATE CODE from Environment->Assign()
	if (v.IsRange())
	{
		printf("Unable to assign range.\n");
	}
	else
	{
		if (v.IsInt() && value.IsDouble()) value = Literal(int32_t(value.DoubleValue()));
		if (v.IsDouble() && value.IsInt()) value = Literal(double(value.IntValue()));
		if (v.GetType() == value.GetType())
		{
			v = value;
		}
		else if (v.IsVector())
		{
			if (index < v.Len() && index != -1)
			{
				if (v.IsVecBool())
					v.SetValueAt(value.BoolValue(), index);
				else if (v.IsVecInteger())
					v.SetValueAt(value.IntValue(), index);
				else if (v.IsVecDouble())
					v.SetValueAt(value.DoubleValue(), index);
				else if (v.IsVecString())
					v.SetValueAt(value.StringValue(), index);
				else if (v.IsVecEnum())
					v.SetValueAt(value.EnumValue(), index);
				else if (v.IsVecStruct())
					v.SetValueAt(value, index);
			}
			else if (-1 == index)
			{
				printf("No vector index provided.\n");
			}
			else
			{
				printf("%s\n", ("Vector index [" + std::to_string(index) + "] out of bounds during assignment (Size: " + std::to_string(v.Len()) + ").").c_str());
			}
		}
		else
		{
			printf("%s\n", ("Unable to cast between types during assignment (" + v.ToString() + ", " + value.ToString() + ").\n").c_str());
		}
	}


	return true;
}


void Literal::SetCallable(FunctionStmt* stmt)
{
	m_ftnStmt = stmt;
	m_arity = stmt->GetParams().size();
	m_type = LITERAL_TYPE_TT_FUNCTION;
	m_fqns = stmt->FQNS();
}


void Literal::SetCallable(StructStmt* stmt)
{
	m_stuctStmt = stmt;
	m_arity = 0;
	m_type = LITERAL_TYPE_TT_STRUCT;
	m_fqns = stmt->FQNS();
}


std::string Literal::ToString() const
{
	switch (m_type)
	{
	case LITERAL_TYPE_INVALID:
		return "<Literal Invalid>";

	case LITERAL_TYPE_FUNCTION:
		return "<native ftn " + m_ftnStmt->Operator()->Lexeme() + ">";

	case LITERAL_TYPE_TT_FUNCTION:
		return "<def " + m_ftnStmt->Operator()->Lexeme() + ">";

	case LITERAL_TYPE_TT_STRUCT:
	{
		// destructure the structure
		std::string ret = "<struct " + m_stuctStmt->Operator()->Lexeme() + "\n";
		for (auto& value : m_parameters)
		{
			ret.append("  param: " + value.first + " = " + value.second.ToString() + ",\n");
		}
		ret.append("  >");
		return ret;
	}

	case LITERAL_TYPE_ENUM:
		return "<enum " + m_enumValue.enumValue + ">";

	case LITERAL_TYPE_BOOL:
		return m_boolValue ? "true" : "false";

	case LITERAL_TYPE_DOUBLE:
		return std::to_string(m_doubleValue);

	case LITERAL_TYPE_INTEGER:
		return std::to_string(m_intValue);

	case LITERAL_TYPE_RANGE:
		return "<Range [" + std::to_string(m_leftValue) + ", " + std::to_string(m_rightValue) + ")>";

	case LITERAL_TYPE_VEC:
	{
		std::string ret;
		switch (m_vecType)
		{
		case LITERAL_TYPE_BOOL:
			ret = "<Vec,bool,Size:" + std::to_string(m_vecValue_b.size()) + ">[";
			for (size_t i = 0; i < m_vecValue_b.size(); ++i)
			{
				if (0 == i)
				{
					std::string s = "false";
					if (m_vecValue_b[i]) s = "true";
					ret.append(s);
				}
				else
				{
					std::string s = "false";
					if (m_vecValue_b[i]) s = "true";
					ret.append(", " + s);
				}
			}
			break;
		case LITERAL_TYPE_INTEGER:
			ret = "<Vec,i32,Size:" + std::to_string(m_vecValue_i.size()) + ">[";
			for (size_t i = 0; i < m_vecValue_i.size(); ++i)
			{
				if (0 == i)
				{
					ret.append(std::to_string(m_vecValue_i[i]));
				}
				else
				{
					ret.append(", " + std::to_string(m_vecValue_i[i]));
				}
			}
			break;
		case LITERAL_TYPE_DOUBLE:
			ret = "<Vec,f32,Size:" + std::to_string(m_vecValue_d.size()) + ">[";
			for (size_t i = 0; i < m_vecValue_d.size(); ++i)
			{
				if (0 == i)
				{
					ret.append(std::to_string(m_vecValue_d[i]));
				}
				else
				{
					ret.append(", " + std::to_string(m_vecValue_d[i]));
				}
			}
			break;
		case LITERAL_TYPE_STRING:
			ret = "<Vec,string,Size:" + std::to_string(m_vecValue_s.size()) + ">[";
			for (size_t i = 0; i < m_vecValue_s.size(); ++i)
			{
				if (0 == i)
				{
					ret.append(m_vecValue_s[i]);
				}
				else
				{
					ret.append(", " + m_vecValue_s[i]);
				}
			}
			break;
		case LITERAL_TYPE_ENUM:
			ret = "<Vec,enum,Size:" + std::to_string(m_vecValue_e.size()) + ">[";
			for (size_t i = 0; i < m_vecValue_e.size(); ++i)
			{
				if (0 == i)
				{
					ret.append(m_vecValue_e[i].enumValue);
				}
				else
				{
					ret.append(", " + m_vecValue_e[i].enumValue);
				}
			}
			break;
		case LITERAL_TYPE_TT_STRUCT:
			ret = "<Vec,struct,Size:" + std::to_string(m_vecValue_u.size()) + ">[";
			for (size_t i = 0; i < m_vecValue_u.size(); ++i)
			{
				if (0 == i)
				{
					ret.append(m_vecValue_u[i].ToString());
				}
				else
				{
					ret.append(", " + m_vecValue_u[i].ToString());
				}
			}
			break;
		};
		
		return ret + "]";
	}

	// raylib custom
	case LITERAL_TYPE_FONT:
		return "<raylib Font>";

	case TOKEN_VAR_IMAGE:
		return "<raylib Image>";

	case LITERAL_TYPE_TEXTURE:
		return "<raylib Texture2D>";

	case LITERAL_TYPE_SOUND:
		return "<raylib Sound>";

	case LITERAL_TYPE_STRING: // intentional fall-through
	default:
		return m_stringValue;
	}
}
