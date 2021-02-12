#ifndef QC_AUDIO_TRANSFORMAT_H
#define QC_AUDIO_TRANSFORMAT_H

#include "media_global.h"
#include <stdint.h>
#include "QsAudiodef.h"

struct SwrContext;
struct AVFrame;
class AVFrameRef;
struct QcAudioTransformatPrivate;
class MEDIA_API QcAudioTransformat
{
public:
	QcAudioTransformat();
	~QcAudioTransformat();

	bool init(const QsAudioParam& sourceInfo, const QsAudioParam& destInfo);
    const QsAudioParam& srcPara() const;
    const QsAudioParam& dstPara() const;
	int getDelaySamples();
	bool transformat(const uint8_t* const srcData[], int nb_samples, AVFrameRef& outFrame);
protected:
	void CloseSwrContext();
protected:
    QcAudioTransformatPrivate* m_ptr;
};
#endif
