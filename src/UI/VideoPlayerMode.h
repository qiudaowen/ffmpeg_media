#pragma once

#include <memory>
#include "libmedia/QcMultiMediaPlayer.h"
#include "QcDIBSection.h"

class QcMultiMediaPlayer;
class FFmpegVideoTransformat;
class QcAudioPlayer;
class QcAudioTransformat;
class VideoPlayerMode : public IMultiMediaNotify
{
public:
	VideoPlayerMode();
	~VideoPlayerMode();

	void init(HWND hWnd);
	bool open(const std::wstring& fileName);
protected:
	virtual bool OnVideoFrame(const AVFrameRef& frame);
	virtual bool OnAudioFrame(const AVFrameRef& frame);
	virtual void ToEndSignal();
protected:
	HWND m_hWnd;
	QcDIBSection m_memorySurface;
	std::unique_ptr<QcMultiMediaPlayer> m_player;
	std::unique_ptr<FFmpegVideoTransformat> m_transFormat;
	std::unique_ptr<QcAudioPlayer> m_audioPlayer;
	std::unique_ptr<QcAudioTransformat> m_audioTrans;
};