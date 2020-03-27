#pragma once

#include "media.h"

struct AVFormatContext;
struct AVStream;
class FFmpegDemuxer
{
public:
    FFmpegDemuxer();
    ~FFmpegDemuxer();

    bool open(const char* file);
    void start();
    void 
    void close();
    
	int readPacket(int type, AVPacketPtr& pkt);
	int seek(double fPos);

    AVStream* videoStream() { return m_pVideoStream;}
	AVStream* audioStream() { return m_pAudioStream;}
protected:
    void openVideoStream(int i);
    void openAudioStream(int i);

    void demuxeThread();
protected:
    AVFormatContext* m_pFormatContext = nullptr;
    AVStream* m_pVideoStream = nullptr;
    AVStream* m_pAudioStream = nullptr;

    std::atomic<int> m_demuxerState = eReady;
    std::atomic<int> m_demuxerThreadState = eReady;
    std::thread m_demuxerThread;

    std::mutex m_mutex;
    PacketQueue m_videoPacketQueue;
    PacketQueue m_audioPacketQueue;
    bool m_fileEnd;

    QsMediaInfo m_mediaInfo;
};

