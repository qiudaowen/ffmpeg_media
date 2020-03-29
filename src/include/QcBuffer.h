#pragma once 
#include <assert.h>
#include <utility>
#include <malloc.h>
#include <stdint.h>

class QcBuffer
{
public:
	QcBuffer()
		: m_pData(NULL)
		, m_uDataSize(0)
		, m_uBufferSize(0)
		, m_bExternalBuffer(false)
	{
	}

	QcBuffer::~QcBuffer()
	{
		clear();
	}

	void clear()
	{
		if (!m_bExternalBuffer && m_pData)
			_aligned_free(m_pData);
		m_pData = NULL;
		m_uDataSize = 0;
		m_uBufferSize = 0;
		m_bExternalBuffer = false;
	}
	uint32_t bufferSize() const{ return m_uBufferSize;}
	uint8_t* data() const { return m_pData; }
	void checkBufferSize(uint32_t uSize)
	{
		if (m_uBufferSize < uSize)
		{
			realloc(uSize);
		}
	}

	void write(const void* data, int len)
	{
		append(data, len);
	}
	void append(const void* pdata, int len)
	{
		if (len > 0)
		{
			if (m_uDataSize + len > m_uBufferSize)
			{
				checkBufferSize(m_uDataSize + len);
			}
			memcpy(data() + m_uDataSize, pdata, len);
			m_uDataSize += len;
		}
	}
	uint32_t getDataSize() const { return m_uDataSize; }
	uint32_t size() const { return m_uDataSize; }
	bool setDataSize(uint32_t size)
	{
		assert(size >= 0 && size <= m_uBufferSize);
		if (size < 0 || size > m_uBufferSize)
			return false;

		m_uDataSize = size;
		return true;
	}
	void attachExternelBuffer(uint8_t *pBuf, uint32_t nSize)
	{
		if (!m_bExternalBuffer && m_pData)
			_aligned_free(m_pData);

		m_pData = pBuf;
		m_uDataSize = m_uBufferSize = nSize;
		m_bExternalBuffer = true;
	}

	bool isEmpty() const
	{
		return (m_pData == NULL || m_uDataSize == 0);
	}

	void swap(QcBuffer& buffer)
	{
		std::swap(buffer.m_bExternalBuffer, m_bExternalBuffer);
		std::swap(buffer.m_pData, m_pData);
		std::swap(buffer.m_uBufferSize, m_uBufferSize);
		std::swap(buffer.m_uDataSize, m_uDataSize);
	}
protected:
	void realloc(uint32_t size)
	{
		void* pTemp = m_pData;
		if (m_bExternalBuffer)
			m_pData = NULL;
		m_pData = (uint8_t*)_aligned_realloc(m_pData, size, 16);
		if (m_bExternalBuffer && m_uDataSize)
			memcpy(m_pData, pTemp, m_uDataSize);

		if (m_pData)
		{
			m_uBufferSize = size;
		}
		else
		{
			m_uDataSize = 0;
			m_uBufferSize = 0;
		}
		m_bExternalBuffer = false;
	}
private:
	bool		m_bExternalBuffer;
	uint8_t *		m_pData;
	uint32_t			m_uBufferSize;
	uint32_t			m_uDataSize;
};
template<class T>
class QcBufferEx : public QcBuffer
{
public:
	void swap(QcBufferEx<T>& buffer)
	{
		QcBuffer::swap(buffer);
		std::swap(buffer.m_first, m_first);
	}
	T m_first;
};

class QcMediaBuffer : public QcBuffer
{
public:
    void swap(QcMediaBuffer& buffer)
    {
        QcBuffer::swap(buffer);
        std::swap(buffer.m_tm, m_tm);
        std::swap(buffer.m_flag, m_flag);
        std::swap(buffer.m_type, m_type);
    }
    uint64_t m_tm;
    int m_flag;
    int m_type;
};


namespace std
{
	template<> inline void swap(QcBuffer& one, QcBuffer& two)
	{
		one.swap(two);
	}
	template<class T> inline void swap(QcBufferEx<T>& one, QcBufferEx<T>& two)
	{
		one.swap(two);
	}
    template<> inline void swap(QcMediaBuffer& one, QcMediaBuffer& two)
    {
        one.swap(two);
    }
}

