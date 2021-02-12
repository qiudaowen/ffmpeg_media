#pragma once

#include <vector>
#include "QsMediaInfo.h"
#include "QsVideodef.h"

struct AVCodecContext;
struct AVCodec;
struct QsVideoParam;
class QcVideoFrame;
class AVFrameRef;
class FFmpegVideoTransformat;
class FFmpegVideoEncoder
{
public:
	enum
    {
        kOk = 0,
        kEOF,
        kAgain,
        kOtherError,
    };
	FFmpegVideoEncoder();
	~FFmpegVideoEncoder();

	void setCodecTimeBase(const QsTimeBase& timebase);
	const QsTimeBase& timebase() const { return m_timebase; }

	void setEncodeParam(const QsVideoParam& para);
	const QsVideoParam& encodeParam() const { return m_encodeParam; }

	int getExtradata(uint8_t*& extradata) const;

	bool paramValid() const;
	int width() const { return m_encodeParam.width; }
	int height() const { return m_encodeParam.height; }

	bool open();
	void close();

	int encode(AVFrameRef frame);
	int recv(AVPacketPtr& pkt);
	
	void flush();
protected:
	AVFrameRef transformat(AVFrameRef src);
protected:
	QsVideoParam    m_encodeParam;
	QsTimeBase		m_timebase;
	int64_t			m_startPts = -1;

	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pEncodeCodec = nullptr;
	FFmpegVideoTransformat* m_transformat = nullptr;
};
