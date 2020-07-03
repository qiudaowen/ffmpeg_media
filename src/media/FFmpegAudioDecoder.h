#pragma once

#include "media_global.h"

struct AVCodecContext;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;
class AVFrameRef;

class MEDIA_API FFmpegAudioDecoder
{
public:
    enum
    {
        kOk = 0,
        kEOF,
        kAgain,
        kOtherError,
    };
	FFmpegAudioDecoder();
	~FFmpegAudioDecoder();

	void open(const AVCodecParameters *par);
	void close();

	int decode(const AVPacket* pkt);
	int decode(const char* dataIn, int dataSize);
	int recv(AVFrameRef& frame);
	void flush();
protected:
    void OpenCodec(const AVCodecParameters *par, AVCodec* pCodec);
protected:
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;
};
