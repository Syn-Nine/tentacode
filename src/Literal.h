#ifndef LITERAL_H
#define LITERAL_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdint.h>
#include <cfloat>
#include <cmath>
#include <memory>

#ifndef NO_RAYLIB
#include <raylib.h>
#endif

class FunctionStmt;
class StructStmt;
class FunctorExpr;
class Interpreter;
class Literal;

typedef std::vector<Literal> LiteralList;

enum LiteralTypeEnum
{
	LITERAL_TYPE_INVALID,
	LITERAL_TYPE_FLOAT,
	LITERAL_TYPE_INTEGER,
	LITERAL_TYPE_STRING,
	LITERAL_TYPE_BOOL,
	LITERAL_TYPE_ENUM,
	LITERAL_TYPE_VEC_DYNAMIC,
	LITERAL_TYPE_VEC_FIXED,
	LITERAL_TYPE_POINTER,
	LITERAL_TYPE_UDT,
};

struct EnumLiteral
{
	std::string enumValue;
	EnumLiteral() {}
	EnumLiteral(std::string v) : enumValue(v) {}
};

class Literal
{
public:
	

	Literal()
	{
		m_type = LITERAL_TYPE_INVALID;
		//m_isInstance = false;
	}

	Literal(double val)
	{
		m_doubleValue = val;
		m_intValue = int32_t(val);
		m_type = LITERAL_TYPE_FLOAT;
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

	Literal(bool val)
	{
		m_boolValue = val;
		m_type = LITERAL_TYPE_BOOL;
	}

	LiteralTypeEnum GetType() const { return m_type; }
	
	bool BoolValue() const { return m_boolValue; }
	std::string StringValue() const { return m_stringValue; }
	EnumLiteral EnumValue() const { return m_enumValue; }
	double DoubleValue() const{ return m_doubleValue; }
	int32_t IntValue() const{ return m_intValue; }


private:

	double m_doubleValue;
	int32_t m_intValue;
	std::string m_stringValue;
	EnumLiteral m_enumValue;
	bool m_boolValue;
	LiteralTypeEnum m_type;
	LiteralTypeEnum m_vecType;
	
};

#endif // LITERAL_H