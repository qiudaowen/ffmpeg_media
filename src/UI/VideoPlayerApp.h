#pragma once

#include <memory>
#include "QcComInit.h"

class VideoPlayerModel;
class VideoRenderWindow;
class FFmpegHwDevice;
class CaptureModel;
class RecordModel;
class AudioMuxerModel;
class VideoPlayerApp
{
	VideoPlayerApp();
	~VideoPlayerApp();
public:
	static VideoPlayerApp* instance();

	VideoPlayerModel* playerModel() const {
		return m_playerModel.get();
	}
	VideoRenderWindow* renderWindow() const {
		return m_renderWindow.get();
	}
	CaptureModel* captureModel() const {
		return m_captureModel.get();
	}
	RecordModel* recordModel() const {
		return m_recordModel.get();
	}
	AudioMuxerModel* audioMuxerModel() const {
		return m_audioMuxerModel.get();
	}

	void init();
	void unInit();
protected:
	std::shared_ptr<FFmpegHwDevice> m_hwDevice;
	std::shared_ptr<VideoPlayerModel> m_playerModel;
	std::shared_ptr<CaptureModel> m_captureModel;
	std::shared_ptr<RecordModel> m_recordModel;
	std::shared_ptr<AudioMuxerModel> m_audioMuxerModel;
	std::shared_ptr<VideoRenderWindow> m_renderWindow;
};

#define QmVideoApp VideoPlayerApp::instance()
#define QmVideoPlayerModel VideoPlayerApp::instance()->playerModel()
#define QmCaptureModel VideoPlayerApp::instance()->captureModel()
#define QmRecordModel VideoPlayerApp::instance()->recordModel()
#define QmAudioMuxerModel VideoPlayerApp::instance()->audioMuxerModel()
#define QmVideoRenderWindow VideoPlayerApp::instance()->renderWindow()