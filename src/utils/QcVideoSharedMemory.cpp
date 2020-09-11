#include "QcVideoSharedMemory.h"
#include "QcSharedMemory.h"
#include <algorithm>
#include "videodef.h"

const int kBlockShift = 18;
const int kBlockSize = 1 << kBlockShift;
const int kBlockMask = kBlockSize - 1;

static int calcBlocks(int size)
{
    return (size + kBlockMask) >> kBlockShift;
}

QcVideoSharedMemory::QcVideoSharedMemory(const wchar_t* name)
{
     m_pHead = new QcSharedMemory(name);

     std::wstring eventName1 = m_pHead->key() + L"_headChangedEvent";
     std::wstring eventName2 = m_pHead->key() + L"_frameEvent";
     m_headChangeEvent = CreateEventW(NULL, FALSE, FALSE, eventName2.c_str());
     m_frameEvent = CreateEventW(NULL, FALSE, FALSE, eventName1.c_str());
}

QcVideoSharedMemory::~QcVideoSharedMemory()
{
    delete m_pHead;
    m_pHead = nullptr;
}

bool QcVideoSharedMemory::create(int nFrameBuffer)
{
    int iSize = std::max((int)sizeof(QsHeadInfo) + sizeof(QsFrameInfo) * nFrameBuffer, 1024);
    m_pHead->lock();
    if (m_pHead->create(iSize, QcSharedMemory::ReadWrite))
    {
        m_pHeadInfo = (QsHeadInfo*)m_pHead->data();
        if (m_pHead->isCreator())
        {
			m_pHeadInfo->nFrameBuffer = nFrameBuffer;
			m_pHeadInfo->writeIndex = 0;
			m_pHeadInfo->readIndex = 0;
        }
    }
    m_pHead->unlock();
}

bool QcVideoSharedMemory::attach(bool bReadOnly /*= false*/)
{
    return m_pHead->attach(bReadOnly ? QcSharedMemory::ReadOnly : QcSharedMemory::ReadWrite);
}

bool QcVideoSharedMemory::detach()
{
    return m_pHead->detach();
}

void* QcVideoSharedMemory::lockWriteBuffer(int w, int h, int format, bool bOverWrite /*= false*/)
{
    uint32_t blockSize = (video::CalBufNeedSize(w, h, format) + kBlockMask) & kBlockSize;
    if (m_pBlockInfo && m_pBlockInfo->blockSize != blockSize)
    {
        delete m_pData;
        m_pBlockInfo = nullptr;
    }
    if (m_pBlockInfo == nullptr)
    {
        ++m_pHeadInfo->mapID;

        std::wstring newName = m_pHead->key() + L"_map" + std::to_wstring(m_pHeadInfo->mapID);
        m_pData = new QcSharedMemory(newName);
        m_pData->create(m_pHeadInfo->frameBufferCount * blockSize + sizeof(QsBlockInfo));
        m_pBlockInfo = (QsBlockInfo*)m_pData->data();
        m_pBlockInfo->blockSize = blockSize;
        m_pBlockInfo->readIndex = 0;
        m_pBlockInfo->writeIndex = 0;
    }

}

bool QcVideoSharedMemory::ensureFrameBuffer(int w, int h, int format)
{
	frameInfo(m_pHead->ReadWrite);
}

void QcVideoSharedMemory::unLockWriteBuffer()
{

}

QsFrameInfo* QcVideoSharedMemory::frameInfo(int index)
{
	return m_pHeadInfo->frameInfo[index % m_pHeadInfo->nFrameBuffer];
}
