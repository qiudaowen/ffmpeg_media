#include "QcMultiMediaPlayerPrivate.h"
extern "C"{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "FFmpegDemuxer.h"
#include "FFmpegVideoDecoder.h"
#include "FFmpegAudioDecoder.h"
#include "FFmpegUtils.h"

#include <windows.h>


static const char *get_error_text(const int error)
{
    static char error_buffer[255];
    av_strerror(error, error_buffer, sizeof(error_buffer));
    return error_buffer;
}

QcMultiMediaPlayerPrivate::QcMultiMediaPlayerPrivate(IMultiMediaNotify* pNotify)
    : m_pNotify(pNotify)

{

}

QcMultiMediaPlayerPrivate::~QcMultiMediaPlayerPrivate()
{
    close();
}

void QcMultiMediaPlayerPrivate::setHwDevice(AVBufferRef* device_ctx)
{
	av_buffer_unref(&m_hw_device_ctx);
	if (device_ctx)
		m_hw_device_ctx = av_buffer_ref(device_ctx);
}

bool QcMultiMediaPlayerPrivate::open(const char* pFile)
{
    close();

	m_pDemuxer = std::make_unique<FFmpegDemuxer>();
	bool bOk = m_pDemuxer->open(pFile);
	if (bOk)
	{
		AVStream* pVideoStream = m_pDemuxer->videoStream();
		if (pVideoStream)
		{
			m_pVideoDecoder = std::make_unique<FFmpegVideoDecoder>();
			m_pVideoDecoder->setHwDevice(m_hw_device_ctx);
			m_pVideoDecoder->open(pVideoStream->codecpar);
		}
		AVStream* pAudioStream = m_pDemuxer->audioStream();
		if (pAudioStream)
		{
			m_pAudioDecoder = std::make_unique<FFmpegAudioDecoder>();
			m_pAudioDecoder->open(pAudioStream->codecpar);
		}
	}
	_start();
    return true;
}

bool QcMultiMediaPlayerPrivate::close()
{
	if (!m_pDemuxer)
		return false;

	_synState(eExitThread);
	if (m_videoThread.joinable())
		m_videoThread.join();
	if (m_audioThread.joinable())
		m_audioThread.join();
	if (m_demuxerThread.joinable())
		m_demuxerThread.join();

	m_pVideoDecoder->close();
	av_buffer_unref(&m_hw_device_ctx);

	m_pAudioDecoder->close();
	m_pDemuxer = nullptr;

    m_playState = eReady;
    m_videoThreadState = eReady;
    m_audioThreadState = eReady;
    m_demuxerThreadState = eReady;

    m_iVideoCurTime = 0;
    m_iAudioCurTime = 0;
    m_videoQueue.clear();
    m_audioQueue.clear();

    m_videoPacketQueue.clear();
    m_audioPacketQueue.clear();
	m_bFileEnd = false;
	m_videoDecodeEnd = false;
	m_audioDecodeEnd = false;

    m_iBeginSystemTime = 0;
    return true;
}

bool QcMultiMediaPlayerPrivate::isEnd() const
{
	bool bEnd = (m_videoDecodeEnd || !hasVideo()) &&
		(m_audioDecodeEnd || !hasAudio()) && m_bFileEnd;
	return bEnd;
}

bool QcMultiMediaPlayerPrivate::readFrame(bool bVideo, AVFrameRef& frame)
{
	if (!m_pDemuxer)
		return false;

	if (bVideo)
	{
		QmStdMutexLocker(m_videoQueue.mutex());
		bool bRet = m_videoQueue.pop(frame);
		if (bRet)
			m_iVideoCurTime = frame.ptsMsTime();
		return bRet;
	}
		
	QmStdMutexLocker(m_audioQueue.mutex());
	bool bRet = m_audioQueue.pop(frame);
	if (bRet)
		m_iAudioCurTime = frame.ptsMsTime();
	return bRet;
}

const QsMediaInfo* QcMultiMediaPlayerPrivate::getMediaInfo() const
{
	return m_pDemuxer ? &(m_pDemuxer->getMediaInfo()) : NULL;
}

int QcMultiMediaPlayerPrivate::getCurTime() const 
{ 
	return m_iVideoCurTime > m_iAudioCurTime ? m_iVideoCurTime : m_iAudioCurTime; 
}

int QcMultiMediaPlayerPrivate::getTotalTime() const
{
	return m_pDemuxer ? m_pDemuxer->getMediaInfo().iFileTotalTime : 0;
}

void QcMultiMediaPlayerPrivate::play()
{
	if (m_pDemuxer == nullptr)
		return;

	if (m_playState != ePlaying)
	{
        m_iBeginSystemTime = (int)FFmpegUtils::currentMilliSecsSinceEpoch() - getCurTime();
		_synState(ePlaying);
	}	
}

void QcMultiMediaPlayerPrivate::pause()
{
	if (m_pDemuxer == nullptr)
		return;

	if (m_playState != ePause)
	{
		_synState(ePause);
	}
}

void QcMultiMediaPlayerPrivate::seek(int msTime)
{
	if (m_pDemuxer == nullptr)
		return;

	//TODO: ÒÆµ½ Demuxer Ïß³Ì
	int lastState = m_playState;
	if (m_playState == ePlaying)
		_synState(ePause);

	m_pDemuxer->seek(msTime);
	m_videoPacketQueue.clear();
	m_audioPacketQueue.clear();

	m_pVideoDecoder->flush();
	m_pAudioDecoder->flush();
	m_videoQueue.clear();
	m_audioQueue.clear();
	m_bFileEnd = false;
	m_videoDecodeEnd = false;
	m_audioDecodeEnd = false;

	m_iVideoCurTime = msTime;
	m_iAudioCurTime = msTime;
	m_iBeginSystemTime = (int)FFmpegUtils::currentMilliSecsSinceEpoch() - msTime;
	if (lastState == ePlaying)
		_synState(ePlaying);
}


void QcMultiMediaPlayerPrivate::_synState(int eState)
{
	m_playState = eState;
	while ((hasVideo() && m_playState != m_videoThreadState)
		|| (hasAudio() && m_playState != m_audioThreadState)
		|| (m_demuxerThread.joinable() && m_playState != m_demuxerThreadState))
		SwitchToThread();
}

void QcMultiMediaPlayerPrivate::_start()
{
	m_playState = eReady;

	m_demuxerThread = std::thread([this] { demuxeThread(); });
	m_videoThread = std::thread([this] {videoDecodeThread(); });
	m_audioThread = std::thread([this] {audioDecodeThread(); });
}

bool QcMultiMediaPlayerPrivate::isPacketQueueFull()
{
	std::lock_guard<std::mutex> lck(m_demuxerMutex);
	return m_videoPacketQueue.packetSize() + m_audioPacketQueue.packetSize() > 15 * 1024 * 1024;
}

int QcMultiMediaPlayerPrivate::readPacket(bool bVideo, AVPacketPtr& ptr)
{
	bool bRet = false;
	std::lock_guard<std::mutex> lck(m_demuxerMutex);
	if (bVideo)
	{
		bRet = m_videoPacketQueue.pop(ptr);
	}
	else
	{
		bRet = m_audioPacketQueue.pop(ptr);
	}
	return bRet;
}

int QcMultiMediaPlayerPrivate::toMediaTime(int64_t pts, AVStream* pStream)
{
	int iTime = 0;
	if (pStream == nullptr)
	{
		iTime = QmBaseTimeToMSTime(pts, (*FFmpegUtils::contextBaseTime()));
	}
	else
	{
		iTime = QmBaseTimeToMSTime(pts, pStream->time_base);
	}
	return iTime;
}

int QcMultiMediaPlayerPrivate::diffToCurrentTime(const AVFrameRef& frame)
{
	int iDiff = (int)frame.ptsMsTime() - (FFmpegUtils::currentMilliSecsSinceEpoch() - m_iBeginSystemTime);
	return iDiff;
}

void QcMultiMediaPlayerPrivate::demuxeThread()
{
	for (;;)
	{
		m_demuxerThreadState = m_playState;
		if (m_demuxerThreadState == eExitThread)
			break;

		if (m_bFileEnd)
		{
			::Sleep(10);
			continue;
		}
		switch (m_demuxerThreadState)
		{
		case eReady:
		case ePause:
		{
			::Sleep(10);
			break;
		}
		case ePlaying:
		{
			if (isPacketQueueFull())
			{
				::Sleep(10);
				continue;
			}

			AVPacketPtr pkt = FFmpegUtils::allocAVPacket();
			int iRet = m_pDemuxer->readPacket(pkt);
			if (iRet == 0)
			{
				std::lock_guard<std::mutex> lck(m_demuxerMutex);
				if (m_pDemuxer->videoStream() && pkt->stream_index == m_pDemuxer->videoStream()->index)
				{
					m_videoPacketQueue.push(pkt);
				}
				if (m_pDemuxer->audioStream() && pkt->stream_index == m_pDemuxer->audioStream()->index)
				{
					m_audioPacketQueue.push(pkt);
				}
			}
			else
			{
				m_bFileEnd = m_pDemuxer->isFileEnd();
			}
			break;
		}
		}
	}
}

void QcMultiMediaPlayerPrivate::videoDecodeThread()
{
	for (;;)
	{
		m_videoThreadState = m_playState;
		if (m_videoThreadState == eExitThread)
			break;

		if (m_videoDecodeEnd)
		{
			::Sleep(10);
			continue;
		}

		switch (m_videoThreadState)
		{
		case eReady:
		case ePause:
		{
			::Sleep(10);
			break;
		}

		case ePlaying:
		{
			int iVideoQueueSize = 0;
			{
				QmStdMutexLocker(m_videoQueue.mutex());
				iVideoQueueSize = m_videoQueue.size();
				if (iVideoQueueSize > 0)
				{
					AVFrameRef frame;
					if (m_pNotify)
					{
                        //¶ªÖ¡
                        while (m_videoQueue.front(frame) && diffToCurrentTime(frame) < 5)
                        {
                            --iVideoQueueSize;
                            m_iVideoCurTime = frame.ptsMsTime();
                            m_videoQueue.pop(frame);
                        }
                        if (frame)
                        {
                            m_pNotify->OnVideoFrame(frame);
                        }
					}
				}
			}
			if (iVideoQueueSize < 3)
			{
				AVPacketPtr pkt;
				if (readPacket(true, pkt) || m_pDemuxer->isFileEnd())
				{
					int iRet = m_pVideoDecoder->decode(pkt.get());
					for (; iRet == FFmpegVideoDecoder::kOk;)
					{
						AVFrameRef frame;
						iRet = m_pVideoDecoder->recv(frame);
						if (iRet == FFmpegVideoDecoder::kOk)
						{
							int mediaTime = toMediaTime(frame->pts, m_pDemuxer->videoStream());
							frame.setPtsMsTime(mediaTime);

							QmStdMutexLocker(m_videoQueue.mutex());
							m_videoQueue.push(frame);
							continue;
						}
						else if (iRet == FFmpegVideoDecoder::kEOF)
						{
							m_videoDecodeEnd = true;
							onNotifyFileEnd();
						}
						break;
					}
				}
			}
			else
			{
				::Sleep(10);
			}
			break;
		}
		}
	}
}

void QcMultiMediaPlayerPrivate::audioDecodeThread()
{
	for (;;)
	{
		m_audioThreadState = m_playState;
		if (m_audioThreadState == eExitThread)
			break;

		if (m_audioDecodeEnd)
		{
			::Sleep(10);
			continue;
		}
		switch (m_audioThreadState)
		{
		case eReady:
		case ePause:
		{
			::Sleep(10);
			break;
		}

		case ePlaying:
		{
			int iQueueSize = 0;
			{
				QmStdMutexLocker(m_audioQueue.mutex());
				iQueueSize = m_audioQueue.size();
				if (iQueueSize > 0)
				{
					AVFrameRef frame;
					if (m_pNotify && m_audioQueue.front(frame) && diffToCurrentTime(frame) < 5)
					{
						if (m_pNotify->OnAudioFrame(frame))
						{
							--iQueueSize;
							m_iAudioCurTime = frame.ptsMsTime();
							m_audioQueue.pop(frame);
						}
					}
				}
			}
			if (iQueueSize < 3)
			{
				AVPacketPtr pkt;
				if (readPacket(false, pkt) || m_pDemuxer->isFileEnd())
				{
					int iRet = m_pAudioDecoder->decode(pkt.get());
					for (; iRet == FFmpegVideoDecoder::kOk;)
					{
						AVFrameRef frame;
						iRet = m_pAudioDecoder->recv(frame);
						if (iRet == FFmpegVideoDecoder::kOk)
						{
							int mediaTime = toMediaTime(frame->pts, m_pDemuxer->audioStream());
							frame.setPtsMsTime(mediaTime);

							QmStdMutexLocker(m_audioQueue.mutex());
							m_audioQueue.push(frame);
							continue;
						}
						else if (iRet == FFmpegVideoDecoder::kEOF)
						{
							m_audioDecodeEnd = true;
							onNotifyFileEnd();
						}
						break;
					}
				}
			}
			else
			{
				::Sleep(10);
			}
			break;
		}
		}
	}
}

void QcMultiMediaPlayerPrivate::onNotifyFileEnd()
{
	if (m_pNotify && isEnd())
	{
		m_pNotify->ToEndSignal();
	}
}
