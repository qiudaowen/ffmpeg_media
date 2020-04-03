#include "QcAudioTransformat.h"
#include "QcLog.hpp"
#include "QcExceptionContextHelper.h"
#include "QcEfficiencyProfiler.h"

#define __STDC_FORMAT_MACROS
extern "C"{
#define snprintf _snprintf
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

QcAudioTransformat::QcAudioTransformat()
	: m_pSwsCtx(NULL)
{

}

QcAudioTransformat::~QcAudioTransformat()
{
	CloseSwrContext();
}

void QcAudioTransformat::CloseSwrContext()
{
	if (m_pSwsCtx)
	{
		swr_free(&m_pSwsCtx);
		m_pSwsCtx = NULL;
	}
}

int QcAudioTransformat::GetDelaySamples()
{
	if (m_pSwsCtx)
	{
		return swr_get_delay(m_pSwsCtx, m_sourceInfo.iSamplingFreq);
	}
	return 0;
}

bool QcAudioTransformat::Transformat(const char** pInData, int nb_samples, const QsAudioPara& source, const QsAudioPara& dest, char* destOut, int nBufSize, int* dest_samples)
{
	int iSampleRate = 0;
	int iSampleFormat = 0;
	int iChannelLayout = 0;
	AudioParaToFFmpegPara(source, iSampleRate, iSampleFormat, iChannelLayout);

	int iDestSampleRate = 0;
	int iDestSampleFormat = 0;
	int iDestChannelLayout = 0;
	AudioParaToFFmpegPara(dest, iDestSampleRate, iDestSampleFormat, iDestChannelLayout);

	bool bSame = true;
	if (source != m_sourceInfo)
	{
		m_sourceInfo = source;
		bSame = false;
	}
	if (dest != m_destInfo)
	{
		m_destInfo = dest;
		bSame = false;
	}

    if (!bSame || m_pSwsCtx == NULL)
	{
		if (m_pSwsCtx != NULL)
		{
			CloseSwrContext();
		}
		m_pSwsCtx = swr_alloc_set_opts(NULL, iDestChannelLayout, (AVSampleFormat)iDestSampleFormat, iDestSampleRate, iChannelLayout, (AVSampleFormat)iSampleFormat, iSampleRate, 0, NULL);
        if (m_pSwsCtx == NULL)
        {
            QmLogNormal("Fail To swr_alloc_set_opts[dest=%d %d %d Source=%d %d %d]",
                        iDestChannelLayout, iDestSampleFormat, iDestSampleRate, iChannelLayout, iSampleFormat, iSampleRate);
            return false;
        }
        if (swr_init(m_pSwsCtx) < 0)
		{
            QmLogNormal("Fail To swr_init");
			swr_free(&m_pSwsCtx);
			m_pSwsCtx = NULL;
			return false;
		}
	}

	int dst_nb_samples = av_rescale_rnd(nb_samples + swr_get_delay(m_pSwsCtx, iSampleRate), iDestSampleRate, iSampleRate, AV_ROUND_UP);
	AVFrame destAudioFrame = { 0 };
	destAudioFrame.channels = av_get_channel_layout_nb_channels(iDestChannelLayout);
	destAudioFrame.format = iDestSampleFormat;
	destAudioFrame.sample_rate = iDestSampleRate;
	destAudioFrame.nb_samples = dst_nb_samples;

	int nDestLen = av_samples_get_buffer_size(NULL, destAudioFrame.channels, dst_nb_samples, (AVSampleFormat)iDestSampleFormat, 1);
    QmAssert(nBufSize >= nDestLen);
	avcodec_fill_audio_frame(&destAudioFrame, destAudioFrame.channels, AVSampleFormat(destAudioFrame.format), (const uint8_t*)destOut, nDestLen, 1);

	int nCount = swr_convert(m_pSwsCtx, destAudioFrame.data, destAudioFrame.nb_samples, (const uint8_t**)pInData, nb_samples);
    //QmAssert(nCount == dst_nb_samples);
	//*dest_samples = dst_nb_samples;
	*dest_samples = nCount;
	return nCount > 0;
}
