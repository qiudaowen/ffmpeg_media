#ifndef QC_FFMPEG_DECODER_H
#define QC_FFMPEG_DECODER_H

#include "./../IVideoDecoder.h"
#include <vector>

struct AVCodecContext;
struct AVCodec;
struct SwsContext;
class QcVideoTransformat;
struct AVFrame;
class QcFFmpegDecoder : public IVideoDecoder
{
public:
	QcFFmpegDecoder();
	~QcFFmpegDecoder();

	virtual void SetVideoNotify(IVideoDecodeDataNotify* pNotify);
	virtual bool Open(const QsVideoInfo& destInfo);
	virtual bool Decode(const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame, void* pContext = 0);
    virtual bool Decode(const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame, char** outBuf, int bufSize);
	virtual bool Decode(AVFrame* pFrameOut, const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame);
	virtual void Close();
protected:
	bool Decode2(const char* dataIn, int dataSize, const QsVideoInfo& sourceInfo, int keyFrame, char** outBuf, int bufSize, IVideoDecodeDataNotify* pNotify = 0, void* pContext = 0);
	bool Open2(const QsVideoInfo& sourceInfo);
	void ResizeBuf(std::vector<char>& buf, int iWidth, int iHeight, CAMERA_DATATYPE_E eType);
	void CloseSwsContext();
protected:
	QsVideoInfo m_sourceInfo;
	QsVideoInfo m_destInfo;
	QcVideoTransformat* m_pTransFormat;

	std::vector<char> m_buffer;
	AVCodecContext* m_pCodecCtx;
	AVCodec* m_pDecodeCodec;

    AVFrame* m_pDecodeFrame;
	IVideoDecodeDataNotify* m_pNotify;
};
#endif
