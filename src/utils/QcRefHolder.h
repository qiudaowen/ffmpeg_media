#pragma once

template <class T>
class QcRefHolder
{
public:
	QcRefHolder(T* p = NULL)
		: m_p(NULL)
	{
		set(p);
	}
	QcRefHolder(const QcRefHolder<T>& p)
		: m_p(NULL)
	{
		*this = p;
	}
	~QcRefHolder()
	{
		set(NULL);
	}
	void operator=(T* p)
	{
		set(p);
	}
	void operator=(const QcRefHolder<T>& p)
	{
		set(p.m_p);
	}
	bool operator==(const QcRefHolder<T>& p) const
	{
		return m_p == p.m_p;
	}
	operator bool() const
	{
		return m_p != NULL;
	}
	operator T*()
	{
		return m_p;
	}
	T* operator-> () const// never throws
	{
		return m_p;
	}
	T* get()
	{
		return m_p;
	}
	const T* get() const// never throws
	{
		return m_p;
	}

	template<class P>
	P* operator-> () const// never throws
	{
		return static_cast<P*>(m_p);
	}
	template<class P>
	operator P*() const// never throws
	{
		return static_cast<P*>(m_p);
	}
protected:
	void set(T* p)
	{
		if (p)
			p->addRef();
		if (m_p)
			m_p->release();
		m_p = p;
	}
protected:
	T* m_p;
};