#include "Literal.h"
#include "Statements.h"
#include "Expressions.h"

std::string Literal::ToString() const
{
	switch (m_type)
	{
	case LITERAL_TYPE_INVALID:
		return "<Literal Invalid>";

	case LITERAL_TYPE_BOOL:
		return m_boolValue ? "true" : "false";

	case LITERAL_TYPE_DOUBLE:
		return std::to_string(m_doubleValue);

	case LITERAL_TYPE_INTEGER:
		return std::to_string(m_intValue);

	case LITERAL_TYPE_STRING: // intentional fall-through
	default:
		return m_stringValue;
	}
}
