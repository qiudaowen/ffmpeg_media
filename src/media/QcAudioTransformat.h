#ifndef QC_AUDIO_TRANSFORMAT_H
#define QC_AUDIO_TRANSFORMAT_H

#include "mediaPub.h"
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

	bool init(const QsAudioPara& sourceInfo, const QsAudioPara& destInfo);
    const QsAudioPara& srcPara() const;
    const QsAudioPara& dstPara() const;
	int getDelaySamples();
	bool transformat(const uint8_t* const data[], int nb_samples, AVFrameRef& outFrame);
protected:
	void CloseSwrContext();
protected:
    QcAudioTransformatPrivate* m_ptr;
};
#endif
