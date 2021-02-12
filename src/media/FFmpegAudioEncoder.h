#pragma once

#include <vector>

#include "media_global.h"
#include "QsMediaInfo.h"
#include "QsAudiodef.h"

struct AVCodecContext;
struct AVCodec;
struct QsAudioParam;
class AVFrameRef;
class QcAudioTransformat;
class FFmpegAudioFrameBuffer;
class FFmpegAudioEncoder
{
public:
	enum
    {
        kOk = 0,
        kEOF,
        kAgain,
        kOtherError,
    };
	FFmpegAudioEncoder();
	~FFmpegAudioEncoder();

	void setCodecTimeBase(const QsTimeBase& timebase);
	const QsTimeBase& timebase() const { return m_timebase; }

	void setEncodeParam(const QsAudioParam& param);
	const QsAudioParam& encodeParam() const { return m_encodeParam; }
	bool paramValid() const;

	bool open();
	void close();

	int encode(AVFrameRef frame);
	int recv(AVPacketPtr& pkt);
	
	void flush();
protected:
	AVFrameRef transformat(AVFrameRef frame);
protected:
	QsTimeBase      m_timebase;
	QsAudioParam    m_encodeParam;
	uint64_t		m_startPts = -1;
	uint64_t        m_nSampleCount = 0;

	FFmpegAudioFrameBuffer* m_frameBuffer = nullptr;
	QcAudioTransformat* m_transformat = nullptr;
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pEncodeCodec = nullptr;
};
