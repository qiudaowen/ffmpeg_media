#include "QcMultiMediaPlayer.h"
#include "Data/Multimedia/Audio/QcAudioTransformat.h"
#include "mediaCommon.h"
#include "QcLog.hpp"
#include "QcExceptionContextHelper.h"
#include <QDebug>
#include <stdlib.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define QmAPI_MODE_NEW_API_REF_COUNT_On

#define QmBaseTimeToSecondTime(value, base) (value * double(base.num) )/(base.den)
#define QmSecondTimeToBaseTime(value, base) (value * double(base.den) )/(base.num)
#define QmBaseTimeToMSTime(value, base) int(1000 * (value * double(base.num) )/(base.den))
#define QmMSTimeToBaseTime(value, base) (value * double(base.den) )/(base.num * 1000)
static AVRational gContextBaseTime = { 1, AV_TIME_BASE };

static const char *get_error_text(const int error)
{
    static char error_buffer[255];
    av_strerror(error, error_buffer, sizeof(error_buffer));
    return error_buffer;
}

QcMultiMediaPlayer::QcMultiMediaPlayer(IMultiMediaNotify* pNotify)
    : m_pNotify(pNotify)
    , m_pFormatContext(NULL)
    , m_pVideoStream(NULL)
    , m_pAudioStream(NULL)
    , m_pVideoContext(NULL)
    , m_pAudioContext(NULL)
    , m_iVideoStream(-1)
    , m_iAudioStream(-1)
    , m_videoTotalTime(0)
    , m_audioTotalTime(0)
    , m_contextStartTime(0)
    , m_contextTotalTime(0)
    , m_iVideoCurTime(0)
    , m_iAudioCurTime(0)
    , m_iSeekTime(0)
    , m_eState(eReady)
    , m_iVideoPacketSize(0)
    , m_iAudioPacketSize(0)
    , m_iFrameRate(0)
    , m_iVideoWidth(0)
    , m_iVideoHeight(0)
    , m_iToEndFlag(0)
{

}

QcMultiMediaPlayer::~QcMultiMediaPlayer()
{
    Close();

    std::vector<AVPacket*>::iterator iter(m_UsableVideoFrameBuf.begin());
    std::vector<AVPacket*>::iterator endIter(m_UsableVideoFrameBuf.end());
    for (; iter != endIter; ++iter)
    {
        av_free(*iter);
    }
    iter = m_UsableAudioFrameBuf.begin();
    endIter = m_UsableAudioFrameBuf.end();
    for (; iter != endIter; ++iter)
    {
        av_free(*iter);
    }
}

void QcMultiMediaPlayer::RecoveryFrameBuf()
{
    std::deque<AVPacket*>::iterator iter(m_DecodeVideoFrameBuf.begin());
    std::deque<AVPacket*>::iterator endIter(m_DecodeVideoFrameBuf.end());
    for (; iter != endIter; ++iter)
    {
        av_free_packet(*iter);
        m_UsableVideoFrameBuf.push_back(*iter);
    }
    m_DecodeVideoFrameBuf.clear();
    m_iVideoPacketSize = 0;

    iter = m_DecodeAudioFrameBuf.begin();
    endIter = m_DecodeAudioFrameBuf.end();
    for (; iter != endIter; ++iter)
    {
        av_free_packet(*iter);
        m_UsableAudioFrameBuf.push_back(*iter);
    }
    m_DecodeAudioFrameBuf.clear();
    m_iAudioPacketSize = 0;
}

AVPacket* QcMultiMediaPlayer::GetUsablePacket(int iPacketType)
{
    AVPacket* pPacket = NULL;
    if (iPacketType == 1)
    {
        if (m_UsableVideoFrameBuf.empty())
            pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
        else
        {
            pPacket = m_UsableVideoFrameBuf.back();
            m_UsableVideoFrameBuf.pop_back();
        }
    }
    else if (iPacketType == 0)
    {
        if (m_UsableAudioFrameBuf.empty())
            pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
        else
        {
            pPacket = m_UsableAudioFrameBuf.back();
            m_UsableAudioFrameBuf.pop_back();
        }
    }
    return pPacket;
}

bool QcMultiMediaPlayer::Open(const char* pFile)
{
    Close();

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
        if (m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            openVideoStream(i);
        }
        else if (m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && m_iAudioStream == -1)
        {
            QmAssert(m_iAudioStream < 0);
            m_iAudioStream = i;

            m_pAudioStream = m_pFormatContext->streams[i];
            m_pAudioContext = m_pFormatContext->streams[m_iAudioStream]->codec;
            AVCodec* pAudioCodec = avcodec_find_decoder(m_pAudioContext->codec_id);
            if (pAudioCodec == NULL)
            {
                break;
            }
            m_pAudioContext->refcounted_frames = 1;

            if (avcodec_open2(m_pAudioContext, pAudioCodec, NULL) < 0)
            {
                break;
            }
            if (av_get_channel_layout_nb_channels(m_pAudioContext->channel_layout) != m_pAudioContext->channels)
            {
                m_pAudioContext->channel_layout = av_get_default_channel_layout(m_pAudioContext->channels);
            }

            if (AV_NOPTS_VALUE != m_pAudioStream->duration)
                m_audioTotalTime = QmBaseTimeToMSTime(m_pAudioStream->duration, m_pAudioStream->time_base);
            FFmpegParaToAudioPara(m_srcPara.m_para, m_pAudioContext->sample_rate, m_pAudioContext->sample_fmt, m_pAudioContext->channel_layout);
            m_srcPara.iBlock_align = m_pAudioContext->block_align;
            if (m_srcPara.m_para.nChannel > 2)
            {
                QmLogLEVEL1("%s[AudioChannel=%d]", pFile, m_srcPara.m_para.nChannel);
            }
        }
        else if (m_pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {

        }
    }

    //m_contextStartTime = QmBaseTimeToMSTime(m_pFormatContext->start_time, gContextBaseTime);
    //m_contextTotalTime = QmBaseTimeToMSTime(m_pFormatContext->duration, gContextBaseTime);
    m_contextStartTime = 0;
    m_contextTotalTime = (m_audioTotalTime > m_videoTotalTime) ? m_audioTotalTime : m_videoTotalTime;
    if (m_contextTotalTime == 0 && m_pFormatContext)
    {
        m_contextTotalTime = QmBaseTimeToMSTime(m_pFormatContext->duration, gContextBaseTime);
        m_videoTotalTime = m_contextTotalTime;
        m_audioTotalTime = m_contextTotalTime;
    }
    m_iVideoCurTime = 0;
    m_iAudioCurTime = 0;

    m_iSeekTime = 0;
    m_iVideoPacketSize = 0;
    m_iAudioPacketSize = 0;
    m_eState = eReady;
    m_iToEndFlag = 0;

    return true;
}

bool QcMultiMediaPlayer::openAudioStream(int i)
{
    m_iAudioStream = i;
    m_
}

bool QcMultiMediaPlayer::Close()
{
    if (m_pFormatContext == NULL)
        return false;

    SynState(eRecordExitThread);

    QmExceptionCatch();
    RecoveryFrameBuf();

    QmExceptionCatch();
    //Audio
    if (m_pAudioStream)
    {
        avcodec_close(m_pAudioStream->codec);
        m_pAudioStream = NULL;
    }

    //Video
    if (m_pVideoStream)
    {
        avcodec_close(m_pVideoStream->codec);
        m_pVideoStream = NULL;
    }

    //Free Context
    if (m_pFormatContext)
    {
        avformat_close_input(&m_pFormatContext);
        //avformat_free_context(m_pFormatContext);
        m_pFormatContext = NULL;
    }

    m_videoTotalTime = 0;
    m_audioTotalTime = 0;
    m_contextStartTime = 0;
    m_contextTotalTime = 0;

    m_iVideoCurTime = 0;
    m_iAudioCurTime = 0;

    m_iSeekTime = 0;
    m_iVideoPacketSize = 0;
    m_iAudioPacketSize = 0;
    m_eState = eReady;

    return true;
}

void QcMultiMediaPlayer::Play()
{
    if (m_eState != ePlaying && m_pFormatContext)
    {
        m_startTm.start();
        SynState(ePlaying);
    }
}

void QcMultiMediaPlayer::Pause()
{
    if (m_eState == ePlaying && m_pFormatContext)
    {
        SynState(ePause);
        m_iSeekTime = GetCurTime();
    }
}

bool QcMultiMediaPlayer::ToEndSlot()
{
    if (m_iToEndFlag == -1)
    {
        Close();
        m_iToEndFlag = 0;
        return true;
    }
    return false;
}

void QcMultiMediaPlayer::SynState(QsPlayState eState)
{
    QmExceptionCatch();
    m_eState = eState;
    while (isRunning() && m_eState != m_eThreadState)
        QThread::yieldCurrentThread();
}

void QcMultiMediaPlayer::Seek(double fPos)
{
    if (m_pFormatContext == NULL)
        return;

    QmExceptionCatch();
    QsPlayState iState = m_eState;
    SynState(ePause);

    m_iSeekTime = (int)m_contextTotalTime * fPos;
    m_iVideoCurTime = m_iSeekTime;
    m_iAudioCurTime = m_iSeekTime;

    int64_t seek_target = QmMSTimeToBaseTime(m_contextTotalTime * fPos, gContextBaseTime);
    int64_t rang = QmMSTimeToBaseTime(200, gContextBaseTime);

    int iRet = av_seek_frame(m_pFormatContext, -1, seek_target, AVSEEK_FLAG_BACKWARD);
    if (iRet < 0)
    {
        QmExceptionCatch();
        av_seek_frame(m_pFormatContext, -1, seek_target, AVSEEK_FLAG_FRAME);
    }

    QmExceptionCatch();
    RecoveryFrameBuf();
    if(HasVideo())
        avcodec_flush_buffers(m_pVideoStream->codec);
    if(HasAudio())
        avcodec_flush_buffers(m_pAudioStream->codec);
    m_iAudioPacketSize = 0;
    m_iVideoPacketSize = 0;

    bool bLoop = true;
    while (bLoop)
    {
        AVPacket pkt = { 0 };
        av_init_packet(&pkt);
        int ret = av_read_frame(m_pFormatContext, &pkt);
        if (ret >= 0)
        {
            if (m_pVideoStream && m_pVideoStream->index && pkt.stream_index)
            {
                QmExceptionCatch();
                int iVideoPlayTime = m_iVideoCurTime;
                if (pkt.dts != AV_NOPTS_VALUE)
                    iVideoPlayTime = QmBaseTimeToMSTime(pkt.dts, m_pVideoStream->time_base) - m_contextStartTime;
                if(HasVideo())
                {
                    bLoop = !DecodeVideo(&pkt, iVideoPlayTime >= m_iSeekTime);
                }
            }
            else if (m_pAudioStream && m_pAudioStream->index ==  pkt.stream_index)
            {
                int iAudioPlayTime = m_iAudioCurTime;
                if (pkt.dts != AV_NOPTS_VALUE)
                    iAudioPlayTime = QmBaseTimeToMSTime(pkt.dts, m_pAudioStream->time_base) - m_contextStartTime;

                if (iAudioPlayTime >= m_iSeekTime && m_pNotify->OnAudioPacket((const char*)pkt.data, pkt.size))
                {
                    QmExceptionCatch();
                    if(HasAudio())
                    {
                        bLoop = !DecodeAudio(&pkt);
                    }
                }
            }
        }
        else
        {
            bLoop = false;
        }
        av_free_packet(&pkt);
    }
    m_iToEndFlag = 0;
    m_startTm.start();
    SynState(iState);
}

void QcMultiMediaPlayer::run()
{
    QmExceptionCatch();
    for (;;)
    {
        m_eThreadState = m_eState;
        if (m_eState == eRecordExitThread)
            break;

        switch (m_eState)
        {
        case eReady:
        case ePause:
        case ePlayToEnd:
        {
            QThread::msleep(20);
            break;
        }
        case ePlaying:
        {
            CallBack();


            if (m_iToEndFlag)
            {
                if (m_iToEndFlag == 1 && m_DecodeVideoFrameBuf.size() == 0 && m_DecodeAudioFrameBuf.size() == 0)
                {
                    bool bFinished = true;
#if 1
                    int iRealCurrentTime = (m_startTm.elapsed() + m_iSeekTime);
                    int iDiffTime = m_iAudioCurTime - iRealCurrentTime;
                    if (iDiffTime < 10)
                    {
                        AVPacket emptyPacket = { 0 };
                        if(HasAudio())
                            DecodeAudio(&emptyPacket);
                    }

                    iRealCurrentTime = (m_startTm.elapsed() + m_iSeekTime);
                    iDiffTime = m_iVideoCurTime - iRealCurrentTime;
                    if (iDiffTime < 5)
                    {
                        AVPacket emptyPacket = { 0 };
                        if(HasVideo())
                            DecodeVideo(&emptyPacket);
                    }
#endif					
                    if (bFinished)
                    {
                        m_iToEndFlag = -1;
                        m_pNotify->ToEndSignal();
                    }
                }
                QThread::msleep(10);
                continue;
            }

            AVPacket pkt = {0};
            av_init_packet(&pkt);
            int ret = av_read_frame(m_pFormatContext, &pkt);
            if (ret >= 0)
            {
                if (pkt.stream_index == m_iVideoStream)
                {
                    AVPacket* pPacket = GetUsablePacket(1);
                    *pPacket = pkt;
                    m_DecodeVideoFrameBuf.push_back(pPacket);
                    m_iVideoPacketSize += pPacket->size;
                }
                else if (pkt.stream_index == m_iAudioStream)
                {
                    AVPacket* pPacket = GetUsablePacket(0);
                    *pPacket = pkt;
                    m_DecodeAudioFrameBuf.push_back(pPacket);
                    m_iAudioPacketSize += pPacket->size;
                }
                else
                {
                    av_free_packet(&pkt);
                }
            }
#ifdef QmVSVersion
            else if (ret == AVERROR_EOF || url_feof(m_pFormatContext->pb))
#else
            else if (ret == AVERROR_EOF || url_feof(m_pFormatContext->pb))
#endif
            {
                m_iToEndFlag = 1;
                av_free_packet(&pkt);
            }
            break;
        }
        }
    }
}

void QcMultiMediaPlayer::CallBack()
{
    QmExceptionCatch();

    int iRealCurrentTime = (m_startTm.elapsed() + m_iSeekTime);

    int iAudioSleepTime = 0;
    while (!m_DecodeAudioFrameBuf.empty())
    {
        AVPacket* pAudioPacket = m_DecodeAudioFrameBuf.front();

        int iAudioPlayTime = m_iAudioCurTime;
        if (pAudioPacket->dts != AV_NOPTS_VALUE)
            iAudioPlayTime = QmBaseTimeToMSTime(pAudioPacket->dts, m_pAudioStream->time_base) - m_contextStartTime;

        int iDiffTime = iAudioPlayTime - iRealCurrentTime;
        if (iDiffTime < 10)
        {
            m_iAudioCurTime = iAudioPlayTime;

            m_iAudioPacketSize -= pAudioPacket->size;
            if (m_pNotify->OnAudioPacket((const char*)pAudioPacket->data, pAudioPacket->size))
            {
                DecodeAudio(pAudioPacket);
            }

            m_DecodeAudioFrameBuf.pop_front();
            m_UsableAudioFrameBuf.push_back(pAudioPacket);
            av_free_packet(pAudioPacket);

            if (iRealCurrentTime - iAudioPlayTime > 30)
            {
                QmLogNormal("Repair Time[realTime=%d playTm=%d]", iRealCurrentTime, iAudioPlayTime);
                m_iSeekTime = iAudioPlayTime;
                m_startTm.start();
                iRealCurrentTime = iAudioPlayTime;
            }
        }
        else
        {
            iAudioSleepTime = iDiffTime;
            break;
        }
    }

    //Video
    int iVideoSleepTime = 0;
    while (!m_DecodeVideoFrameBuf.empty())
    {
        AVPacket* pVideoPacket = m_DecodeVideoFrameBuf.front();

        int iVideoPlayTime = m_iVideoCurTime;
        if (pVideoPacket->dts != AV_NOPTS_VALUE)
            iVideoPlayTime = QmBaseTimeToMSTime(pVideoPacket->dts, m_pVideoStream->time_base) - m_contextStartTime;

        int iDiffTime = iVideoPlayTime - iRealCurrentTime;
        if (iDiffTime < 5)
        {
            m_iVideoCurTime = iVideoPlayTime;

            m_iVideoPacketSize -= pVideoPacket->size;
            DecodeVideo(pVideoPacket);

            m_DecodeVideoFrameBuf.pop_front();
            m_UsableVideoFrameBuf.push_back(pVideoPacket);
            av_free_packet(pVideoPacket);

            if (iRealCurrentTime - iVideoPlayTime > 30)
            {
                QmLogNormal("Repair Time[realTime=%d playTm=%d]", iRealCurrentTime, iVideoPlayTime);
                m_iSeekTime = iVideoPlayTime;
                m_startTm.start();
                iRealCurrentTime = iVideoPlayTime;
            }
        }
        else
        {
            iVideoSleepTime = iDiffTime;
            break;
        }
    }
}

bool QcMultiMediaPlayer::DecodeVideo(AVPacket* pVideoPacket, bool bShowFrame)
{
    QmExceptionCatch();
    int iGotFrame = 0;
    int iDecodeRet = avcodec_decode_video2(m_pVideoContext, m_pVideoFrame, &iGotFrame, pVideoPacket);
    if (iDecodeRet < 0)
    {
        QmLogLEVEL1("Error!avcodec_decode_video2: [%d:%s]", iDecodeRet, get_error_text(iDecodeRet));
        iGotFrame = 0;
    }
    else
    {
        QmAssert(iDecodeRet == pVideoPacket->size);
    }
    if (iGotFrame)
    {
        if (bShowFrame)
        {
            QmExceptionCatch();
            AVFrame& decodeFrame = *m_pVideoFrame;
            QsVideoFrame frame = { 0 };
            frame.dataSlice[0] = (const char*)decodeFrame.data[0];
            frame.dataSlice[1] = (const char*)decodeFrame.data[1];
            frame.dataSlice[2] = (const char*)decodeFrame.data[2];
            frame.lineSize[0] = decodeFrame.linesize[0];
            frame.lineSize[1] = decodeFrame.linesize[1];
            frame.lineSize[2] = decodeFrame.linesize[2];
            frame.iDestWidth = decodeFrame.width;
            frame.iDestHeight = decodeFrame.height;
            frame.eDataType = CAMERA_DATATYPE_YV12;
            m_pNotify->OnVideoFrame(&frame);
        }
        m_iVideoCurTime += 1000 / m_iFrameRate;
        if (m_pVideoContext->refcounted_frames > 0)
            av_frame_unref(m_pVideoFrame);
    }
    return iGotFrame;
}
bool QcMultiMediaPlayer::DecodeAudio(AVPacket* pAudioPacket)
{
    bool bGotFrame = false;
    while (pAudioPacket->size > 0)
    {
        QmExceptionCatch();
        int iGotFrame = 0;
        int nLen = avcodec_decode_audio4(m_pAudioContext, m_pAudioFrame, &iGotFrame, pAudioPacket);
        if (nLen < 0)
        {
            QmLogLEVEL1("Error!avcodec_decode_audio4: [%d:%s]", nLen, get_error_text(nLen));
            break;
        }
        if (iGotFrame)
        {
            {
                QmExceptionCatch();
                m_pNotify->OnAudioFrame((const char**)m_pAudioFrame->extended_data, m_pAudioFrame->nb_samples, m_srcPara.m_para);
            }
            if (m_pAudioContext->refcounted_frames > 0)
                av_frame_unref(m_pAudioFrame);

            bGotFrame = true;
            m_iAudioCurTime += ((m_pAudioFrame->nb_samples * 1000) / m_srcPara.m_para.iSamplingFreq);
        }
        else
        {
            QmLogLEVEL1("Do not got frame in avcodec_decode_audio4");
        }
        pAudioPacket->data += nLen;
        pAudioPacket->size -= nLen;
    }
    return bGotFrame;
}
