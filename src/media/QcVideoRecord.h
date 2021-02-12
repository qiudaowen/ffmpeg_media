#pragma once

#include <thread>
#include <mutex>

#include "media_global.h"
#include "QsMediaInfo.h"
#include "QsVideodef.h"
#include "QsAudiodef.h"

class QcFFmpegMuxer;
class FFmpegVideoEncoder;
class FFmpegAudioEncoder;
class AVFrameRef;
class FrameQueue;
class PacketQueue;
struct QsVideoParam;
struct QsAudioParam;
class MEDIA_API QcVideoRecord
{
public:
	QcVideoRecord();
	~QcVideoRecord();

	void setVideoCodecTimeBase(const QsTimeBase& timebase);
	void setaudioCodecTimeBase(const QsTimeBase& timebase);
	void setVideoParam(const QsVideoParam& param);
	void setAudioParam(const QsAudioParam& param);

	//utf8
	bool start(const std::string& filename);
	void stop();
	bool isStop() const { return m_bStop; }
	void waitStop();
	int32_t recordTime() const { return m_recordTime.load(); }

	void pushVideoFrame(AVFrameRef videoFrame);
	void pushAudioFrame(AVFrameRef audioFrame);
protected:
	void videoEncoderLoop();
	void audioEncoderLoop();
	void muxerLoop();

	void encodeVideoFrame(AVFrameRef videoFrame);
	void encodeAudioFrame(AVFrameRef videoFrame);
protected:
	std::string m_fileName;

	FFmpegVideoEncoder* m_videoEncoder = nullptr;
	FFmpegAudioEncoder* m_audioEncoder = nullptr;
	FrameQueue* m_videoQueue = nullptr;
	FrameQueue* m_audioQueue = nullptr;
	std::condition_variable m_videoFrameNotify;
	std::condition_variable m_audioFrameNotify;

	std::thread m_videoEncodeThread;
	std::thread m_audioEncodeThread;
	std::thread m_muxerThread;
	volatile bool m_bStop = true;
	volatile bool m_bVideoEncodeEnd = true;
	volatile bool m_bAudioEncodeEnd = true;

	std::mutex m_muxerMutex;
	std::condition_variable m_muxerNotify;
	QcFFmpegMuxer* m_muxer = nullptr;
	PacketQueue* m_videoPktQueue = nullptr;
	PacketQueue* m_audioPktQueue = nullptr;

	std::atomic_int32_t m_recordTime = 0;
};