#include "VideoPlayerApp.h"
#include "VideoPlayerModel.h"
#include "VideoRenderWindow.h"
#include "libmedia/FFmpegHwDevice.h"

VideoPlayerApp::VideoPlayerApp()
{
	m_renderWindow = std::make_shared<VideoRenderWindow>();
	m_playerModel = std::make_shared<VideoPlayerModel>();
}

VideoPlayerApp::~VideoPlayerApp()
{
	unInit();
}

VideoPlayerApp* VideoPlayerApp::instance()
{
	static VideoPlayerApp gInstance;
	return &gInstance;
}

void VideoPlayerApp::init()
{
	m_playerModel = std::make_shared<VideoPlayerModel>();
	m_playerModel->init(m_renderWindow, m_renderWindow->device());
}

void VideoPlayerApp::unInit()
{

}

