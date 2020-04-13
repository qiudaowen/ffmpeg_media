#include "VideoPlayerMode.h"
#include "libmedia/QcMultiMediaPlayer.h"
#include "libmedia/FFmpegUtils.h"
#include "libmedia/FFmpegVideoTransformat.h"
#include "libmedia/QcAudioTransformat.h"
#include "libmedia/QcAudioPlayer.h"
#include "libmedia/AVFrameRef.h"
#include "utils/libstring.h"
#include "win/MsgWnd.h"

VideoPlayerMode::VideoPlayerMode()
{
	m_player = std::make_unique<QcMultiMediaPlayer>(this);
	m_transFormat = std::make_unique<FFmpegVideoTransformat>();
	m_audioPlayer = std::make_unique<QcAudioPlayer>();
	m_audioTrans = std::make_unique<QcAudioTransformat>();
}

VideoPlayerMode::~VideoPlayerMode()
{
    m_player = nullptr;
    m_audioPlayer = nullptr;
    m_transFormat = nullptr;
    m_audioTrans = nullptr;
}

void VideoPlayerMode::init(HWND hWnd)
{
	m_hWnd = hWnd;

	RECT rc;
	GetClientRect(hWnd, &rc);
	m_memorySurface.resize(rc.right - rc.left, rc.bottom - rc.top);
}

bool VideoPlayerMode::open(const std::wstring& fileName)
{
	bool bRet = m_player->open(libstring::toUtf8(fileName).c_str());
	if (!bRet)
		return false;

	const QsMediaInfo& mediaInfo = m_player->getMediaInfo();
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


void VideoPlayerMode::openNext()
{
    open();
}

bool VideoPlayerMode::OnVideoFrame(const AVFrameRef& frame)
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

bool VideoPlayerMode::OnAudioFrame(const AVFrameRef& frame)
{
	AVFrameRef outFrame;
	m_audioTrans->transformat(frame.data(), frame.sampleCount(), outFrame);
	m_audioPlayer->playAudio(outFrame.data(0), outFrame.sampleCount());
	return true;
}

void VideoPlayerMode::ToEndSignal()
{
    std::weak_ptr<VideoPlayerMode> weakThis = std::shared_from_this();
    MsgWnd::mainMsgWnd()->post([weakThis]() {
        auto pThis = weakThis.lock();
        if (pThis)
            pThis->openNext();
    });
}
