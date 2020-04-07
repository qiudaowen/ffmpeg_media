#include "QcAudioTransformat.h"
#include "FFmpegUtils.h"
#include "AVFrameRef.h"
extern "C" {
#include <libswresample/swresample.h>
}

struct QcAudioTransformatPrivate
{
    QsAudioPara m_srcInfo;
    QsAudioPara m_dstInfo;
    SwrContext * m_pSwsCtx = nullptr;
};

QcAudioTransformat::QcAudioTransformat()
	: m_ptr(new QcAudioTransformatPrivate())
{

}

QcAudioTransformat::~QcAudioTransformat()
{
	CloseSwrContext();
    delete m_ptr;
}

void QcAudioTransformat::CloseSwrContext()
{
	if (m_ptr->m_pSwsCtx)
	{
		swr_free(&m_ptr->m_pSwsCtx);
        m_ptr->m_pSwsCtx = nullptr;
	}
}

bool QcAudioTransformat::init(const QsAudioPara& sourceInfo, const QsAudioPara& destInfo)
{
	CloseSwrContext();
	int64_t iSrcChannelLayout = av_get_default_channel_layout(sourceInfo.nChannel);
	int64_t iDestChannelLayout = av_get_default_channel_layout(destInfo.nChannel);
	AVSampleFormat iSrcSampleFormat = (AVSampleFormat)FFmpegUtils::ToFFmpegAudioFormat(sourceInfo.eSample_fmt);
	AVSampleFormat iDestSampleFormat = (AVSampleFormat)FFmpegUtils::ToFFmpegAudioFormat(destInfo.eSample_fmt);
	
    m_ptr->m_pSwsCtx = swr_alloc_set_opts(NULL, iDestChannelLayout, iDestSampleFormat, destInfo.iSamplingFreq
		, iSrcChannelLayout, iSrcSampleFormat, sourceInfo.iSamplingFreq, 0, NULL);
	if (swr_init(m_ptr->m_pSwsCtx) < 0)
	{
		swr_free(&m_ptr->m_pSwsCtx);
        m_ptr->m_pSwsCtx = NULL;
		return false;
	}
    m_ptr->m_srcInfo = sourceInfo;
    m_ptr->m_dstInfo = destInfo;
	return true;
}

int QcAudioTransformat::getDelaySamples()
{
	if (m_ptr->m_pSwsCtx)
	{
		return (int)swr_get_delay(m_ptr->m_pSwsCtx, m_ptr->m_srcInfo.iSamplingFreq);
	}
	return 0;
}

bool QcAudioTransformat::transformat(const uint8_t* const srcData[], int srcSamples, AVFrameRef& outFrame)
{
	if (m_ptr->m_pSwsCtx == nullptr)
		return false;
	int dstNum = swr_get_out_samples(m_ptr->m_pSwsCtx, srcSamples);

    int dstChannel = m_ptr->m_dstInfo.nChannel;
	AVSampleFormat iDestSampleFormat = (AVSampleFormat)FFmpegUtils::ToFFmpegAudioFormat(m_ptr->m_dstInfo.eSample_fmt);
	if (!(outFrame.format() == iDestSampleFormat
		&& outFrame.channelCount() == dstChannel
		&& outFrame.sampleCount() > dstNum) )
	{
		outFrame = AVFrameRef::allocAudioFrame(dstNum, dstChannel, iDestSampleFormat);
	}
	int nCount = swr_convert(m_ptr->m_pSwsCtx, outFrame->data, dstNum, (const uint8_t**)srcData, srcSamples);
	outFrame->nb_samples = nCount;
	return nCount > 0;
}
