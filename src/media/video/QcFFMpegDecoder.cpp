#include "QcFFMpegDecoder.h"
#include "Data/Multimedia/QcVideoTransformat.h"
//#include "ESVideoPub.h"
#include <QFile>
#include "QcEfficiencyProfiler.h"
#include "QMeetDebug.h"

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <QDebug>

QcFFmpegDecoder::QcFFmpegDecoder()
	: m_pNotify(NULL)
	, m_pCodecCtx(NULL)
	, m_pDecodeCodec(NULL)
    , m_pTransFormat(NULL)
{
    m_pDecodeFrame = avcodec_alloc_frame();
}

QcFFmpegDecoder::~QcFFmpegDecoder()
{
    avcodec_free_frame(&m_pDecodeFrame);
	Close();
    if (m_pTransFormat)
        delete m_pTransFormat;
}

void QcFFmpegDecoder::SetVideoNotify(IVideoDecodeDataNotify* pNotify)
{
	m_pNotify = pNotify;
}

bool QcFFmpegDecoder::Open(const QsVideoInfo& destInfo)
{
	m_destInfo = destInfo;
	return true;
}

bool QcFFmpegDecoder::Open2(const QsVideoInfo& sourceInfo)
{
	bool bRet = false;
	do 
	{
		int nWidth = sourceInfo.iWidth;
		int nHeight = sourceInfo.iHeiht;
		if (nWidth == 0 || nHeight == 0)
			break;

		Close();
		m_pDecodeCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
		if (m_pDecodeCodec == NULL)
			break;

		m_pCodecCtx = avcodec_alloc_context3(m_pDecodeCodec);
		if (m_pCodecCtx == NULL)
			break;

		m_pCodecCtx->coded_width = nWidth;
		m_pCodecCtx->coded_height = nHeight;
		m_pCodecCtx->width = nWidth;
		m_pCodecCtx->height = nHeight;
		m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

		if (avcodec_open2(m_pCodecCtx, m_pDecodeCodec, NULL) < 0)
			break;

		m_sourceInfo = sourceInfo;
		bRet = true;
	} while (0);
	

	return bRet;
}

void QcFFmpegDecoder::Close()
{
	if (m_pCodecCtx)
	{
		if (m_pDecodeCodec)
		{
			avcodec_close(m_pCodecCtx);
			m_pDecodeCodec = NULL;
		}
		av_free(m_pCodecCtx);
		m_pCodecCtx = NULL;
	}
}

bool QcFFmpegDecoder::Decode(const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame, void* pContext)
{
    bool bRet = false;
    char* bufOut = NULL;
    if (m_pNotify)
    {
		bRet = Decode2(dataIn, dataSize, sourceInfo, keyFrame, &bufOut, 0, m_pNotify, pContext);
    }
    return bRet;
}

bool QcFFmpegDecoder::Decode(const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame, char** outBuf, int bufSize)
{
    return Decode2(dataIn, dataSize, sourceInfo, keyFrame, outBuf, bufSize, NULL);
}

bool QcFFmpegDecoder::Decode(AVFrame* pFrameOut, const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame)
{
	if (m_pCodecCtx == NULL || m_sourceInfo != sourceInfo)
	{
		if (!Open2(sourceInfo))
			return false;
	}

    AVPacket packet;
	av_init_packet(&packet);
	packet.data = (uint8_t*)dataIn;
	packet.size = dataSize;

	int nGotPic = 0;
	{
        QmExceptionCatch();
		QmProfileFragment("FFMpegDecode[%d,%d]", sourceInfo.iWidth, sourceInfo.iHeiht);
		avcodec_decode_video2(m_pCodecCtx, pFrameOut, &nGotPic, &packet);
	}
	if (nGotPic == 0)
    {
        if (keyFrame)
        {
            QmLogLEVEL1("Error: Fail to Decode Key Frame[srcW=%d srcH=%d packetSize=%d]", sourceInfo.iWidth, sourceInfo.iHeiht, dataSize);
        }
		return false;
    }
	return true;
}

bool QcFFmpegDecoder::Decode2(const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame, char** outBuf, int bufSize, IVideoDecodeDataNotify* pNotify, void* pContext)
{
    if (m_pCodecCtx == NULL || m_sourceInfo != sourceInfo)
    {
        if (!Open2(sourceInfo))
            return false;
    }

    AVPacket packet;
    av_init_packet(&packet);
    packet.data = (uint8_t*)dataIn;
    packet.size = dataSize;

    int nGotPic = 0;

    AVFrame& decodeFrame = *m_pDecodeFrame;
    int iRet = 0;
    {
        QmExceptionCatch();
        QmProfileFragment("FFMpegDecode[%d,%d]", sourceInfo.iWidth, sourceInfo.iHeiht);
        iRet = avcodec_decode_video2(m_pCodecCtx, &decodeFrame, &nGotPic, &packet);
    }

    if (nGotPic == 0)
    {
        if (keyFrame)
        {
            QmLogLEVEL1("Error: Fail to Decode Key Frame[srcW=%d srcH=%d packetSize=%d]", sourceInfo.iWidth, sourceInfo.iHeiht, dataSize);
        }
        return false;
    }

    CAMERA_DATATYPE_E eDestType = (m_destInfo.eType == CAMERA_DATATYPE_NONE) ? sourceInfo.eType : m_destInfo.eType;
    if (pNotify && eDestType == CAMERA_DATATYPE_YV12)
    {
        QsVideoFrame frame = {0};
        frame.dataSlice[0] = (const char*)decodeFrame.data[0];
        frame.dataSlice[1] = (const char*)decodeFrame.data[1];
        frame.dataSlice[2] = (const char*)decodeFrame.data[2];
        frame.lineSize[0] = decodeFrame.linesize[0];
        frame.lineSize[1] = decodeFrame.linesize[1];
        frame.lineSize[2] = decodeFrame.linesize[2];
        frame.iDestWidth = sourceInfo.iWidth;
        frame.iDestHeight = sourceInfo.iHeiht;
        frame.eDataType = CAMERA_DATATYPE_YV12;

        QmExceptionCatch();
        pNotify->OnDecodeData(&frame, pContext);
    }
    else
    {
        int iDestWidth = m_destInfo.iWidth == 0 ? sourceInfo.iWidth : m_destInfo.iWidth;
        int iDestHeight = m_destInfo.iHeiht == 0 ? sourceInfo.iHeiht : m_destInfo.iHeiht;

        QsVideoInfo destInfo(eDestType, iDestWidth, iDestHeight);
        if (*outBuf == NULL || bufSize == 0)
        {
            m_buffer.resize(destInfo.CalBufSize());
            *outBuf = (char*)&(m_buffer[0]);
            bufSize = m_buffer.size();
        }

        if (eDestType == CAMERA_DATATYPE_YV12 && iDestWidth == decodeFrame.width && iDestHeight == decodeFrame.height)
        {
            int iH = avpicture_layout((const AVPicture*)&decodeFrame, (AVPixelFormat)decodeFrame.format,
                             decodeFrame.width, decodeFrame.height, (unsigned char*)*outBuf,
                             destInfo.CalBufSize());
        }
        else
        {
            if (m_pTransFormat == NULL)
                m_pTransFormat = new QcVideoTransformat();
            m_pTransFormat->Transformat(&decodeFrame, CAMERA_DATATYPE_YV12, QsVideoInfo(eDestType, iDestWidth, iDestHeight), *outBuf, bufSize);
        }

        if (pNotify)
        {
            QsVideoFrame frame = {0};
            {
                AVFrame dataSliceFrame;
                avpicture_fill((AVPicture*)&dataSliceFrame, (const uint8_t*)*outBuf, QmToFFMPegFormat(eDestType), iDestWidth, iDestHeight);
                frame.dataSlice[0] = (const char*)decodeFrame.data[0];
                frame.dataSlice[1] = (const char*)decodeFrame.data[1];
                frame.dataSlice[2] = (const char*)decodeFrame.data[2];
                frame.lineSize[0] = decodeFrame.linesize[0];
                frame.lineSize[1] = decodeFrame.linesize[1];
                frame.lineSize[2] = decodeFrame.linesize[2];
                frame.iDestWidth = iDestWidth;
                frame.iDestHeight = iDestHeight;
                frame.eDataType = eDestType;
            }

            QmExceptionCatch();
            pNotify->OnDecodeData(&frame, pContext);
        }
    }
    if (m_pCodecCtx->refcounted_frames > 0)
        av_frame_unref(&decodeFrame);
    return nGotPic;
}
