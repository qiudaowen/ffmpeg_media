#include "QcAudioTransformat.h"
#include "FFmpegUtils.h"
#include "AVFrameRef.h"
extern "C" {
#include <libswresample/swresample.h>
}

struct QcAudioTransformatPrivate
{
	QsAudioParam m_srcInfo;
	QsAudioParam m_dstInfo;
	SwrContext* m_pSwsCtx = nullptr;
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

bool QcAudioTransformat::init(const QsAudioParam& sourceInfo, const QsAudioParam& destInfo)
{
	CloseSwrContext();
	int64_t iSrcChannelLayout = av_get_default_channel_layout(sourceInfo.nChannels);
	int64_t iDestChannelLayout = av_get_default_channel_layout(destInfo.nChannels);
	AVSampleFormat iSrcSampleFormat = (AVSampleFormat)FFmpegUtils::toFFmpegAudioFormat(sourceInfo.sampleFormat);
	AVSampleFormat iDestSampleFormat = (AVSampleFormat)FFmpegUtils::toFFmpegAudioFormat(destInfo.sampleFormat);

	m_ptr->m_pSwsCtx = swr_alloc_set_opts(NULL, iDestChannelLayout, iDestSampleFormat, destInfo.sampleRate
		, iSrcChannelLayout, iSrcSampleFormat, sourceInfo.sampleRate, 0, NULL);
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

const QsAudioParam& QcAudioTransformat::srcPara() const 
{
	return m_ptr->m_srcInfo;
}
const QsAudioParam& QcAudioTransformat::dstPara() const
{
	return m_ptr->m_dstInfo;
}

int QcAudioTransformat::getDelaySamples()
{
	if (m_ptr->m_pSwsCtx)
	{
		return (int)swr_get_delay(m_ptr->m_pSwsCtx, m_ptr->m_srcInfo.sampleRate);
	}
	return 0;
}

bool QcAudioTransformat::transformat(const uint8_t* const srcData[], int srcSamples, AVFrameRef& outFrame)
{
	if (m_ptr->m_pSwsCtx == nullptr)
		return false;
	int dstNum = swr_get_out_samples(m_ptr->m_pSwsCtx, srcSamples);
	if (dstNum == 0)
		return false;

	int dstChannel = m_ptr->m_dstInfo.nChannels;
	int dstSampleRate = m_ptr->m_dstInfo.sampleRate;
	AVSampleFormat iDestSampleFormat = (AVSampleFormat)FFmpegUtils::toFFmpegAudioFormat(m_ptr->m_dstInfo.sampleFormat);
	if (!(outFrame.format() == iDestSampleFormat
		&& outFrame.channelCount() == dstChannel
		&& outFrame.sampleRate() == dstSampleRate
		&& outFrame.sampleCount() > dstNum))
	{
		outFrame = AVFrameRef::allocAudioFrame(dstNum, dstSampleRate, dstChannel, iDestSampleFormat);
	}
	int nCount = swr_convert(m_ptr->m_pSwsCtx, outFrame->data, dstNum, (const uint8_t**)srcData, srcSamples);
	outFrame->nb_samples = nCount;
	return nCount > 0;
}
