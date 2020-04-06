#ifndef QC_MULTI_MEDIA_H
#define QC_MULTI_MEDIA_H

#include <memory>
#include <thread>
#include <mutex>
#include "mediaPub.h"
#include "QsMediaInfo.h"
#include "FrameQueue.h"
#include "PacketQueue.h"

struct AVCodecContext;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;
struct AVStream;
class AVFrameRef;
class FFmpegDemuxer;
class FFmpegVideoDecoder;
class FFmpegAudioDecoder;

class IMultiMediaNotify
{
public:
	virtual bool OnVideoFrame(const AVFrameRef& frame) = 0;
    virtual bool OnAudioFrame(const AVFrameRef& frame) = 0;
	virtual void ToEndSignal() = 0;
};

class MEDIA_API QcMultiMediaPlayer
{
public:
    QcMultiMediaPlayer(IMultiMediaNotify* pNotify);
    ~QcMultiMediaPlayer();

	bool Open(const char* pFile);
	void Play();
	void Pause();
	void Seek(int msTime);
	bool Close();
    bool IsPlaying() const {return m_playState == ePlaying;}
	bool isEnd() const;
	bool readFrame(bool bVideo, AVFrameRef& frame);

	const QsMediaInfo& getMediaInfo() const;
    bool HasVideo() const {return m_pVideoDecoder != nullptr;}
    bool HasAudio() const  {return m_pAudioDecoder != nullptr; }
	int GetCurTime() const { return m_iVideoCurTime > m_iAudioCurTime ? m_iVideoCurTime : m_iAudioCurTime; }
	int GetTotalTime() const;
protected:
    void _start();
	void SynState(int eState);
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
