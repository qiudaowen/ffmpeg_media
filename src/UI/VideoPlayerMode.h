#pragma once

#include <memory>
#include "libmedia/QcMultiMediaPlayer.h"
#include "libmedia/FFmpegVideoTransformat.h"
#include "QcDIBSection.h"

class QcMultiMediaPlayer;
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
	FFmpegVideoTransformat m_transFormat;
};