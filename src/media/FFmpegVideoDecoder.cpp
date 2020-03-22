#include "FFmpegVideoDecoder.h"
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

        if (hw)
        {
            AVCodec* pCodec = find_hardware_decoder(id);
            if (pCodec)
                OpenCodec(par, pCodec);
        }
        if (m_pCodec == 0)
        {
            AVCodec* pCodec = avcodec_find_decoder(id);
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
            break;

        m_pCodec = pCodec;
        m_pCodecCtx = pCodecCtx;
        return true;
    } while (0);

    if (pCodecCtx)
    {
        avcodec_free_context(pCodecCtx);
    }
    return false;
}


void FFmpegVideoDecoder::Close()
{
    if (m_pCodecCtx)
    {
        avcodec_free_context(m_pCodecCtx);
        m_pCodecCtx = nullptr;
    }
}


int FFmpegVideoDecoder::Decode(const char* dataIn, int dataSize, AVFrameRef& frame)
{
    AVPacket packet;
	av_init_packet(&packet);
	packet.data = (uint8_t*)dataIn;
	packet.size = dataSize;

    avcodec_send_packet(m_pCodecCtx, &packet);
    av_packet_unref(&packet);

    AVFrameRef newFrame(true);
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
