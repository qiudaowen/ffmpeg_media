#ifndef QC_AUDIO_TRANSFORMAT_H
#define QC_AUDIO_TRANSFORMAT_H

#include "Data/Multimedia/mediaCommon.h"
#include "Data/AudioDevice/QsAudioPara.h"
struct SwrContext;
struct AVFrame;

class QcAudioTransformat
{
public:
	QcAudioTransformat();
	~QcAudioTransformat();

	int GetDelaySamples();
	bool Transformat(const char** data, int nb_samples, const QsAudioPara& sourceInfo, const QsAudioPara& destInfo, char* destOut, int nBufSize, int* dest_samples);
protected:
	void CloseSwrContext();
protected:
	QsAudioPara m_sourceInfo;
	QsAudioPara m_destInfo;
	SwrContext * m_pSwsCtx;
};
#endif
