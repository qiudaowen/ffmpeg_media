#ifndef QC_FFMPEG_DECODER_H
#define QC_FFMPEG_DECODER_H

struct AVCodecContext;
struct AVCodec;
struct AVCodecParameters;

class QcFFmpegDecoder
{
public:
    enum
    {
        kOk = 0,
        kEOF,
        kAgain,
        kOtherError,
    };
    QcFFmpegDecoder(const AVCodecParameters *par, bool hw = false);
	QcFFmpegDecoder(int srcW, int srcH, int srcFormat, int codecID, bool hw = false);
	~QcFFmpegDecoder();

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
