#ifndef QC_FFMPEG_DECODER_H
#define QC_FFMPEG_DECODER_H

struct AVCodecContext;
struct AVCodec;
struct AVCodecParameters;

class FFmpegVideoDecoder
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

	int Decode(const char* dataIn, int dataSize, AVFrameRef& frame);
protected:
    void Close();
    void Open(const AVCodecParameters *par, bool hw);
    void OpenCodec(const AVCodecParameters *par, AVCodec* pCodec);
protected:
	AVCodecContext* m_pCodecCtx = nullptr;
	AVCodec* m_pCodec;

    int m_srcW = 0;
    int m_srcH = 0;
    int m_srcFormat = 0;
    int m_codeID = 0;
};
#endif
