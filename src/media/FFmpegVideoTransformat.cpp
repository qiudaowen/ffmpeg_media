#include "FFmpegVideoTransformat.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


FFmpegVideoTransformat::FFmpegVideoTransformat()
{

}

FFmpegVideoTransformat::~FFmpegVideoTransformat()
{
    CloseSwsContext();
}

bool FFmpegVideoTransformat::Transformat(int srcW, int srcH, int srcFormat, const uint8_t *const srcSlice[], const int srcStride[],
    int dstW, int dstH, int destFormat, uint8_t *const dstSlice[], const int dstStride[])
{
    if (!OpenSwsContext(srcW, srcH, srcFormat, dstW, dstH, destFormat))
        return false;

    int iRet = sws_scale(m_pSwsCtx, (const uint8_t* const*)srcSlice, srcStride, 0, srcH, dstSlice, dstStride);
    return true;
}

bool FFmpegVideoTransformat::OpenSwsContext(int srcW, int srcH, int srcFormat, int dstW, int dstH, int destFormat)
{
    if (m_srcW == srcW && m_srcH == srcH && m_srcFormat == srcFormat
        && m_dstW == dstW && m_dstH == dstH && m_dstFormat == destFormat)
        return m_pSwsCtx != nullptr;

    CloseSwsContext();
    m_srcW = srcW;
    m_srcH = srcH;
    m_srcFormat = srcFormat;

    m_dstW = dstW;
    m_dstH = dstH;
    m_dstFormat = destFormat;

    m_pSwsCtx = sws_getContext(m_srcW, m_srcH, (AVPixelFormat)m_srcFormat, m_dstW, m_dstH, (AVPixelFormat)m_dstFormat, SWS_AREA, NULL, NULL, NULL);

    return m_pSwsCtx != nullptr;
}

void FFmpegVideoTransformat::CloseSwsContext()
{
    if (m_pSwsCtx)
    {
        sws_freeContext(m_pSwsCtx);
        m_pSwsCtx = NULL;
    }
}

