#pragma once

struct AVCodecContext;
struct AVCodec;
struct AVCodecParameters;

class FFmpegAudioDecoder
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

	int Decode(const char* dataIn, int dataSize, AVFrameRef& frame);
protected:
    void Close();
    void Open(const AVCodecParameters *par, bool hw);
    void OpenCodec(const AVCodecParameters *par, AVCodec* pCodec);
protected:
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;
};
