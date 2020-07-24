#pragma once

#include <windows.h>
#include <stdint.h>

class QcSharedMemory;

class QcVideoSharedMemory  
{  
public:
    QcVideoSharedMemory(const wchar_t* name);
	~QcVideoSharedMemory();

	struct QsFrameInfo
	{
		int width;
		int heigth;
		int format;
		unsigned long long m_timeStamp;
		uint8_t reverse[64];
		uint8_t data[0];
	};
	struct QsShareInfo
	{
		int readOffset;
		int swapOffset;
		int writeOffset;
		QsFrameInfo frameInfo;
	};

	bool ensure(int w, int h, int format);
	bool attach(bool bReadOnly = false);
	bool detach();

    HANDLE readableEvent() const { return m_readableEvent; }
    HANDLE writableEvent() const { return m_writableEvent; }

	//shareHandle can use in CreateDIBSection
	void* lockWriteBuffer(DWORD* offset = nullptr, HANDLE* shareMemHandle = nullptr, int waitTime = 0);
	void unLockWriteBuffer(QsFrameInfo& info, bool swapReadWrite=true);

	void* lockReadBuffer(QsFrameInfo& info, int waitTime = 0);
	void unLockReadBuffer();
private:
	QcSharedMemory* m_outputShareMgr;
    HANDLE m_readableEvent;
    HANDLE m_writableEvent;
};  
