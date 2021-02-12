#include "QcVideoRecord.h"
#include "QmMacro.h"
#include "QcFFmpegMuxer.h"
#include "FFmpegVideoEncoder.h"
#include "FFmpegAudioEncoder.h"
#include "FrameQueue.h"
#include "PacketQueue.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

QcVideoRecord::QcVideoRecord()
{
	m_videoQueue = new FrameQueue(10);
	m_audioQueue = new FrameQueue(10);

	m_videoPktQueue = new PacketQueue();
	m_audioPktQueue = new PacketQueue();

	m_videoEncoder = new FFmpegVideoEncoder();
	m_audioEncoder = new FFmpegAudioEncoder();

	m_muxer = new QcFFmpegMuxer();
}

QcVideoRecord::~QcVideoRecord()
{
	waitStop();
	if (m_videoQueue) {
		delete m_videoQueue;
		m_videoQueue = nullptr;
	}
	if (m_audioQueue) {
		delete m_audioQueue;
		m_audioQueue = nullptr;
	}
	if (m_videoEncoder) {
		delete m_videoEncoder;
		m_videoEncoder = nullptr;
	}
	if (m_audioEncoder) {
		delete m_audioEncoder;
		m_audioEncoder = nullptr;
	}
	if (m_videoPktQueue)
	{
		delete m_videoPktQueue;
		m_videoPktQueue = nullptr;
	}
	if (m_audioPktQueue)
	{
		delete m_audioPktQueue;
		m_audioPktQueue = nullptr;
	}
	if (m_muxer)
	{
		delete m_muxer;
		m_muxer = nullptr;
	}
}

void QcVideoRecord::setVideoCodecTimeBase(const QsTimeBase& timebase)
{
	m_videoEncoder->setCodecTimeBase(timebase);
}
void QcVideoRecord::setaudioCodecTimeBase(const QsTimeBase& timebase)
{
	m_audioEncoder->setCodecTimeBase(timebase);
}

void QcVideoRecord::setVideoParam(const QsVideoParam& param)
{
	m_videoEncoder->setEncodeParam(param);
}
void QcVideoRecord::setAudioParam(const QsAudioParam& param)
{
	m_audioEncoder->setEncodeParam(param);
}

bool QcVideoRecord::start(const std::string& filename)
{
	waitStop();
	m_fileName = filename;

	m_bStop = false;
	m_bVideoEncodeEnd = false;
	m_bAudioEncodeEnd = false;
	if (m_videoEncoder->paramValid()) {
		m_videoEncodeThread = std::thread([this] { videoEncoderLoop(); });
	}
	else {
		m_bVideoEncodeEnd = true;
	}
	if (m_audioEncoder->paramValid()) {
		m_audioEncodeThread = std::thread([this] { audioEncoderLoop(); });
	}
	else {
		m_bAudioEncodeEnd = true;
	}
	m_muxerThread = std::thread([this] { muxerLoop(); });

	return true;
}
void QcVideoRecord::stop()
{
	m_bStop = true;
	{
		QmStdMutexLocker(m_videoQueue->mutex());
		m_videoFrameNotify.notify_all();
	}
	{
		QmStdMutexLocker(m_audioQueue->mutex());
		m_audioFrameNotify.notify_all();
	}
	{
		QmStdMutexLocker(m_muxerMutex);
		m_muxerNotify.notify_one();
	}
}

void QcVideoRecord::waitStop()
{
	stop();
	if (m_videoEncodeThread.joinable())
		m_videoEncodeThread.join();
	if (m_audioEncodeThread.joinable())
		m_audioEncodeThread.join();
	if (m_muxerThread.joinable())
		m_muxerThread.join();
}

void QcVideoRecord::pushVideoFrame(AVFrameRef videoFrame)
{
	if (m_bStop)
		return;
	if (!m_videoEncoder->paramValid())
		return;

	QmStdMutexLocker(m_videoQueue->mutex());
	m_videoQueue->push(videoFrame, true);
	m_videoFrameNotify.notify_all();
}
void QcVideoRecord::pushAudioFrame(AVFrameRef audioFrame)
{
	if (m_bStop)
		return;
	if (!m_audioEncoder->paramValid())
		return;

	QmStdMutexLocker(m_audioQueue->mutex());
	m_audioQueue->push(audioFrame, true);
	m_audioFrameNotify.notify_all();
}

void QcVideoRecord::videoEncoderLoop()
{
	do
	{
		if (!m_videoEncoder->open())
			return;

		uint8_t* extradata = nullptr;
		int nLen = m_videoEncoder->getExtradata(extradata);
		m_muxer->setVideoHeader(extradata, nLen);
		m_muxer->setVideoFormat(m_videoEncoder->encodeParam());
		m_muxer->setVideoCodecTimeBase(m_videoEncoder->timebase());
		while (true) {
			AVFrameRef frame;
			{
				std::unique_lock<std::mutex> lock(m_videoQueue->mutex());
				if (m_videoQueue->size() == 0) {
					if (m_bStop)
						break;
					m_videoFrameNotify.wait(lock);
				}
				m_videoQueue->pop(frame);
			}
			encodeVideoFrame(frame);
		}
		encodeVideoFrame(AVFrameRef());
		m_videoEncoder->close();
	} while (0);
	QmStdMutexLocker(m_muxerMutex);
	m_bVideoEncodeEnd = true;
	m_muxerNotify.notify_one();
}

void QcVideoRecord::encodeVideoFrame(AVFrameRef frame)
{
	m_videoEncoder->encode(frame);
	AVPacketPtr pkt;
	while (m_videoEncoder->recv(pkt) == FFmpegVideoEncoder::kOk)
	{
		//push to muxer.
		QmStdMutexLocker(m_muxerMutex);
		m_videoPktQueue->push(pkt);
		m_muxerNotify.notify_one();
	}
}

void QcVideoRecord::audioEncoderLoop()
{
	do
	{
		if (!m_audioEncoder->open())
			return;

		m_muxer->setAudioFormat(m_audioEncoder->encodeParam());
		m_muxer->setAudioCodecTimeBase(m_audioEncoder->timebase());
		while (true) {
			AVFrameRef frame;
			{
				std::unique_lock<std::mutex> lock(m_audioQueue->mutex());
				if (m_audioQueue->size() == 0) {
					if (m_bStop)
						break;
					m_audioFrameNotify.wait(lock);
				}
				m_audioQueue->pop(frame);
			}
			encodeAudioFrame(frame);
		}
		encodeAudioFrame(AVFrameRef());
		m_audioEncoder->close();
	} while (0);

	QmStdMutexLocker(m_muxerMutex);
	m_bAudioEncodeEnd = true;
	m_muxerNotify.notify_one();
}

void QcVideoRecord::encodeAudioFrame(AVFrameRef audioFrame)
{
	m_audioEncoder->encode(audioFrame);
	AVPacketPtr pkt;
	while (m_audioEncoder->recv(pkt) == FFmpegAudioEncoder::kOk)
	{
		//push to muxer.
		QmStdMutexLocker(m_muxerMutex);
		m_audioPktQueue->push(pkt);
		m_muxerNotify.notify_one();
	}
}

void QcVideoRecord::muxerLoop()
{
	bool bOpen = false;
	while (true) {
		AVPacketPtr videoPtk;
		AVPacketPtr audioPtk;
		{
			std::unique_lock<std::mutex> lock(m_muxerMutex);
			if (m_videoPktQueue->size() == 0 && m_audioPktQueue->size() == 0) {
				if (m_bStop && m_bAudioEncodeEnd && m_bVideoEncodeEnd)
					break;
				m_muxerNotify.wait(lock);
			}
			m_videoPktQueue->pop(videoPtk);
			m_audioPktQueue->pop(audioPtk);
		}
		if (!bOpen)
		{
			if (!m_muxer->open(m_fileName.c_str()))
				break;
			bOpen = true;
		}
		if (videoPtk)
		{
			m_muxer->writeVideo(videoPtk->pts, videoPtk->data, videoPtk->size, videoPtk->flags & AV_PKT_FLAG_KEY);
		}
		if (audioPtk)
		{
			m_muxer->writeAudio(audioPtk->pts, audioPtk->data, audioPtk->size);
		}
		m_recordTime.store(m_muxer->duration());
	}
	m_muxer->close();
}