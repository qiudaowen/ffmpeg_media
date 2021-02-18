#pragma once

#include <mutex>
#include <atomic>

#include "media_global.h"
#include "QcBuffer.h"
#include "QsMediaInfo.h"
#include "QsVideodef.h"
#include "QsAudiodef.h"

enum AVCodecID;
struct AVStream;
struct AVFormatContext;
struct QsVideoParam;
struct QsAudioParam;
class MEDIA_API QcFFmpegMuxer
{
public:
	QcFFmpegMuxer();
	~QcFFmpegMuxer();

	void setVideoCodecTimeBase(const QsTimeBase& timebase);
	void setAudioCodecTimeBase(const QsTimeBase& timebase);
	void setVideoFormat(const QsVideoParam& param);
	void setAudioFormat(const QsAudioParam& param);
	void setVideoHeader(const uint8_t *pbuf, int len);
	void setAudioHeader(const uint8_t *pbuf, int len);

	bool open(const char *file);
	void close();
	int32_t duration() const { return m_muxerTime.load(); }

	bool writeVideo(int32_t pts, const uint8_t *buf, int size, bool bKeyFrame);
	bool writeVideo(int32_t pts, int32_t dts, const uint8_t *buf, int size, bool bKeyFrame);
	bool writeAudio(int32_t pts, const uint8_t *buf, int size);
protected:
	bool newStream(AVCodecID id, AVStream** stream);
	bool addVideoStream();
	bool addAudioStream();

	AVStream* videoStream() const { return m_videoStream; }
	AVStream* audioStream() const { return m_audioStream; }
protected:
	AVFormatContext *m_oc;
	AVStream *m_videoStream;
	AVStream *m_audioStream;

	QsTimeBase m_videoTimebase;
	QsTimeBase m_audioTimebase;
	
	//video
	QcBuffer        m_videoHeadBuffer;
	QsVideoParam    m_videoParam;

	//audio
	QcBuffer        m_audioHeadBuffer;
	QsAudioParam    m_audioParam;

	std::atomic_int32_t m_muxerTime = 0;
	uint64_t		m_fileLength;
	int64_t         m_startTimeMs;
	bool			m_foundKeyFrame = false;
};
