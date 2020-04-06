#include "FFmpegAudioDecoder.h"
#include "AVFrameRef.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

FFmpegAudioDecoder::FFmpegAudioDecoder(const AVCodecParameters *par)
{
    Open(par);
}

FFmpegAudioDecoder::~FFmpegAudioDecoder()
{
	Close();
}

void FFmpegAudioDecoder::flush()
{
	avcodec_flush_buffers(m_pCodecCtx);
}

void FFmpegAudioDecoder::Open(const AVCodecParameters *par)
{
    do
    {
        Close();

        AVCodec* pCodec = avcodec_find_decoder(par->codec_id);
        if (pCodec)
            OpenCodec(par, pCodec);
    } while (0);
}

void FFmpegAudioDecoder::OpenCodec(const AVCodecParameters *par, AVCodec* pCodec)
{
    AVCodecContext* pCodecCtx = nullptr;
    do 
    {
        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (pCodecCtx == NULL)
            break;

        avcodec_parameters_to_context(pCodecCtx, par);

		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		{
			avcodec_free_context(&pCodecCtx);
			break;
		}
            
        m_pCodec = pCodec;
        m_pCodecCtx = pCodecCtx;
    } while (0);
}


void FFmpegAudioDecoder::Close()
{
    if (m_pCodecCtx)
    {
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }
}


int FFmpegAudioDecoder::Decode(const char* dataIn, int dataSize)
{
    AVPacket packet;
	av_init_packet(&packet);
	packet.data = (uint8_t*)dataIn;
	packet.size = dataSize;

	return Decode(&packet);
}

int FFmpegAudioDecoder::Decode(const AVPacket* pkt)
{
	return avcodec_send_packet(m_pCodecCtx, pkt);
}

int FFmpegAudioDecoder::recv(AVFrameRef& frame)
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
