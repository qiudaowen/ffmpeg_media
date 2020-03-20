#include "Data/Multimedia/mediaCommon.h"
#include "Data/Multimedia/QcVideoTransformat.h"
#include "QcEfficiencyProfiler.h"
#include "assert.h"
#include "QMeetDebug.h"
#include "QcLog.hpp"
#include "CShareBuffer.h"

#ifdef WIN32
#include "libyuv.h"
#include "libyuv_DLL.h"
#endif
//using namespace libyuv;

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

QcVideoTransformat::QcVideoTransformat()
	: m_pSwsCtx(NULL)
    , m_pDestFrame(NULL)
{
    m_pFrame = avcodec_alloc_frame();
}

QcVideoTransformat::~QcVideoTransformat()
{
    if (m_pDestFrame)
        avcodec_free_frame(&m_pDestFrame);
    avcodec_free_frame(&m_pFrame);
    CloseSwsContext();
}

bool QcVideoTransformat::OpenSwsContext(const QsVideoInfo& source, const QsVideoInfo& dest)
{
	bool bSame = true;
	if (source != m_sourceInfo)
	{
		m_sourceInfo = source;
		bSame = false;
	}
	if (dest != m_destInfo)
	{
		m_destInfo = dest;
		bSame = false;
	}
	if (!bSame)
	{
        QmExceptionCatch();
        if (m_pSwsCtx != NULL)
        {
            CloseSwsContext();
        }
        m_pSwsCtx = sws_getContext(source.iWidth, source.iHeiht, QmToFFMPegFormat(m_sourceInfo.eType), dest.iWidth, dest.iHeiht, QmToFFMPegFormat(dest.eType), SWS_AREA, NULL, NULL, NULL);
	}
	return m_pSwsCtx;
}

void QcVideoTransformat::CloseSwsContext()
{
	if (m_pSwsCtx)
	{
		sws_freeContext(m_pSwsCtx);
		m_pSwsCtx = NULL;
	}
}

bool QcVideoTransformat::Transformat(const AVFrame* pFrame, CAMERA_DATATYPE_E eSourceType, const QsVideoInfo& destInfo, char* destOut, int nBufSize)
{
    QmExceptionCatch();
    QmAssert(destOut && nBufSize >= destInfo.CalBufSize());

	QmProfileFragment("Transformat2");
	bool bRet = false;
//#ifndef WIN32
#if 1   // 丘道文 使用ffmpeg
	if (OpenSwsContext(QsVideoInfo(eSourceType, pFrame->width, pFrame->height), destInfo))
	{
        QmExceptionCatch();
        if (m_pDestFrame == NULL)
            m_pDestFrame = avcodec_alloc_frame();

        QmExceptionCatch();
        avpicture_fill((AVPicture*)m_pDestFrame, (const uint8_t*)destOut, QmToFFMPegFormat(destInfo.eType), destInfo.iWidth, destInfo.iHeiht);
        if(m_pSwsCtx != NULL)
        {
            QmExceptionCatch();
            sws_scale(m_pSwsCtx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pFrame->height, m_pDestFrame->data, m_pDestFrame->linesize);
        }
		bRet = true;
	}
    else
    {
        QmLogLEVEL1("Fail To OpenSwsContext[%d-%d][%d-%d]", pFrame->width, pFrame->height, destInfo.iWidth, destInfo.iHeiht);
        QmAssert(false);
    }
#else
	/*
	* I420ToARGB
	* I420Scale
	* ARGBScale
	* ABGRToI420
	*/
	if (eSourceType == CAMERA_DATATYPE_RGBA)
	{
		if (destInfo.eType == CAMERA_DATATYPE_RGBA)
		{
            QmExceptionCatch();
            ARGBScale(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, (uint8*)destOut, destInfo.iWidth * 4, destInfo.iWidth, destInfo.iHeiht, kFilterBox);
            bRet = true;
        }
		else if (destInfo.eType == CAMERA_DATATYPE_YV12)
		{
            CShareBuffer buffer(destInfo.iWidth * destInfo.iHeiht * 4);
            CShareBuffer* m_tempBuffer = &buffer;
            ARGBScale(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, (uint8*)m_tempBuffer->data(), destInfo.iWidth * 4, destInfo.iWidth, destInfo.iHeiht, kFilterBox);

            uint8* destData[3];
            int destLineSize[3];
            QmFillYUVVideoFrame((uint8*)destOut, destInfo.iWidth, destInfo.iHeiht, destData, destLineSize);

            QmExceptionCatch();
            ARGBToI420((const uint8*)m_tempBuffer->data(), destInfo.iWidth * 4, destData[0], destLineSize[0], destData[1], destLineSize[1], destData[2], destLineSize[2], destInfo.iWidth, destInfo.iHeiht);
            bRet = true;
        }
	}
	else if (eSourceType == CAMERA_DATATYPE_YV12)
	{
		if (destInfo.eType == CAMERA_DATATYPE_RGBA)
		{
            CShareBuffer buffer(destInfo.iWidth * destInfo.iHeiht * 2);
            CShareBuffer* m_tempBuffer = &buffer;

            uint8* destData[3];
            int destLineSize[3];
            QmFillYUVVideoFrame((uint8*)m_tempBuffer->data(), destInfo.iWidth, destInfo.iHeiht, destData, destLineSize);

            QmExceptionCatch();
            I420Scale(pFrame->data[0], pFrame->linesize[0], pFrame->data[1], pFrame->linesize[1], pFrame->data[2], pFrame->linesize[2], pFrame->width, pFrame->height
                      , destData[0], destLineSize[0], destData[1], destLineSize[1], destData[2], destLineSize[2], destInfo.iWidth, destInfo.iHeiht, kFilterBox);

            QmExceptionCatch();
            I420ToARGB(destData[0], destLineSize[0], destData[1], destLineSize[1], destData[2], destLineSize[2], (uint8*)destOut, destInfo.iWidth * 4, destInfo.iWidth, destInfo.iHeiht);
            bRet = true;
        }
		else if (destInfo.eType == CAMERA_DATATYPE_YV12)
		{
			uint8* destData[3];
            int destLineSize[3];
            QmFillYUVVideoFrame((uint8*)&destOut[0], destInfo.iWidth, destInfo.iHeiht, destData, destLineSize);

            QmExceptionCatch();
			I420Scale(pFrame->data[0], pFrame->linesize[0], pFrame->data[1], pFrame->linesize[1], pFrame->data[2], pFrame->linesize[2], pFrame->width, pFrame->height
                      , destData[0], destLineSize[0], destData[1], destLineSize[1], destData[2], destLineSize[2], destInfo.iWidth, destInfo.iHeiht, kFilterBox);
            bRet = true;
        }
	}
#endif
	return bRet;
}

bool QcVideoTransformat::Transformat(const QsVideoFrame* pFrame, const QsVideoInfo& destInfo, char* destOut, int nBufSize)
{
    QmExceptionCatch();
    AVFrame& sourceFrame = *m_pFrame;
    for (int i=0; i<QMaxSlice; ++i)
    {
        sourceFrame.data[i] = (uint8_t*)pFrame->dataSlice[i];
        sourceFrame.linesize[i] = pFrame->lineSize[i];
    }
    sourceFrame.width = pFrame->iDestWidth;
    sourceFrame.height = pFrame->iDestHeight;

    return Transformat(&sourceFrame, pFrame->eDataType, destInfo, destOut, nBufSize);
}
