#pragma once

struct QsMediaInfo
{
    int iFrameRate;
    int videoFormat;
    int iVideoWidth;
    int iVideoHeight;
    int videoTotalTime;

    int sample_rate;
    int audioFormat;
    int channels;
    int m_audioTotalTime;

    int m_fileTotalTime;
};

class FFmpegDemuxer
{
public:
    FFmpegDemuxer();

    bool open(const char* file);
    void close();

protected:
    void openVideoStream(int i);
    void openAudioStream(int i);
protected:
    AVFormatContext* m_pFormatContext = nullptr;
    AVStream* m_pVideoStream = nullptr;
    AVStream* m_pAudioStream = nullptr;

    QsMediaInfo m_mediaInfo;
};

