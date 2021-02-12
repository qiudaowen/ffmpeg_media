#pragma once

#include "media_global.h"

struct AVCodecContext;
struct AVBufferRef;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;
class AVFrameRef;

class MEDIA_API FFmpegVideoDecoder
{
public:
    enum
    {
        kOk = 0,
        kEOF,
        kAgain,
        kOtherError,
    };
    FFmpegVideoDecoder();
	~FFmpegVideoDecoder();

	void setHwDevice(AVBufferRef* device_ctx);
	bool open(const AVCodecParameters *par);
	bool open(int srcW, int srcH, int srcFormat, int codecID);
	void close();

	int decode(const AVPacket* pkt);
	int decode(const char* dataIn, int dataSize);
	int recv(AVFrameRef& frame);
	void flush();
protected:
    void openCodec(const AVCodecParameters *par, int srcW, int srcH, int srcFormat, int codecID);
protected:
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;
	AVBufferRef * m_hw_device_ctx = nullptr;
};
