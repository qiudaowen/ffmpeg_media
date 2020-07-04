#include "FFmpegVideoDecoder.h"
#include "AVFrameRef.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/hwcontext.h>
#include <libavutil/error.h>
}
#include <libavutil/hwcontext_d3d11va.h>

FFmpegVideoDecoder::FFmpegVideoDecoder()
{
    
}

FFmpegVideoDecoder::~FFmpegVideoDecoder()
{
	close();
	setHwDevice(nullptr);
}


void FFmpegVideoDecoder::setHwDevice(AVBufferRef* device_ctx)
{
	av_buffer_unref(&m_hw_device_ctx);
	if (device_ctx)
		m_hw_device_ctx = av_buffer_ref(device_ctx);
}

void FFmpegVideoDecoder::openCodec(const AVCodecParameters *par, int srcW, int srcH, int srcFormat, int codecID)
{
    AVCodecContext* pCodecCtx = nullptr;
    do 
    {
		AVCodec* pCodec = avcodec_find_decoder(par ? par->codec_id : (AVCodecID)codecID);
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (pCodecCtx == NULL)
            break;

        if (par != nullptr)
        {
            avcodec_parameters_to_context(pCodecCtx, par);
        }
        else
        {
            pCodecCtx->width = srcW;
            pCodecCtx->height = srcH;
            pCodecCtx->coded_width = srcW;
            pCodecCtx->coded_height = srcH;
            pCodecCtx->pix_fmt = (AVPixelFormat)srcFormat;
        }

		bool bDone = false;
		int err = 0;
		if (m_hw_device_ctx)
		{
			do 
			{
				auto old_get_format = pCodecCtx->get_format;
				pCodecCtx->hw_device_ctx = av_buffer_ref(m_hw_device_ctx);

#if 1
				pCodecCtx->get_format = [](AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts) {
					const enum AVPixelFormat *p;
					AVBufferRef* new_frames_ctx = nullptr;
					for (p = pix_fmts; *p != -1; p++) {
						if (*p == AV_PIX_FMT_D3D11)
						{
							{
								AVCodecContext* pCodecCtx = ctx;
								AVBufferRef* new_frames_ctx = nullptr;
								if (avcodec_get_hw_frames_parameters(pCodecCtx, pCodecCtx->hw_device_ctx, AV_PIX_FMT_D3D11, &new_frames_ctx) < 0)
								{
									break;
								}
								auto fctx = (AVHWFramesContext*)new_frames_ctx->data;

								auto d3d11HFrameCtx = static_cast<AVD3D11VAFramesContext*>(fctx->hwctx);
								d3d11HFrameCtx->BindFlags |= D3D11_BIND_SHADER_RESOURCE;
								auto ret = av_hwframe_ctx_init(new_frames_ctx);
								if (ret < 0)
								{
									char buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
									av_strerror(ret, buffer, AV_ERROR_MAX_STRING_SIZE);
								}
								pCodecCtx->hw_frames_ctx = new_frames_ctx;
							}
							return *p;
						}
					}
					return AV_PIX_FMT_NONE;
				};
#endif

				//pCodecCtx->hw_frames_ctx = av_hwframe_ctx_alloc(m_hw_device_ctx);
				err = avcodec_open2(pCodecCtx, pCodec, NULL);
				if (err < 0)
				{
					av_buffer_unref(&pCodecCtx->hw_frames_ctx);
					av_buffer_unref(&pCodecCtx->hw_device_ctx);
					pCodecCtx->get_format = old_get_format;
					break;
				}

				bDone = true;
			} while (0);
		}

		if (!bDone)
		{
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


bool FFmpegVideoDecoder::open(const AVCodecParameters *par)
{
	openCodec(par, 0, 0, 0, 0);
	return m_pCodecCtx != nullptr;
}

bool FFmpegVideoDecoder::open(int srcW, int srcH, int srcFormat, int codecID)
{
	openCodec(nullptr, srcW, srcH, srcFormat, codecID);
	return m_pCodecCtx != nullptr;
}

void FFmpegVideoDecoder::close()
{
    if (m_pCodecCtx)
    {
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }
}

int FFmpegVideoDecoder::decode(const char* dataIn, int dataSize)
{
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = (uint8_t*)dataIn;
	packet.size = dataSize;

	return decode(&packet);
}

int FFmpegVideoDecoder::decode(const AVPacket* pkt)
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
	if (m_pCodecCtx)
		avcodec_flush_buffers(m_pCodecCtx);
}
