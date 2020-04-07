#pragma once

#include "mediaPub.h"

#include "QsMediaInfo.h"

struct AVFormatContext;
struct AVStream;
class MEDIA_API FFmpegDemuxer
{
public:
    FFmpegDemuxer();
    ~FFmpegDemuxer();

    bool open(const char* file);
    void close();
    
	int readPacket(AVPacketPtr& pkt);
	bool isFileEnd();
	int seek(int msTime);
	const QsMediaInfo& getMediaInfo() { return m_mediaInfo; }

    AVStream* videoStream() { return m_pVideoStream;}
	AVStream* audioStream() { return m_pAudioStream;}
protected:
    void openVideoStream(int i);
    void openAudioStream(int i);
protected:
    AVFormatContext* m_pFormatContext = nullptr;
    AVStream* m_pVideoStream = nullptr;
    AVStream* m_pAudioStream = nullptr;

    QsMediaInfo m_mediaInfo;
	bool m_fileEnd = false;
};

