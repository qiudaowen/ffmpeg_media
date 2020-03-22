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

void FFmpegAudioDecoder::Open(const AVCodecParameters *par)
{
    do
    {
        Close();

        AVCodec* pCodec = avcodec_find_decoder(id);
        if (pCodec)
            OpenCodec(par, pCodec);
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

            avcodec_parameters_to_context(pCodecCtx, par);
            m_srcW = par->width;
            m_srcH = par->height;
            m_srcFormat = par->format;
            m_codeID = par->codec_id;

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
