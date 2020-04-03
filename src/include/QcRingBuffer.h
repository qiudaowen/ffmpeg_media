#pragma once
#include <algorithm>
#include <malloc.h>

class QcRingBuffer
{
public:
	QcRingBuffer(int capacity)
		: m_read_index(0)
		, m_write_index(0)
		, m_size(0)
		, m_capacity(0)
		, m_data(NULL)
	{
		if (capacity)
		{
			ensureCapacity(capacity);
		}
	}
	~QcRingBuffer()
	{
		if (m_data)
			free(m_data);
	}

	void clear() { m_size = 0; m_read_index = 0;  m_write_index = 0; }
	size_t size() const { return m_size; }
	size_t capacity() const { return m_capacity; }
    size_t usableSize() const { return m_capacity - m_size;}
	// Return number of bytes written.
	size_t write(const char *data, size_t bytes, bool bAutoExpand = false)
	{
		if (bAutoExpand)
		{
			ensureCapacity(m_size + bytes);
		}
		if (bytes == 0) return 0;

		size_t capacity = m_capacity;
		size_t bytes_to_write = min(bytes, capacity - m_size);

		// Write in a single step
		if (bytes_to_write <= capacity - m_write_index)
		{
			memcpy(m_data + m_write_index, data, bytes_to_write);
			m_write_index += bytes_to_write;
			if (m_write_index == capacity) m_write_index = 0;
		}
		// Write in two steps
		else
		{
			size_t size_1 = capacity - m_write_index;
			memcpy(m_data + m_write_index, data, size_1);
			size_t size_2 = bytes_to_write - size_1;
			memcpy(m_data, data + size_1, size_2);
			m_write_index = size_2;
		}

		m_size += bytes_to_write;
		return bytes_to_write;
	}
	// Return number of bytes read.
	size_t read(char *data, size_t bytes)
	{
		if (bytes == 0) return 0;

		size_t capacity = m_capacity;
		size_t bytes_to_read = min(bytes, m_size);

		// Read in a single step
		if (bytes_to_read <= capacity - m_read_index)
		{
			if (data)
				memcpy(data, m_data + m_read_index, bytes_to_read);
			m_read_index += bytes_to_read;
			if (m_read_index == capacity) m_read_index = 0;
		}
		// Read in two steps
		else
		{
			size_t size_1 = capacity - m_read_index;
			size_t size_2 = bytes_to_read - size_1;
			if (data)
			{
				memcpy(data, m_data + m_read_index, size_1);
				memcpy(data + size_1, m_data, size_2);
			}
			m_read_index = size_2;
		}

		m_size -= bytes_to_read;
		return bytes_to_read;
	}

	template<typename T> size_t write(T data)
	{
		int iRet = write((const char*)&data, sizeof(data), true);
		QmAssert(iRet == sizeof(T));
		return iRet;
	}
	template<typename T> size_t read(T& data)
	{
		return read((char*)&data, sizeof(T));
	}

	void ensureCapacity(int capacity)
	{
		if (capacity > (int)m_capacity)
		{
			capacity = ((capacity) / 4 + 1) * 4;

			m_data = (char*)realloc(m_data, capacity);
			if (m_size > 0 && m_read_index > m_write_index)
			{
				size_t difference = capacity - m_capacity;
				char* srcData = m_data + m_read_index;
				memmove(srcData + difference, srcData, m_capacity - m_read_index);
				m_read_index += difference;
			}
			m_capacity = capacity;
		}
	}
	void swap(QcRingBuffer& other)
	{
		std::swap(m_read_index, other.m_read_index);
		std::swap(m_write_index, other.m_write_index);
		std::swap(m_size, other.m_size);
		std::swap(m_capacity, other.m_capacity);
		std::swap(m_data, other.m_data);
	}
private:
	size_t m_read_index;
	size_t m_write_index;
	size_t m_size;
	size_t m_capacity;
	char *m_data;
};
namespace std
{
	template<> inline void swap(QcRingBuffer& one, QcRingBuffer& two)
	{
		one.swap(two);
	}
}

