#ifndef QC_AUDIO_TRANSFORMAT_H
#define QC_AUDIO_TRANSFORMAT_H

#include "mediaPub.h"
#include <stdint.h>
#include "QsAudiodef.h"

struct SwrContext;
struct AVFrame;
class AVFrameRef;
class MEDIA_API QcAudioTransformat
{
public:
	QcAudioTransformat();
	~QcAudioTransformat();

	bool init(const QsAudioPara& sourceInfo, const QsAudioPara& destInfo);
	const QsAudioPara& srcPara() const { return m_srcInfo; }
	const QsAudioPara& dstPara() const { return m_dstInfo; }
	int GetDelaySamples();
	bool Transformat(const uint8_t* const data[], int nb_samples, AVFrameRef& outFrame);
protected:
	void CloseSwrContext();
protected:
	QsAudioPara m_srcInfo;
	QsAudioPara m_dstInfo;
	SwrContext * m_pSwsCtx;
};
#endif
