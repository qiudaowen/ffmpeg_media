#include "VideoPlayerModel.h"
#include "libmedia/QcMultiMediaPlayer.h"
#include "libmedia/FFmpegUtils.h"
#include "libmedia/FFmpegVideoTransformat.h"
#include "libmedia/QcAudioTransformat.h"
#include "libmedia/QcAudioPlayer.h"
#include "libmedia/AVFrameRef.h"
#include "utils/libstring.h"
#include "win/MsgWnd.h"

VideoPlayerModel::VideoPlayerModel()
{
	m_player = std::make_unique<QcMultiMediaPlayer>(this);
	m_transFormat = std::make_unique<FFmpegVideoTransformat>();
	m_audioPlayer = std::make_unique<QcAudioPlayer>();
	m_audioTrans = std::make_unique<QcAudioTransformat>();
}

VideoPlayerModel::~VideoPlayerModel()
{
    m_player = nullptr;
    m_audioPlayer = nullptr;
    m_transFormat = nullptr;
    m_audioTrans = nullptr;
}

void VideoPlayerModel::init(HWND hWnd)
{
	m_hWnd = hWnd;

	RECT rc;
	GetClientRect(hWnd, &rc);
	m_memorySurface.resize(rc.right - rc.left, rc.bottom - rc.top);
}

bool VideoPlayerModel::open(const std::wstring& fileName)
{
	bool bRet = m_player->open(libstring::toUtf8(fileName).c_str());
	if (!bRet)
		return false;

	const QsMediaInfo& mediaInfo = *(m_player->getMediaInfo());
	QsAudioPara audioPara;
	audioPara.iSamplingFreq = mediaInfo.sampleRate;
	audioPara.eSample_fmt = FFmpegUtils::FromFFmpegAudioFormat(mediaInfo.audioFormat);
	audioPara.nChannel = mediaInfo.nChannels;
	QsAudioPara bestAudioPara;
	bRet = m_audioPlayer->open(nullptr, audioPara, &bestAudioPara);
	if (!bRet)
		return false;

	bRet = m_audioTrans->init(audioPara, bestAudioPara);
	if (!bRet)
		return false;

	m_audioPlayer->start();

	m_player->play();

	m_player->seek(mediaInfo.iFileTotalTime * 0.5);
	return true;
}


void VideoPlayerModel::close()
{
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

void VideoPlayerModel::addVideoFileList(const std::vector<std::wstring>& fileList)
{
	m_fileList.insert(m_fileList.end(), fileList.begin(), fileList.end());
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

void VideoPlayerModel::openNext()
{
    //open();
}

bool VideoPlayerModel::OnVideoFrame(const AVFrameRef& frame)
{
	RECT rc;
	GetClientRect(m_hWnd, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	m_memorySurface.resize(w, h);

	m_transFormat->transformat(frame.width(), frame.height(), frame.format(), frame.data(), frame.linesize()
		, w, h, FFmpegUtils::fourccToFFmpegFormat(m_memorySurface.format()), m_memorySurface.datas(), m_memorySurface.lineSizes());

	HDC hDC = GetDC(m_hWnd);
	::BitBlt(hDC, 0, 0, w, h, m_memorySurface.dcHandle(), 0, 0, SRCCOPY);
	ReleaseDC(m_hWnd, hDC);
	return true;
}

bool VideoPlayerModel::OnAudioFrame(const AVFrameRef& frame)
{
	AVFrameRef outFrame;
	m_audioTrans->transformat(frame.data(), frame.sampleCount(), outFrame);
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
