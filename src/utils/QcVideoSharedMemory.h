#pragma once

#include <windows.h>
#include <stdint.h>
#include <string>

class QcSharedMemory;
class QcVideoSharedMemory
{
    struct QsHeadInfo
    {
        int readIndex;
        int writeIndex;
		int nFrameBuffer;
		uint8_t reverse[64];
		QsFrameInfo frameInfo[0];
    };
	struct QsFrameInfo
	{
		int mapID;
		int bufferSize;
		int width;
		int heigth;
		int format;
		unsigned long long m_timeStamp;
		uint8_t reverse[64];
		uint8_t data[0];
	};
	struct QsFrameData
	{
		QcSharedMemory* m_pData;
	};
public:
    QcVideoSharedMemory(const wchar_t* name);
    ~QcVideoSharedMemory();

    bool create(int nFrameBuffer);
    bool attach(bool bReadOnly);
    bool detach();

    void* lockWriteBuffer(int w, int h, int format, bool bOverWrite = false);
    void unLockWriteBuffer();

    void* lockReadBuffer(int waitTime = 0);
    void unLockReadBuffer();
protected:
    bool ensureFrameBuffer(int w, int h, int format);
	QsFrameInfo* frameInfo(int index);
private:
    QsHeadInfo* m_pHeadInfo = nullptr;
    QcSharedMemory* m_pHead = nullptr;
    HANDLE m_headChangeEvent = 0;
    HANDLE m_frameEvent = 0;
};
