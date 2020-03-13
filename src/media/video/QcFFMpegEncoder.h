#ifndef QC_FFMPEG_ENCODER_H
#define QC_FFMPEG_ENCODER_H

#include "./../IVideoEncoder.h"
#include <vector>

struct AVCodecContext;
struct AVCodec;
class QcFFMpegEncoder : public IVideoEncoder
{
public:
	QcFFMpegEncoder();
	~QcFFMpegEncoder();

	virtual void SetVideoNotify(IVideoEncodeDataNotify* pNotify);
	virtual bool Open(const QsEncodePara& para);
	virtual bool Encode(const char* const dataSlice[], int width, int height, CAMERA_DATATYPE_E eDataType, unsigned long long = 0);
	virtual int Encode(const char* const dataSlice[], int width, int height, CAMERA_DATATYPE_E eDataType, int& keyFrame, char*& outBuf, int nBufSize = 0);
	virtual void Close();
protected:
	AVCodecContext* m_pCodecCtx;
	AVCodec* m_pEncodeCodec;
	IVideoEncodeDataNotify* m_pNotify;
	QsEncodePara m_para;
	std::vector<char> m_buffer;
    int m_iLastClock;
};
#endif
