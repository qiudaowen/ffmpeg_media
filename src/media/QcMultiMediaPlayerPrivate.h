#ifndef QC_MULTI_MEDIA_H
#define QC_MULTI_MEDIA_H

#include <memory>
#include <thread>
#include <mutex>
#include "mediaPub.h"
#include "QsMediaInfo.h"
#include "FrameQueue.h"
#include "PacketQueue.h"
#include "QcMultiMediaPlayer.h"

struct AVCodecContext;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;
struct AVStream;
class AVFrameRef;
class FFmpegDemuxer;
class FFmpegVideoDecoder;
class FFmpegAudioDecoder;

class QcMultiMediaPlayerPrivate
{
public:
    QcMultiMediaPlayerPrivate(IMultiMediaNotify* pNotify);
    ~QcMultiMediaPlayerPrivate();

	bool open(const char* pFile);
	void play();
	void pause();
	void seek(int msTime);
	bool close();
    bool isPlaying() const {return m_playState == ePlaying;}
	bool isEnd() const;
	bool readFrame(bool bVideo, AVFrameRef& frame);

	const QsMediaInfo* getMediaInfo() const;
    bool hasVideo() const {return m_pVideoDecoder != nullptr;}
    bool hasAudio() const  {return m_pAudioDecoder != nullptr; }
	int getCurTime() const;
	int getTotalTime() const;
protected:
    void _start();
	void _synState(int eState);
	bool isPacketQueueFull();
	int readPacket(bool bVideo, AVPacketPtr& ptr);
	int toSystemTime(int64_t pts, AVStream*);
	int diffToCurrentTime(const AVFrameRef& frame);

	void demuxeThread();
	void videoDecodeThread();
	void audioDecodeThread();
	void onNotifyFileEnd();
protected: 
	std::unique_ptr<FFmpegDemuxer> m_pDemuxer;
	std::unique_ptr<FFmpegVideoDecoder> m_pVideoDecoder;
	std::unique_ptr<FFmpegAudioDecoder> m_pAudioDecoder;
    
    int m_playState = eReady;
	int m_videoThreadState = eReady;
	int m_audioThreadState = eReady;
	int m_demuxerThreadState = eReady;
    std::thread m_videoThread;
    std::thread m_audioThread;
	std::thread m_demuxerThread;

	int m_iVideoCurTime = 0;
	int m_iAudioCurTime = 0;
	FrameQueue m_videoQueue;
	FrameQueue m_audioQueue;

	std::mutex m_demuxerMutex;
	PacketQueue m_videoPacketQueue;
	PacketQueue m_audioPacketQueue;
	bool m_bFileEnd = false;
	bool m_videoDecodeEnd = false;
	bool m_audioDecodeEnd = false;
	
    IMultiMediaNotify* m_pNotify = nullptr;

	int m_iBeginSystemTime = 0;
};

#endif
