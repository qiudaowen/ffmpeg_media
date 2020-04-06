#include "QcMultiMediaPlayer.h"
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

QcMultiMediaPlayer::QcMultiMediaPlayer(IMultiMediaNotify* pNotify)
    : m_pNotify(pNotify)

{

}

QcMultiMediaPlayer::~QcMultiMediaPlayer()
{
    Close();
}

bool QcMultiMediaPlayer::Open(const char* pFile)
{
    Close();

	m_pDemuxer = std::make_unique<FFmpegDemuxer>();
	bool bOk = m_pDemuxer->open(pFile);
	if (bOk)
	{
		AVStream* pVideoStream = m_pDemuxer->videoStream();
		if (pVideoStream)
		{
			m_pVideoDecoder = std::make_unique<FFmpegVideoDecoder>(pVideoStream->codecpar);
		}
		AVStream* pAudioStream = m_pDemuxer->audioStream();
		if (pAudioStream)
		{
			m_pAudioDecoder = std::make_unique<FFmpegAudioDecoder>(pAudioStream->codecpar);
		}
	}
	_start();
    return true;
}

bool QcMultiMediaPlayer::Close()
{
	SynState(eExitThread);
	if (m_videoThread.joinable())
		m_videoThread.join();
	if (m_audioThread.joinable())
		m_audioThread.join();
	if (m_demuxerThread.joinable())
		m_demuxerThread.join();

	m_pVideoDecoder = nullptr;
	m_pAudioDecoder = nullptr;
	m_pDemuxer = 0;

	m_bFileEnd = false;
	m_videoDecodeEnd = false;
	m_audioDecodeEnd = false;
    return true;
}

bool QcMultiMediaPlayer::isEnd() const
{
	bool bEnd = (m_videoDecodeEnd || !HasVideo()) &&
		(m_audioDecodeEnd || !HasAudio()) && m_bFileEnd;
	return bEnd;
}

bool QcMultiMediaPlayer::readFrame(bool bVideo, AVFrameRef& frame)
{
	if (bVideo)
	{
		std::lock_guard<std::mutex> QmUniqueVarName(m_videoQueue.mutex());
		//QmStdMutexLocker(m_videoQueue.mutex());
		return m_videoQueue.pop(frame);
	}
		
	QmStdMutexLocker(m_audioQueue.mutex());
	return m_audioQueue.pop(frame);
}

const QsMediaInfo& QcMultiMediaPlayer::getMediaInfo() const
{
	return m_pDemuxer->getMediaInfo();
}

int QcMultiMediaPlayer::GetTotalTime() const
{
	return m_pDemuxer ? m_pDemuxer->getMediaInfo().iFileTotalTime : 0;
}

void QcMultiMediaPlayer::Play()
{
	if (m_playState != ePlaying)
	{
		SynState(ePlaying);
		m_iBeginSystemTime = (int)FFmpegUtils::currentMilliSecsSinceEpoch() - GetCurTime();
	}	
}

void QcMultiMediaPlayer::Pause()
{
	if (m_playState != ePlaying)
	{
		SynState(ePause);
	}
}

void QcMultiMediaPlayer::Seek(int msTime)
{
	int lastState = m_playState;
	if (m_playState == ePlaying)
		SynState(ePause);

	m_pDemuxer->seek(msTime);
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
		SynState(ePlaying);
}


void QcMultiMediaPlayer::SynState(int eState)
{
	m_playState = eState;
	while ((HasVideo() && m_playState != m_videoThreadState)
		|| (HasAudio() && m_playState != m_audioThreadState)
		|| (m_demuxerThread.joinable() && m_playState != m_demuxerThreadState))
		SwitchToThread();
}

void QcMultiMediaPlayer::_start()
{
	m_playState = eReady;

	m_demuxerThread = std::thread([this] { demuxeThread(); });
	m_videoThread = std::thread([this] {videoDecodeThread(); });
	m_audioThread = std::thread([this] {audioDecodeThread(); });
}

bool QcMultiMediaPlayer::isPacketQueueFull()
{
	std::lock_guard<std::mutex> lck(m_demuxerMutex);
	return m_videoPacketQueue.packetSize() + m_audioPacketQueue.packetSize() > 15 * 1024 * 1024;
}

int QcMultiMediaPlayer::readPacket(bool bVideo, AVPacketPtr& ptr)
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

int QcMultiMediaPlayer::toSystemTime(int64_t pts, AVStream* pStream)
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
	return m_iBeginSystemTime + iTime;
}

int QcMultiMediaPlayer::diffToCurrentTime(const AVFrameRef& frame)
{
	int iDiff = (int)frame.ptsSystemTime() - FFmpegUtils::currentMilliSecsSinceEpoch();
	return iDiff;
}

void QcMultiMediaPlayer::demuxeThread()
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

void QcMultiMediaPlayer::videoDecodeThread()
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
					if (m_pNotify && m_videoQueue.front(frame) && diffToCurrentTime(frame) < 5)
					{
						if (m_pNotify->OnVideoFrame(frame))
						{
							--iVideoQueueSize;
							m_videoQueue.pop(frame);
						}
					}
				}
			}
			if (iVideoQueueSize < 3)
			{
				AVPacketPtr pkt;
				if (readPacket(true, pkt) || m_pDemuxer->isFileEnd())
				{
					int iRet = m_pVideoDecoder->Decode(pkt.get());
					for (; iRet == FFmpegVideoDecoder::kOk;)
					{
						AVFrameRef frame;
						iRet = m_pVideoDecoder->recv(frame);
						if (iRet == FFmpegVideoDecoder::kOk)
						{
							int playSysTime = toSystemTime(frame->pts, m_pDemuxer->videoStream());
							int mediaTime = playSysTime - m_iBeginSystemTime;
							frame.setPtsSystemTime(playSysTime);

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

void QcMultiMediaPlayer::audioDecodeThread()
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
					int iRet = m_pAudioDecoder->Decode(pkt.get());
					for (; iRet == FFmpegVideoDecoder::kOk;)
					{
						AVFrameRef frame;
						iRet = m_pAudioDecoder->recv(frame);
						if (iRet == FFmpegVideoDecoder::kOk)
						{
							int playSysTime = toSystemTime(frame->pts, m_pDemuxer->audioStream());
							int mediaTime = playSysTime - m_iBeginSystemTime;
							frame.setPtsSystemTime(playSysTime);

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

void QcMultiMediaPlayer::onNotifyFileEnd()
{
	if (m_pNotify && isEnd())
	{
		m_pNotify->ToEndSignal();
	}
}
