#include "VideoPlayerMode.h"
#include "libmedia/QcMultiMediaPlayer.h"
#include "libmedia/FFmpegUtils.h"
#include "utils/libstring.h"

VideoPlayerMode::VideoPlayerMode()
{
	m_player = std::make_unique<QcMultiMediaPlayer>(this);
}

VideoPlayerMode::~VideoPlayerMode()
{

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
	bool bRet = m_player->Open(libstring::toUtf8(fileName).c_str());
	if (!bRet)
		return false;
	m_player->Play();
	return true;
}

bool VideoPlayerMode::OnVideoFrame(const AVFrameRef& frame)
{
	RECT rc;
	GetClientRect(m_hWnd, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;
	m_memorySurface.resize(w, h);

	m_transFormat.Transformat(frame.width(), frame.height(), frame.format(), frame.data(), frame.linesize()
		, w, h, FFmpegUtils::fourccToFFmpegFormat(m_memorySurface.format()), m_memorySurface.datas(), m_memorySurface.lineSizes());

	HDC hDC = GetDC(m_hWnd);
	::BitBlt(hDC, 0, 0, w, h, m_memorySurface.dcHandle(), 0, 0, SRCCOPY);
	ReleaseDC(m_hWnd, hDC);
	return true;
}

bool VideoPlayerMode::OnAudioFrame(const AVFrameRef& frame)
{
	return true;
}

void VideoPlayerMode::ToEndSignal()
{

}
