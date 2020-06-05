#ifndef QC_FFMPEG_DECODER_H
#define QC_FFMPEG_DECODER_H

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
    FFmpegVideoDecoder(const AVCodecParameters *par, bool hw = false);
	FFmpegVideoDecoder(int srcW, int srcH, int srcFormat, int codecID, bool hw = false);
	~FFmpegVideoDecoder();

	int Decode(const AVPacket* pkt);
	int Decode(const char* dataIn, int dataSize);
	int recv(AVFrameRef& frame);
	void flush();
protected:
    void Close();
    void Open(const AVCodecParameters *par, bool hw);
    void OpenCodec(const AVCodecParameters *par, AVCodec* pCodec, bool hw);
protected:
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec = nullptr;
	AVBufferRef * m_hw_device_ctx = nullptr;

    int m_srcW = 0;
    int m_srcH = 0;
    int m_srcFormat = 0;
    int m_codeID = 0;
};
#endif
