#include "FFmpegDemuxer.h"

static AVRational gContextBaseTime = { 1, AV_TIME_BASE };

FFmpegDemuxer::FFmpegDemuxer()
{

}

bool FFmpegDemuxer::open(const char* pFile)
{
    close();

    if (avformat_open_input(&m_pFormatContext, pFile, NULL, NULL) != 0)
    {
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = NULL;
        return false;
    }
    if (avformat_find_stream_info(m_pFormatContext, NULL) < 0)
    {
    }

    for (int i = 0; i < m_pFormatContext->nb_streams; i++)
    {
        if (m_pVideoStream && m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            openVideoStream(i);
        }
        else if (m_pAudioStream && m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            openAudioStream(i);
        }
    }

    if (m_pFormatContext->duration != AV_NOPTS_VALUE)
    {
        m_contextTotalTime = QmBaseTimeToMSTime(m_pFormatContext->duration, gContextBaseTime);
        m_videoTotalTime = m_contextTotalTime;
        m_audioTotalTime = m_contextTotalTime;
    }
}

void FFmpegDemuxer::close()
{
    if (m_pFormatContext)
    {
        avformat_close_input(&m_pFormatContext);
        m_pFormatContext = NULL;
    }
}


void FFmpegDemuxer::openVideoStream(int i)
{
    m_pVideoStream = m_pFormatContext->streams[i];
    AVCodecParameters* p_codec_par = m_pFormatContext->streams[i]->codecpar;

    if (AV_NOPTS_VALUE != m_pVideoStream->duration)
        m_mediaInfo.m_videoTotalTime = QmBaseTimeToMSTime(m_pVideoStream->duration, m_pVideoStream->time_base);

    m_mediaInfo.iFrameRate = av_q2d(av_stream_get_r_frame_rate(m_pVideoStream));
    m_mediaInfo.iVideoWidth = p_codec_par->width;
    m_mediaInfo.iVideoHeight = p_codec_par->height;
    m_mediaInfo.videoFormat = p_codec_par->format;
    return true;
}

void FFmpegDemuxer::openAudioStream(int i)
{
    m_pAudioStream = m_pFormatContext->streams[i];
    AVCodecParameters* p_codec_par = m_pFormatContext->streams[i]->codecpar;
    if (AV_NOPTS_VALUE != m_pAudioStream->duration)
        m_mediaInfo.m_audioTotalTime = QmBaseTimeToMSTime(m_pAudioStream->duration, m_pAudioStream->time_base);

    m_mediaInfo.sample_rate = p_codec_par->sample_rate;
    m_mediaInfo.channels = p_codec_par->channels;
    m_mediaInfo.channel_layout = p_codec_par->channel_layout;
    m_mediaInfo.audioFormat = p_codec_par->format;
}
