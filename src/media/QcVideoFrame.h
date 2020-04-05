#pragma once

#include "QcBuffer.h"
#include "QsVideodef.h"

class QcVideoFrame
{
public:
	QcVideoFrame()
	{
		memset(m_dataSlice, 0, sizeof(m_dataSlice));
		memset(m_lineSize, 0, sizeof(m_lineSize));
		m_width = 0;
		m_height = 0;
		m_format = 0;
		m_sampleTime = 0;
	}
	QcVideoFrame(int w, int h, int format)
	{
		m_sampleTime = 0;
		resize(w, h, format);
	}
	void resize(int w, int h, int format = 0)
	{
		if (format == 0)
			format = m_format;
		m_buffer.checkBufferSize((uint32_t)video::CalBufNeedSize(w, h, format));
		attach(w, h, format, m_buffer.data());
	}
	uint32_t CalBufSize() const
	{
		return video::CalBufNeedSize(m_width, m_height, m_format);
	}
	void attach(int w, int h, int format, uint8_t* plane[QMaxSlice], int strides[QMaxSlice])
	{
		m_width = w;
		m_height = h;
		m_format = format;
		memcpy(m_dataSlice, plane, sizeof(m_dataSlice));
		memcpy(m_lineSize, strides, sizeof(m_lineSize));
	}
	void attach(int w, int h, int format, void* buffer)
	{
		m_width = w;
		m_height = h;
		m_format = format;
		video::FillVideoFrameInfo((uint8_t*)buffer, m_width, m_height, m_format, m_dataSlice, (uint32_t*)m_lineSize);
	}
	void swap(QcVideoFrame& other)
	{
		std::swap(other.m_width, m_width);
		std::swap(other.m_height, m_height);
		std::swap(other.m_format, m_format);
		std::swap(other.m_sampleTime, m_sampleTime);
		for (int i=0; i<QMaxSlice; ++i)
		{
			std::swap(other.m_dataSlice[i], m_dataSlice[i]);
			std::swap(other.m_lineSize[i], m_lineSize[i]);
		}
		std::swap(other.m_buffer, m_buffer);
	}

	int width() const { return m_width; }
	int height() const { return m_height; }
	int format() const { return m_format;  }

	uint8_t** datas() const { return (uint8_t**)m_dataSlice; }
	int* lineSizes() const { return (int*)m_lineSize; }


	uint8_t* planarArgb() const { return m_dataSlice[0]; }
	uint8_t* planarY() const { return m_dataSlice[0]; }
	uint8_t* planarU() const { return m_dataSlice[1]; }
	uint8_t* planarV() const { return m_dataSlice[2]; }

	int lineSizeArgb() const { return m_lineSize[0]; }
	int lineSizeY() const { return m_lineSize[0]; }
	int lineSizeU() const { return m_lineSize[1]; }
	int lineSizeV() const { return m_lineSize[2]; }

	unsigned long long m_sampleTime;
protected:
	QcBuffer m_buffer;
	uint8_t* m_dataSlice[QMaxSlice];
	int m_lineSize[QMaxSlice];
	int m_width;
	int m_height;
	int m_format;
};

namespace std
{
	template<> inline void swap(QcVideoFrame& one, QcVideoFrame& two)
	{
		one.swap(two);
	}
}
