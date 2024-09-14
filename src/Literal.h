#ifndef LITERAL_H
#define LITERAL_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdint.h>
#include <cfloat>
#include <cmath>

#ifndef NO_RAYLIB
#include <raylib.h>
#endif

class FunctionStmt;
class StructStmt;
class FunctorExpr;
class Interpreter;
class Literal;

typedef std::vector<Literal> LiteralList;

struct EnumLiteral
{
	std::string enumValue;
	EnumLiteral() {}
	EnumLiteral(std::string v) : enumValue(v) {}
};

struct FunctorLiteral
{
};

class Literal
{
public:
	enum LiteralTypeEnum
	{
		LITERAL_TYPE_INVALID,
		LITERAL_TYPE_DOUBLE,
		LITERAL_TYPE_INTEGER,
		LITERAL_TYPE_STRING,
		LITERAL_TYPE_BOOL,
		LITERAL_TYPE_ENUM,
		LITERAL_TYPE_RANGE,
		LITERAL_TYPE_VEC,
		LITERAL_TYPE_ANONYMOUS, // used for destructuring comma separated values
		LITERAL_TYPE_FUNCTION,
		LITERAL_TYPE_TT_FUNCTION,
		LITERAL_TYPE_TT_STRUCT,
		LITERAL_TYPE_FUNCTOR,
		
		// raylib custom
		LITERAL_TYPE_FONT,
		LITERAL_TYPE_IMAGE,
		LITERAL_TYPE_TEXTURE,
		LITERAL_TYPE_SOUND,
	};

	Literal()
	{
		m_type = LITERAL_TYPE_INVALID;
		m_isInstance = false;
	}

	Literal(double val)
	{
		m_doubleValue = val;
		m_intValue = int32_t(val);
		m_type = LITERAL_TYPE_DOUBLE;
	}

	Literal(int32_t val)
	{
		m_intValue = val;
		m_doubleValue = double(val);
		m_type = LITERAL_TYPE_INTEGER;
	}

	Literal(std::string val)
	{
		m_stringValue = val;
		m_type = LITERAL_TYPE_STRING;
	}

	Literal(EnumLiteral val)
	{
		m_enumValue = val;
		m_type = LITERAL_TYPE_ENUM;
	}

	Literal(FunctorLiteral val)
	{
		m_functorValue = val;
		m_type = LITERAL_TYPE_FUNCTOR;
		m_functorExpr = nullptr;
	}

	Literal(bool val)
	{
		m_boolValue = val;
		m_type = LITERAL_TYPE_BOOL;
	}

	Literal(int32_t lval, int32_t rval)
	{
		m_leftValue = lval;
		m_rightValue = rval;
		m_type = LITERAL_TYPE_RANGE;
	}



	Literal(std::vector<bool> val)
	{
		m_vecValue_b = val;
		m_type = LITERAL_TYPE_VEC;
		m_vecType = LITERAL_TYPE_BOOL;
	}
	Literal(std::vector<int32_t> val)
	{
		m_vecValue_i = val;
		m_type = LITERAL_TYPE_VEC;
		m_vecType = LITERAL_TYPE_INTEGER;
	}
	Literal(std::vector<double> val)
	{
		m_vecValue_d = val;
		m_type = LITERAL_TYPE_VEC;
		m_vecType = LITERAL_TYPE_DOUBLE;
	}
	Literal(std::vector<std::string> val)
	{
		m_vecValue_s = val;
		m_type = LITERAL_TYPE_VEC;
		m_vecType = LITERAL_TYPE_STRING;
	}
	Literal(std::vector<EnumLiteral> val)
	{
		m_vecValue_e = val;
		m_type = LITERAL_TYPE_VEC;
		m_vecType = LITERAL_TYPE_ENUM;
	}
	Literal(std::vector<Literal> val, LiteralTypeEnum vecType)
	{
		m_vecValue_u = val;
		m_type = LITERAL_TYPE_VEC;
		m_vecType = vecType;
	}
	

#ifndef NO_RAYLIB
	// raylib custom
	Literal(Font font)
	{
		m_font = font;
		m_type = LITERAL_TYPE_FONT;
	}
	Literal(Image tex)
	{
		m_image = tex;
		m_type = LITERAL_TYPE_IMAGE;
	}
	Literal(Texture2D tex)
	{
		m_texture = tex;
		m_type = LITERAL_TYPE_TEXTURE;
	}
	Literal(Sound tex)
	{
		m_sound = tex;
		m_type = LITERAL_TYPE_SOUND;
	}
#endif

	Literal Call(Interpreter* interpreter, LiteralList args);

	size_t Arity() { return m_arity; }

	bool IsCallable() const { return m_type == LITERAL_TYPE_FUNCTION || m_type == LITERAL_TYPE_TT_FUNCTION || m_type == LITERAL_TYPE_TT_STRUCT || m_type == LITERAL_TYPE_FUNCTOR; }
	bool ExplicitArgs() const { return m_explicitArgs; }
	
	bool IsDouble() const { return m_type == LITERAL_TYPE_DOUBLE; }
	bool IsInt() const { return m_type == LITERAL_TYPE_INTEGER; }
	bool IsNumeric() const { return (m_type == LITERAL_TYPE_DOUBLE || m_type == LITERAL_TYPE_INTEGER); }
	bool IsString() const { return m_type == LITERAL_TYPE_STRING; }
	bool IsEnum() const { return m_type == LITERAL_TYPE_ENUM; }
	bool IsBool() const { return m_type == LITERAL_TYPE_BOOL; }
	bool IsFunctor() const { return m_type == LITERAL_TYPE_FUNCTOR; }
	bool IsRange() const { return m_type == LITERAL_TYPE_RANGE; }
	bool IsInvalid() const { return m_type == LITERAL_TYPE_INVALID; }
	bool IsVector() const { return m_type == LITERAL_TYPE_VEC; }
	bool IsInstance() const {
		return (m_type == LITERAL_TYPE_TT_STRUCT && m_isInstance);
	}
	bool IsFunctionDef() const { return m_type == LITERAL_TYPE_TT_FUNCTION || m_type == LITERAL_TYPE_FUNCTION; }
	bool IsStructDef() const { return m_type == LITERAL_TYPE_TT_STRUCT && !m_isInstance; }

	// ralylib custom
	bool IsFont() const { return m_type == LITERAL_TYPE_FONT; }
	bool IsImage() const { return m_type == LITERAL_TYPE_IMAGE; }
	bool IsTexture() const { return m_type == LITERAL_TYPE_TEXTURE; }
	bool IsSound() const { return m_type == LITERAL_TYPE_SOUND; }

	int32_t Len() const
	{
		switch (m_type)
		{
		case LITERAL_TYPE_VEC:
			if (LITERAL_TYPE_BOOL == m_vecType) return m_vecValue_b.size();
			if (LITERAL_TYPE_INTEGER == m_vecType) return m_vecValue_i.size();
			if (LITERAL_TYPE_DOUBLE == m_vecType) return m_vecValue_d.size();
			if (LITERAL_TYPE_STRING == m_vecType) return m_vecValue_s.size();
			if (LITERAL_TYPE_ENUM == m_vecType) return m_vecValue_e.size();
			if (LITERAL_TYPE_TT_STRUCT == m_vecType) return m_vecValue_u.size();
			break;

		case LITERAL_TYPE_STRING:
			return m_stringValue.size();

		case LITERAL_TYPE_BOOL:
		case LITERAL_TYPE_DOUBLE:
		case LITERAL_TYPE_INTEGER:
			return 1;

		}
		return 0;
	}

	LiteralTypeEnum GetType() const { return m_type; }
	
	bool BoolValue() const { return m_boolValue; }
	std::string StringValue() const { return m_stringValue; }
	EnumLiteral EnumValue() const { return m_enumValue; }
	double DoubleValue() const{ return m_doubleValue; }
	int32_t IntValue() const{ return m_intValue; }
	int32_t LeftValue() const{ return m_leftValue; }
	int32_t RightValue() const{ return m_rightValue; }

#ifndef NO_RAYLIB
	// ralylib custom
	const Font& FontValue() const { return m_font; }
	const Texture& TextureValue() const { return m_texture; }
	const Sound& SoundValue() const { return m_sound; }
	Image& ImageValue() { return m_image; }
#endif
	
	std::vector<bool> VecValue_B() const { return m_vecValue_b; }
	bool VecValueAt_B(size_t i) const { return m_vecValue_b[i]; }
	std::vector<int32_t> VecValue_I() const { return m_vecValue_i; }
	int32_t VecValueAt_I(size_t i) const { return m_vecValue_i[i]; }
	std::vector<double> VecValue_D() const { return m_vecValue_d; }
	double VecValueAt_D(size_t i) const { return m_vecValue_d[i]; }
	std::vector<std::string> VecValue_S() const { return m_vecValue_s; }
	std::string VecValueAt_S(size_t i) const { return m_vecValue_s[i]; }
	std::vector<EnumLiteral> VecValue_E() const { return m_vecValue_e; }
	EnumLiteral VecValueAt_E(size_t i) const { return m_vecValue_e[i]; }
	std::vector<Literal> VecValue_U() const { return m_vecValue_u; }
	Literal VecValueAt_U(size_t i) const { return m_vecValue_u[i]; }
	
	LiteralTypeEnum GetVecType() const { return m_vecType; }
	bool IsVecBool() const { return LITERAL_TYPE_BOOL == m_vecType; }
	bool IsVecInteger() const { return LITERAL_TYPE_INTEGER == m_vecType; }
	bool IsVecDouble() const { return LITERAL_TYPE_DOUBLE == m_vecType; }
	bool IsVecString() const { return LITERAL_TYPE_STRING == m_vecType; }
	bool IsVecEnum() const { return LITERAL_TYPE_ENUM == m_vecType; }
	bool IsVecStruct() const { return LITERAL_TYPE_TT_STRUCT == m_vecType; }
	bool IsVecAnonymous() const { return LITERAL_TYPE_ANONYMOUS == m_vecType; }

	Literal GetParameter(const std::string& name);
	bool SetParameter(const std::string& name, Literal value, size_t index);

	void SetCallable(FunctionStmt* stmt);
	void SetCallable(StructStmt* stmt);
	void SetCallable(FunctorExpr* expr);

	void SetCallable(int nArgs, std::function<Literal(LiteralList args)> ftn, std::string fqns, bool explicitArgs = true)
	{
		m_explicitArgs = explicitArgs;
		m_arity = nArgs;
		m_ftn = ftn;
		m_type = LITERAL_TYPE_FUNCTION;
		m_fqns = fqns;
	}

	// set values in arrays
	void SetValueAt(bool value, size_t index)
	{
		m_vecValue_b[index] = value;
	}
	void SetValueAt(int32_t value, size_t index)
	{
		m_vecValue_i[index] = value;
	}
	void SetValueAt(double value, size_t index)
	{
		m_vecValue_d[index] = value;
	}
	void SetValueAt(std::string value, size_t index)
	{
		m_vecValue_s[index] = value;
	}
	void SetValueAt(EnumLiteral value, size_t index)
	{
		m_vecValue_e[index] = value;
	}
	void SetValueAt(Literal value, size_t index)
	{
		m_vecValue_u[index] = value;
	}


	std::string ToString() const;

	bool Equals(const Literal& val) const
	{
		if (val.IsDouble()) return Equals(val.DoubleValue());
		if (val.IsInt()) return Equals(val.IntValue());
		if (val.IsString()) return Equals(val.StringValue());
		if (val.IsEnum()) return Equals(val.EnumValue());
		if (val.IsBool()) return Equals(val.BoolValue());

		return false;
	}

	bool Equals(double val) const
	{
		if (!IsDouble() && !IsInt()) return false;
		
		double rhs = m_doubleValue;
		if (IsInt()) rhs = double(m_intValue);
		
		if (std::fabs(val - rhs) > DBL_MIN) return false;
		
		return true;
	}

	bool Equals(int32_t val) const
	{
		if (!IsDouble() && !IsInt()) return false;

		if (IsDouble())
		{
			if (std::fabs(double(val) - m_doubleValue) > DBL_MIN) return false;
			return true;
		}
		
		return m_intValue == val;
	}

	bool Equals(std::string val) const
	{
		if (!IsString()) return false;
		if (m_stringValue.compare(val) != 0) return false;
		return true;
	}

	bool Equals(EnumLiteral val) const
	{
		if (!IsEnum()) return false;
		if (m_enumValue.enumValue.compare(val.enumValue) != 0) return false;
		return true;
	}

	bool Equals(bool val) const
	{
		if (!IsBool()) return false;
		return m_boolValue == val;
	}

private:

	double m_doubleValue;
	int32_t m_intValue;
	int32_t m_leftValue;
	int32_t m_rightValue;
	std::string m_stringValue;
	EnumLiteral m_enumValue;
	FunctorLiteral m_functorValue;
	std::vector<bool> m_vecValue_b;
	std::vector<int32_t> m_vecValue_i;
	std::vector<double> m_vecValue_d;
	std::vector<std::string> m_vecValue_s;
	std::vector<EnumLiteral> m_vecValue_e;
	std::vector<Literal> m_vecValue_u;
	bool m_boolValue;
	bool m_explicitArgs;
	LiteralTypeEnum m_type;
	LiteralTypeEnum m_vecType;
	bool m_isInstance;

	int m_arity;
	std::function<Literal(LiteralList args)> m_ftn;
	FunctionStmt* m_ftnStmt;
	FunctorExpr* m_functorExpr;
	
	StructStmt* m_stuctStmt;
	std::map<std::string, Literal> m_parameters;

	std::string m_fqns;
	

#ifndef NO_RAYLIB
	// raylib custom
	Font m_font;
	Texture2D m_texture;
	Image m_image;
	Sound m_sound;
#endif

};



#endif // LITERAL_H