#include "FFmpegVideoDecoder.h"
#include "AVFrameRef.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
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


       AVCodec* pCodec = avcodec_find_decoder(par ? par->codec_id : (AVCodecID)m_codeID);
       if (pCodec)
           OpenCodec(par, pCodec, hw);
    } while (0);
}

void FFmpegVideoDecoder::OpenCodec(const AVCodecParameters *par, AVCodec* pCodec, bool hw)
{
    AVCodecContext* pCodecCtx = nullptr;
    do 
    {
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (pCodecCtx == NULL)
            break;

		auto old_get_format = pCodecCtx->get_format;
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

		bool bDone = false;
		int err = 0;
		if (hw)
		{
			do 
			{
				err = av_hwdevice_ctx_create(&m_hw_device_ctx, AV_HWDEVICE_TYPE_D3D11VA, NULL, NULL, 0);
				if (err < 0) {
					break;
				}
				pCodecCtx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);

				pCodecCtx->get_format = [](AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
					const enum AVPixelFormat *p;
					for (p = pix_fmts; *p != -1; p++) {
						if (*p == AV_PIX_FMT_D3D11)
							return *p;
					}
					return AV_PIX_FMT_NONE;
				};

				err = avcodec_open2(pCodecCtx, pCodec, NULL);
				if (err < 0)
					break;

				bDone = true;
			} while (0);
		}

		if (!bDone)
		{
			av_buffer_unref(&m_hw_device_ctx);
			pCodecCtx->hw_device_ctx = nullptr;
			pCodecCtx->get_format = old_get_format;

			if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
			{
				avcodec_free_context(&pCodecCtx);
				break;
			}
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

int FFmpegVideoDecoder::Decode(const char* dataIn, int dataSize)
{
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = (uint8_t*)dataIn;
	packet.size = dataSize;

	return Decode(&packet);
}

int FFmpegVideoDecoder::Decode(const AVPacket* pkt)
{
	return avcodec_send_packet(m_pCodecCtx, pkt);
}

int FFmpegVideoDecoder::recv(AVFrameRef& frame)
{
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


void FFmpegVideoDecoder::flush()
{
	avcodec_flush_buffers(m_pCodecCtx);
}
