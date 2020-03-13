#include "QcFFMpegEncoder.h"
#include "QcEfficiencyProfiler.h"
#include <time.h>
#include "QcLog.hpp"
#include "QMeetDebug.h"

extern "C"{
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

int FFMpegInit()
{
	av_register_all();
	avcodec_register_all();
	return 0;
}
const int giTempddddddd = FFMpegInit();



QcFFMpegEncoder::QcFFMpegEncoder()
	: m_pNotify(NULL)
	, m_pCodecCtx(NULL)
	, m_pEncodeCodec(NULL)
    , m_iLastClock(0)
{
    QMDEBUGDATA("", "MultiMedia");
}

QcFFMpegEncoder::~QcFFMpegEncoder()
{
    QMDEBUGDATA("", "MultiMedia");
	Close();
}

void QcFFMpegEncoder::SetVideoNotify(IVideoEncodeDataNotify* pNotify)
{
    QMDEBUGDATA("", "MultiMedia");
	m_pNotify = pNotify;
}

bool QcFFMpegEncoder::Open(const QsEncodePara& para)
{
    QMDEBUGDATA("", "MultiMedia");
	bool bRet = false;
	do
	{
		if (para.iWidth == 0 || para.iHeight == 0)
			break;

		Close();
        m_pEncodeCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (m_pEncodeCodec == NULL)
		{
			break;
		}
			
		m_pCodecCtx = avcodec_alloc_context3(m_pEncodeCodec);
		if (m_pCodecCtx == NULL)
		{
			break;
		}
			
		m_pCodecCtx->width = para.iWidth;
		m_pCodecCtx->height = para.iHeight;
		AVRational rate;
		rate.num = 1;
		rate.den = para.iFrame;
		m_pCodecCtx->time_base = rate;
		m_pCodecCtx->gop_size = para.iFrame * 4;
		m_pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

		////
		m_pCodecCtx->bit_rate = para.iBitRate * 1000;
        //m_pCodecCtx->rc_min_rate = m_pCodecCtx->bit_rate;
        //m_pCodecCtx->rc_max_rate = m_pCodecCtx->bit_rate;
        //m_pCodecCtx->bit_rate_tolerance = m_pCodecCtx->bit_rate;
        //m_pCodecCtx->rc_buffer_size = m_pCodecCtx->bit_rate;
        //m_pCodecCtx->rc_initial_buffer_occupancy = m_pCodecCtx->rc_buffer_size * 3 / 4;
        //m_pCodecCtx->rc_buffer_aggressivity = (float)1.0;
        //m_pCodecCtx->rc_initial_cplx = 0.5;

        //m_pCodecCtx->qcompress = (float)1.0;
        //m_pCodecCtx->qmin = para.iQMin;
        //m_pCodecCtx->qmax = para.iQMax;

        av_opt_set(m_pCodecCtx->priv_data, "tune", "zerolatency", 0); //no delay encode.

		if (avcodec_open2(m_pCodecCtx, m_pEncodeCodec, NULL) < 0)
			break;

		m_buffer.resize(para.iWidth * para.iHeight * 2);

        m_para = para;

		bRet = true;
	} while (0);
	return bRet;
}

void QcFFMpegEncoder::Close()
{
    QMDEBUGDATA("", "MultiMedia");
	if (m_pCodecCtx)
	{
		if (m_pEncodeCodec)
		{
			avcodec_close(m_pCodecCtx);
			m_pEncodeCodec = NULL;
		}
		av_free(m_pCodecCtx);
		m_pCodecCtx = NULL;
	}
}

//YUV
bool QcFFMpegEncoder::Encode(const char * const dataSlice[], int width, int height, CAMERA_DATATYPE_E eDataType, unsigned long long timestamp)
{
    QMDEBUGDATA("", "MultiMedia");
	char* bufOut = NULL;
	int keyFrame = 0;
	int nSize = Encode(dataSlice, width, height, eDataType, keyFrame, bufOut, 0);
	if (m_pNotify)
	{
		QsEncodeData dataOut;
		dataOut.pData = (const char*)bufOut;
		dataOut.nDataLen = nSize;
		dataOut.keyFrame = keyFrame;
		dataOut.width = width;
		dataOut.height = height;
		dataOut.eDataType = eDataType;
		dataOut.timestamp = timestamp;
		m_pNotify->OnEncodeData(&dataOut);
	}
	return nSize;
}

int QcFFMpegEncoder::Encode(const char* const dataSlice[], int width, int height, CAMERA_DATATYPE_E eDataType, int& keyFrame, char*& outBuf, int nBufSize)
{
    QMDEBUGDATA("", "MultiMedia");
	if (m_para.iWidth != width || m_para.iHeight != height)
	{
		m_para.iWidth = width;
		m_para.iHeight = height;
		if (!Open(m_para))
			return false;
	}
	if (m_pCodecCtx == NULL || eDataType != CAMERA_DATATYPE_YV12)
		return false;

	if (outBuf == NULL || nBufSize <= 0)
	{
		outBuf = (char*)&(m_buffer[0]);
		nBufSize = m_buffer.size();
	}

	AVFrame frame = {0};
    char* p = 0;
    avpicture_fill((AVPicture*)&frame, (const uint8_t*)p, AV_PIX_FMT_YUV420P, width, height);
	frame.data[0] = (unsigned char*)dataSlice[0];
	frame.data[1] = (unsigned char*)dataSlice[1];
	frame.data[2] = (unsigned char*)dataSlice[2];
	frame.width = width;
	frame.height = height;
	frame.format = AV_PIX_FMT_YUV420P;
    frame.pts = 0;

    AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = (uint8_t*)outBuf;
	pkt.size = nBufSize;

	int got_packet = 0;
	int iRet = 0;
	{
        frame.key_frame = clock() - m_iLastClock >= (2 * CLOCKS_PER_SEC) ? 1 : 0;
        m_iLastClock = clock();

		QmProfileFragment("FFMpegEncode[%d-%d]", width, height);
		iRet = avcodec_encode_video2(m_pCodecCtx, &pkt, &frame, &got_packet);
        if (m_pCodecCtx->coded_frame->key_frame)
            pkt.flags |= AV_PKT_FLAG_KEY;
	}
	
    if (got_packet && outBuf != (char*)pkt.data)
    {
        keyFrame = (pkt.flags & AV_PKT_FLAG_KEY);

        QmPrintf_KeyFrameInfo(keyFrame, pkt.size);

        m_buffer.resize(pkt.size);
        outBuf = (char*)&(m_buffer[0]);
        memcpy(outBuf, pkt.data, pkt.size);

		av_free_packet(&pkt);
    }
    else
    {
        //here is delay encode. we can call av_opt_set funion before avcodec_open2.
    }
	return got_packet ? pkt.size : 0;
}

