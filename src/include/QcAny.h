#pragma once

#include "QmMacro.h"
#include <map>

class QcAny
{
public: // structors

	QcAny()
		: content(0)
	{
	}

	QcAny(const char* p)
		: content(new holder<std::string>(p))
	{
	}

#define QmDefAnyType(type, type2) \
	QcAny(type val) \
		: content(new holder<type2>(val)) \
	{}
	QmDefAnyType(bool, int)
	QmDefAnyType(char, int)
	QmDefAnyType(unsigned char, int)
	QmDefAnyType(short, int)
	QmDefAnyType(unsigned short, int)
	QmDefAnyType(int, int)
	QmDefAnyType(unsigned int, int)
	QmDefAnyType(long, int)
	QmDefAnyType(unsigned long, int)

	QmDefAnyType(long long, long long)
	QmDefAnyType(unsigned long long, long long)
	QmDefAnyType(float, float)
	QmDefAnyType(double, double)

	template<typename ValueType>
	QcAny(const ValueType & value)
		: content(new holder<ValueType>(value))
	{
	}

	QcAny(const QcAny & other)
		: content(other.content ? other.content->clone() : 0)
	{
	}

	~QcAny()
	{
		delete content;
	}

public: // modifiers

	QcAny & swap(QcAny & rhs)
	{
		std::swap(content, rhs.content);
		return *this;
	}

	template<typename ValueType>
	QcAny & operator=(const ValueType & rhs)
	{
		QcAny(rhs).swap(*this);
		return *this;
	}

	//Add by Qwen;
	template<typename ValueType>
	operator const ValueType&() const
	{
		const std::type_info& typeInfo = type();
		if (typeInfo == typeid(ValueType))
			return (static_cast<holder<ValueType> *>(content)->held);

		QmAssert(false);
		static ValueType value;
		return value;
	}
	
	operator bool() const 
	{ 
		const std::type_info& typeInfo = type();
		if (typeInfo == typeid(bool)) 
			return (static_cast<holder<bool> *>(content)->held); 
		if (typeInfo == typeid(int)) 
			return (static_cast<holder<int> *>(content)->held) != 0; 
		if (typeInfo == typeid(long long)) 
			return (static_cast<holder<long long> *>(content)->held) != 0; 
		QmAssert(false);
		return false;
	}
#define QmAnyToType(T) \
	operator T() const \
	{ \
		const std::type_info& typeInfo = type();\
		if (typeInfo == typeid(T)) \
			return (static_cast<holder<T> *>(content)->held); \
		if (typeInfo == typeid(int)) \
			return (T)(static_cast<holder<int> *>(content)->held); \
		if (typeInfo == typeid(long long)) \
			return (T)(static_cast<holder<long long> *>(content)->held); \
		if (typeInfo == typeid(float)) \
			return (T)(static_cast<holder<float> *>(content)->held); \
		if (typeInfo == typeid(double)) \
			return (T)(static_cast<holder<double> *>(content)->held); \
		static T value = 0; \
		QmAssert(false); \
		return value; \
	}
	QmAnyToType(char)
	QmAnyToType(unsigned char)
	QmAnyToType(short)
	QmAnyToType(unsigned short)
	QmAnyToType(int)
	QmAnyToType(unsigned int)
	QmAnyToType(long)
	QmAnyToType(unsigned long)
	QmAnyToType(long long)
	QmAnyToType(unsigned long long)
	QmAnyToType(float)
	QmAnyToType(double)

	QcAny & operator=(QcAny rhs)
	{
		rhs.swap(*this);
		return *this;
	}

public: // queries
	bool empty() const
	{
		return !content;
	}

	const std::type_info & type() const
	{
		return content ? content->type() : typeid(void);
	}
private:
	class placeholder
	{
	public: // structors

		virtual ~placeholder()
		{
		}

	public: // queries

		virtual const std::type_info & type() const = 0;

		virtual placeholder * clone() const = 0;

	};

	template<typename ValueType>
	class holder : public placeholder
	{
	public: // structors

		holder(const ValueType & value)
			: held(value)
		{
		}

	public: // queries

		virtual const std::type_info & type() const
		{
			return typeid(ValueType);
		}

		virtual placeholder * clone() const
		{
			return new holder(held);
		}

	public: // representation

		ValueType held;

	private: // intentionally left unimplemented
		holder & operator=(const holder &);
	};
private: // representation
	placeholder * content;
};

typedef QcAny QcVariant;
typedef std::map<std::string, QcAny> QcAnyMap;

template<class T>
inline T getAnyValue(const char* key, const QcAnyMap& anyMap, const T& defaultValue)
{
	QcAnyMap::const_iterator iter(anyMap.find(key));
	if (iter != anyMap.end())
	{
		return iter->second;
	}
	return defaultValue;
}
