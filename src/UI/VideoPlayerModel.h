#pragma once

#include <memory>
#include <string>
#include <vector>
#include "libmedia/QcMultiMediaPlayer.h"
#include "QcDIBSection.h"

class QcMultiMediaPlayer;
class FFmpegVideoTransformat;
class QcAudioPlayer;
class QcAudioTransformat;
class VideoPlayerModel : public IMultiMediaNotify, public std::enable_shared_from_this<VideoPlayerModel>
{
public:
	VideoPlayerModel();
	~VideoPlayerModel();

	void init(HWND hWnd);
	bool open(const std::wstring& fileName);
	void close();

	void trigger();
	void setVolume(double fPos);
	void setProgress(double fPos);
	double getProgress();

	void addVideoFileList(const std::vector<std::wstring>& fileList);
	void removeVideoFileList(const std::vector<std::wstring>& fileList);
protected:
    void openNext();
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

	std::vector<std::wstring> m_fileList;
};