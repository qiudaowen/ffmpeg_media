#pragma once

#include <memory>
#include "QcComInit.h"

class VideoPlayerModel;
class VideoRenderWindow;
class FFmpegHwDevice;

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

	void init();
	void unInit();
protected:
	std::shared_ptr<FFmpegHwDevice> m_hwDevice;
	std::shared_ptr<VideoPlayerModel> m_playerModel;
	std::shared_ptr<VideoRenderWindow> m_renderWindow;
};

#define QmVideoApp VideoPlayerApp::instance()
#define QmVideoPlayerModel VideoPlayerApp::instance()->playerModel()
#define QmVideoRenderWindow VideoPlayerApp::instance()->renderWindow()