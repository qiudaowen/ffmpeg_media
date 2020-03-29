#include "FFmpegVideoDecoder.h"
#include "AVFrameRef.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


static AVCodec *find_hardware_decoder(enum AVCodecID id)
{
	AVHWAccel *hwa = av_hwaccel_next(NULL);
	AVCodec *c = NULL;

	while (hwa) {
		if (hwa->id == id) {
			if (hwa->pix_fmt == AV_PIX_FMT_DXVA2_VLD ||
				hwa->pix_fmt == AV_PIX_FMT_VAAPI_VLD) {
				c = avcodec_find_decoder_by_name(hwa->name);
				if (c)
					break;
			}
		}

		hwa = av_hwaccel_next(hwa);
	}

	return c;
}

FFmpegVideoDecoder::FFmpegVideoDecoder(const AVCodecParameters *par, bool hw)
{
    Open(par, hw);
}

FFmpegVideoDecoder::FFmpegVideoDecoder(int srcW, int srcH, int srcFormat, int codecID, bool hw)
    : m_srcW(srcW)
    , m_srcH(srcH)
    , m_srcFormat(srcFormat)
    , m_codeID(codecID)
{
    Open(nullptr, hw);
}

FFmpegVideoDecoder::~FFmpegVideoDecoder()
{
	Close();
}

void FFmpegVideoDecoder::Open(const AVCodecParameters *par, bool hw)
{
    do
    {
        Close();

        if (hw)
        {
            AVCodec* pCodec = find_hardware_decoder(par ? par->codec_id : (AVCodecID)m_codeID);
            if (pCodec)
                OpenCodec(par, pCodec);
        }
        if (m_pCodec == 0)
        {
            AVCodec* pCodec = avcodec_find_decoder(par ? par->codec_id : (AVCodecID)m_codeID);
            if (pCodec)
                OpenCodec(par, pCodec);
        }
    } while (0);
}

void FFmpegVideoDecoder::OpenCodec(const AVCodecParameters *par, AVCodec* pCodec)
{
    AVCodecContext* pCodecCtx = nullptr;
    do 
    {
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (pCodecCtx == NULL)
            break;

        if (par != nullptr)
        {
            avcodec_parameters_to_context(pCodecCtx, par);
            m_srcW = par->width;
            m_srcH = par->height;
            m_srcFormat = par->format;
            m_codeID = par->codec_id;
        }
        else
        {
            pCodecCtx->width = m_srcW;
            pCodecCtx->height = m_srcH;
            pCodecCtx->coded_width = m_srcW;
            pCodecCtx->coded_height = m_srcH;
            pCodecCtx->pix_fmt = (AVPixelFormat)m_srcFormat;
        }
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			avcodec_free_context(&pCodecCtx);
			break;
		}
            
        m_pCodec = pCodec;
        m_pCodecCtx = pCodecCtx;
    } while (0);
}


void FFmpegVideoDecoder::Close()
{
    if (m_pCodecCtx)
    {
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }
}

int FFmpegVideoDecoder::Decode(const AVPacket* pkt, AVFrameRef& frame)
{
	avcodec_send_packet(m_pCodecCtx, pkt);

	AVFrameRef newFrame = AVFrameRef::allocFrame();
	int ret = avcodec_receive_frame(m_pCodecCtx, newFrame);
	switch (ret)
	{
	case AVERROR_EOF:
		return kEOF;
	case AVERROR(EAGAIN):
		return kAgain;
	case 0:
		frame = newFrame;
		return kOk;
	}
	return kOtherError;
}


int FFmpegVideoDecoder::Decode(const char* dataIn, int dataSize, AVFrameRef& frame)
{
    AVPacket packet;
	av_init_packet(&packet);
	packet.data = (uint8_t*)dataIn;
	packet.size = dataSize;

	return Decode(&packet, frame);
}

void FFmpegVideoDecoder::flush()
{
	avcodec_flush_buffers(m_pCodecCtx);
}
