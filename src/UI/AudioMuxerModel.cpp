#include "wasapi/WASAPIPlayer.h"
#include "AudioMuxerModel.h"
#include "wasapi/WASAPICapture.h"
#include "libmedia/AVFrameRef.h"
#include "libmedia/FFmpegUtils.h"

AudioMuxerModel::AudioMuxerModel()
{

}
AudioMuxerModel::~AudioMuxerModel()
{
	waitStop();
}

void AudioMuxerModel::setParam(const QsAudioParam& param)
{
	m_audioParam = param;
}
void AudioMuxerModel::start()
{
	waitStop();
	m_bStop = false;

	m_audioCapture = new WASAPICapture();
	m_audioCapture->setCaptureCb([this](const QsAudioData* audioData) {
		int bufferSize = getAudioBufferSize(audioData->sampleFormat, audioData->nChannels, audioData->nSamples);
		AVFrameRef audioFrame = AVFrameRef::allocAudioFrame(audioData->nSamples
			, audioData->sampleRate
			, audioData->nChannels
			, audioData->sampleFormat
			, FFmpegUtils::currentMilliSecsSinceEpoch());
		memcpy(audioFrame.data(0), audioData->data[0], bufferSize);
		m_frameNotify.invoke(audioFrame);
		});

	QsAudioParam closestParam;
	if (!m_audioCapture->init(nullptr, false, &m_audioParam, &closestParam))
		return;

	m_audioCapture->start();
}
void AudioMuxerModel::stop()
{
	m_bStop = true;
	if (m_audioCapture)
		m_audioCapture->stop();
}
void AudioMuxerModel::waitStop()
{
	stop();
	if (m_audioCapture)
	{
		delete m_audioCapture;
		m_audioCapture = nullptr;
	}
}

int32_t AudioMuxerModel::addNotify(AudioFrameCb cb)
{
	return m_frameNotify.addNotify(std::move(cb));
}
void AudioMuxerModel::removeNotify(int32_t notify)
{
	m_frameNotify.removeNotify(notify);
}

void AudioMuxerModel::muxerLoop()
{
	//TODO muxer.
}