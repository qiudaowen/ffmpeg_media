#pragma once

#include "libmedia/QcVideoFrame.h"

class QcDIBSection : public QcVideoFrame
{
public:
	QcDIBSection()
		: m_memBitmap(NULL)
		, m_memHdc(NULL)
		, m_hSectionAttached(NULL)
		, m_secOffset(0)
	{
	}
	~QcDIBSection()
	{
		Close();
	}

	void resize(int w, int h, HANDLE hSection = NULL, DWORD offset = 0)
	{
		bool needResize = false;
		if (w != width() || h != height())
		{
			needResize = true;
		}
		//else if((hSection!=NULL && (hSection!=m_hSectionAttached || m_secOffset!=offset)))
		else if(((hSection!=m_hSectionAttached || m_secOffset!=offset)))
		{
			needResize = true;
		}
		if(needResize)
		{
			Close();

			m_memHdc = CreateCompatibleDC(NULL);

			void* pBits;
			BITMAPINFO m_memBitmapInfo = {0};
			m_memBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			m_memBitmapInfo.bmiHeader.biWidth = w;
			m_memBitmapInfo.bmiHeader.biHeight = -h;
			m_memBitmapInfo.bmiHeader.biPlanes = 1;
			m_memBitmapInfo.bmiHeader.biBitCount = 32;
			m_memBitmapInfo.bmiHeader.biCompression = BI_RGB;
			m_memBitmap = CreateDIBSection(m_memHdc,&m_memBitmapInfo,DIB_RGB_COLORS,(void **)&pBits, hSection, offset);
			SelectObject(m_memHdc, m_memBitmap);
			m_hSectionAttached = hSection;
			m_secOffset = offset;

			attach(w, h, FOURCC_BGRA, pBits);
		}
	}

	//HBITMAP dibHandle() {return m_memBitmap;}
	HDC dcHandle() {return m_memHdc;}

	void swap(QcDIBSection& other)
	{
		QcVideoFrame::swap(other);
		std::swap(other.m_memBitmap, m_memBitmap);
		std::swap(other.m_memHdc, m_memHdc);
		std::swap(other.m_hSectionAttached, m_hSectionAttached);
	}

protected:
	void Close()
	{
		if (NULL != m_memBitmap)
		{
			::DeleteObject(m_memBitmap);
			m_memBitmap = NULL;
		}
		if (NULL != m_memHdc)
		{
			::DeleteDC(m_memHdc);
			m_memHdc = NULL;
		}
	}

protected:
	HBITMAP m_memBitmap;
	HDC m_memHdc;
	HANDLE m_hSectionAttached;//在resize中需要判断当前的section handle是否与传入的section handle为同一个
	int m_secOffset;
};