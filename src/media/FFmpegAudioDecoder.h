#pragma once

#include "mediaPub.h"

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
	FFmpegAudioDecoder(const AVCodecParameters *par);
	~FFmpegAudioDecoder();

	int Decode(const AVPacket* pkt);
	int Decode(const char* dataIn, int dataSize);
	int recv(AVFrameRef& frame);
	void flush();
protected:
    void Close();
    void Open(const AVCodecParameters *par);
    void OpenCodec(const AVCodecParameters *par, AVCodec* pCodec);
protected:
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;
};
