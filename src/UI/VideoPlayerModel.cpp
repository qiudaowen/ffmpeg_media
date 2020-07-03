#include "VideoPlayerModel.h"
#include "libmedia/QcMultiMediaPlayer.h"
#include "libmedia/FFmpegUtils.h"
#include "libmedia/QcAudioTransformat.h"
#include "libmedia/QcAudioPlayer.h"
#include "libmedia/AVFrameRef.h"
#include "libmedia/FFmpegHwDevice.h"
#include "utils/libstring.h"
#include "win/MsgWnd.h"

VideoPlayerModel::VideoPlayerModel()
{
	m_player = std::make_unique<QcMultiMediaPlayer>(this);
	m_audioPlayer = std::make_unique<QcAudioPlayer>();
	m_audioTransForPlayer = std::make_unique<QcAudioTransformat>();
	m_hwDevice = std::make_unique<FFmpegHwDevice>();
}

VideoPlayerModel::~VideoPlayerModel()
{
    m_player = nullptr;
    m_audioPlayer = nullptr;
    m_audioTransForPlayer = nullptr;
}

void VideoPlayerModel::init(std::weak_ptr<VideoFrameNotify>&& notify, ID3D11Device* pHwDecodeDevcie)
{
	m_videoNotify = std::move(notify);
	m_hwDevice->attach(pHwDecodeDevcie);
	m_player->setHwDevice(m_hwDevice->hwDevice().get());
}

bool VideoPlayerModel::open(const std::wstring& fileName)
{
	close();

	m_currentPlayFile = fileName;
	bool bRet = m_player->open(libstring::toUtf8(fileName).c_str());
	if (!bRet)
		return false;

	const QsMediaInfo& mediaInfo = *(m_player->getMediaInfo());
	QsAudioPara audioPara;
	audioPara.sampleRate = mediaInfo.sampleRate;
	audioPara.sampleFormat = FFmpegUtils::FromFFmpegAudioFormat(mediaInfo.audioFormat);
	audioPara.nChannels = mediaInfo.nChannels;

	QsAudioPara bestAudioPara;
	bRet = m_audioPlayer->open(nullptr, nullptr, &bestAudioPara);
	if (!bRet)
		return false;
	bRet = m_audioTransForPlayer->init(audioPara, bestAudioPara);
	if (!bRet)
		return false;
	m_audioPlayer->start();

	m_player->play();
	return true;
}


int VideoPlayerModel::getCurTime() const
{
	return m_player ? m_player->getCurTime() : 0;
}

int VideoPlayerModel::getTotalTime() const
{
	return m_player ? m_player->getTotalTime() : 0;
}

void VideoPlayerModel::close()
{
	if (m_player)
		m_player->close();
}

void VideoPlayerModel::trigger()
{
	if (m_player->isPlaying())
		m_player->pause();
	else
		m_player->play();
}

void VideoPlayerModel::setVolume(double fPos)
{
	m_audioPlayer->setVolume(fPos);
}

void VideoPlayerModel::setProgress(double fPos)
{
	m_player->seek(m_player->getTotalTime() * fPos);
}

double VideoPlayerModel::getProgress()
{
	return m_player->getCurTime() / (double)m_player->getTotalTime();
}

void VideoPlayerModel::addVideoFileList(const std::vector<std::wstring>& fileList)
{
	m_fileList.insert(m_fileList.end(), fileList.begin(), fileList.end());
	if (m_fileList.size() == fileList.size())
		openNext();
}

void VideoPlayerModel::removeVideoFileList(const std::vector<std::wstring>& fileList)
{
	for (const auto& file : fileList)
	{
		auto iter = std::find(m_fileList.begin(), m_fileList.end(), file);
		if (iter != m_fileList.end())
			m_fileList.erase(iter);
	}
}

const std::vector<std::wstring>& VideoPlayerModel::fileList() const
{
	return m_fileList;
}

void VideoPlayerModel::openNext()
{
    //open();
	if (m_fileList.size())
	{
		for (int i=0; i<(int)m_fileList.size(); ++i)
		{
			auto iter = std::find(m_fileList.begin(), m_fileList.end(), m_currentPlayFile);
			if (iter == m_fileList.end() || ++iter == m_fileList.end())
			{
				if (open(m_fileList.front()))
					break;
			}
			else
			{
				if (open(*iter))
					break;
			}
		}
	}
}

bool VideoPlayerModel::OnVideoFrame(const AVFrameRef& frame)
{
	auto videoNotify = m_videoNotify.lock();
	if (videoNotify)
		videoNotify->OnVideoFrame(frame);
	return true;
}

bool VideoPlayerModel::OnAudioFrame(const AVFrameRef& frame)
{
	AVFrameRef outFrame;
	m_audioTransForPlayer->transformat(frame.data(), frame.sampleCount(), outFrame);
	m_audioPlayer->playAudio(outFrame.data(0), outFrame.sampleCount());
	return true;
}

void VideoPlayerModel::ToEndSignal()
{
    std::weak_ptr<VideoPlayerModel> weakThis = shared_from_this();
    MsgWnd::mainMsgWnd()->post([weakThis]() {
        auto pThis = weakThis.lock();
        if (pThis)
            pThis->openNext();
    });
}
