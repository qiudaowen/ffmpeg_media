#pragma once

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "libmedia/QcMultiMediaPlayer.h"
#include "libmedia/AVFrameRef.h"

class QcMultiMediaPlayer;
class FFmpegVideoTransformat;
class QcAudioPlayer;
class QcAudioTransformat;
class FFmpegHwDevice;
class D3D11Device;
struct ID3D11Device;

struct VideoFrameNotify
{
	virtual bool OnVideoFrame(const AVFrameRef& frame) = 0;
};

class VideoPlayerModel : public IMultiMediaNotify, public std::enable_shared_from_this<VideoPlayerModel>
{
public:
	VideoPlayerModel();
	~VideoPlayerModel();

	void init(std::weak_ptr<VideoFrameNotify>&& notify, ID3D11Device* pHwDecodeDevcie = nullptr);
	bool open(const std::wstring& fileName);
	int getCurTime() const;
	int getTotalTime() const;
	void close();

	void trigger();
	void setVolume(double fPos);
	void setProgress(double fPos);
	double getProgress();

	void addVideoFileList(const std::vector<std::wstring>& fileList);
	void removeVideoFileList(const std::vector<std::wstring>& fileList);
	const std::vector<std::wstring>& fileList() const;
protected:
    void openNext();
protected:
	virtual bool OnVideoFrame(const AVFrameRef& frame);
	virtual bool OnAudioFrame(const AVFrameRef& frame);
	virtual void ToEndSignal();
protected:
	std::weak_ptr<VideoFrameNotify> m_videoNotify;

	std::unique_ptr<FFmpegHwDevice> m_hwDevice;
	std::unique_ptr<QcMultiMediaPlayer> m_player;
	std::unique_ptr<QcAudioPlayer> m_audioPlayer;
	std::unique_ptr<QcAudioTransformat> m_audioTransForPlayer;

	std::vector<std::wstring> m_fileList;
	std::wstring m_currentPlayFile;
};