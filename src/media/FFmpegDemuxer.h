#pragma once

#include "media.h"

struct AVFormatContext;
struct AVStream;
class FFmpegDemuxer
{
public:
    FFmpegDemuxer();

    bool open(const char* file);
    void close();
	int readPacket(AVPacketPtr& pkt);
	int seek(double fPos);

	AVStream* videoStream() 
	{
		return m_pVideoStream;
	}
	AVStream* audioStream()
	{
		return m_pAudioStream;
	}
protected:
    void openVideoStream(int i);
    void openAudioStream(int i);
protected:
    AVFormatContext* m_pFormatContext = nullptr;
    AVStream* m_pVideoStream = nullptr;
    AVStream* m_pAudioStream = nullptr;

    QsMediaInfo m_mediaInfo;
};

