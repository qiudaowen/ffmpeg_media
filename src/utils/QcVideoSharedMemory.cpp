#include "QcSharedMemory.h"

QcVideoSharedMemory::QcVideoSharedMemory(const wchar_t* name)
{
	m_outputShareMgr = new QcSharedMemory(name);
    std::wstring eventName1 = std::wstring(name) + L"_readEventEx";
    std::wstring eventName2 = std::wstring(name) + L"_writeEventEx";
    m_writedEvent = CreateEventW(NULL, FALSE, FALSE, eventName2.c_str());
    m_readedEvent = CreateEventW(NULL, FALSE, FALSE, eventName1.c_str());
}

void QcVideoSharedMemory::create()
{
	m_outputShareMgr->lock();
	if (!m_outputShareMgr->attach(QcSharedMemory::ReadWrite))
	{
		if (m_outputShareMgr->create(QmMaxVideoShareMemeSize, QcSharedMemory::ReadWrite))
		{
			BYTE* pData = (BYTE*)m_outputShareMgr->data();
			QsShareInfo* pHeadInfo = (QsShareInfo*)pData;
			memset(pHeadInfo, 0, sizeof(QsShareInfo));

			pHeadInfo->writeOffset = QmMaxVideoInfoSize;
			pHeadInfo->readOffset = pHeadInfo->writeOffset + QmMaxVideoBufferSize;
		}	
	}
	m_outputShareMgr->unlock(); 
}

bool QcVideoSharedMemory::attach(bool bReadOnly)
{
	if (!m_outputShareMgr->isAttached())
		return m_outputShareMgr->attach(bReadOnly ? QcSharedMemory::ReadOnly : QcSharedMemory::ReadWrite);
	return true;
}

bool QcVideoSharedMemory::detach()
{
	return m_outputShareMgr->detach();
}

void* QcVideoSharedMemory::lockWriteBuffer(DWORD* offset, HANDLE* handle, int waitTime)
{
    if (waitTime && WaitForSingleObject(m_readedEvent, waitTime) == WAIT_TIMEOUT)
        return nullptr;

	BYTE* pData = (BYTE*)m_outputShareMgr->data();
	QsShareInfo* pHeadInfo = (QsShareInfo*)pData;
    if (offset)
	    *offset = (DWORD)pHeadInfo->writeOffset;
    if (handle)
	    *handle = m_outputShareMgr->handle();
	return pData + pHeadInfo->writeOffset;
}
void QcVideoSharedMemory::unLockWriteBuffer(QsShareInfo& info, bool swapReadWrite/* = true*/)
{
	m_outputShareMgr->lock();
	QsShareInfo* pData = (QsShareInfo*)m_outputShareMgr->data();
    SetEvent(m_writedEvent);
	m_outputShareMgr->unlock();
}

void* QcVideoSharedMemory::lockReadBuffer(QsShareInfo& info, int waitTime)
{
    if (waitTime && WaitForSingleObject(m_writedEvent, waitTime) == WAIT_TIMEOUT)
        return nullptr;

	m_outputShareMgr->lock();
	BYTE* pData = (BYTE*)m_outputShareMgr->data();
	info = *(QsShareInfo*)pData;
	if (info.width == 0 || info.heigth == 0)
	{
		m_outputShareMgr->unlock();
		return NULL;
	}
	return pData + info.readOffset;
}
void QcVideoSharedMemory::unLockReadBuffer()
{
	QsShareInfo* pData = (QsShareInfo*)m_outputShareMgr->data();
	pData->readCount++;
	pData->readProcessId = GetCurrentProcessId();

    SetEvent(m_readedEvent);
	m_outputShareMgr->unlock();
}

QcVideoSharedMemory::~QcVideoSharedMemory()
{
	delete m_outputShareMgr;
}
