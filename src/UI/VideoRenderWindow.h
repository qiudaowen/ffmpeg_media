#pragma once

#include <memory>
#include <mutex>
#include "QcDIBSection.h"
#include "VideoPlayerModel.h"
#include "libmedia/AVFrameRef.h"

class D3D11Device;
class D3D11Texture;
struct ID3D11Texture2D;
class VideoRenderWindow : public CWnd, public VideoFrameNotify, public std::enable_shared_from_this<VideoRenderWindow>
{
public:
	DECLARE_DYNAMIC(VideoRenderWindow)
	VideoRenderWindow();
	~VideoRenderWindow();

	void init(int x, int y, int w, int h, HWND hParent);

	ID3D11Device* device() const;

	bool drawTexture(ID3D11Texture2D* texture, int subResouce = 0);
	void drawFrame(const uint8_t* const datas[], const int dataSlice[], int w, int h, int forccFormat);
	void beginRender();
	void endRender();

	void postFrame(const AVFrameRef& frame);
protected:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

protected:
	void onRender();
	bool onVideoFrame(const AVFrameRef& frame) override;
protected:
	AVFrameRef m_lastFrame;
	std::mutex m_lastFrameMutex;
	bool m_bUpdating = false;

	std::unique_ptr<FFmpegVideoTransformat> m_transFormat;
	std::shared_ptr<QcDIBSection> m_memorySurface;

	std::shared_ptr<D3D11Device> m_d3d11Device;
	std::shared_ptr<D3D11Texture> m_videoTex;

	int m_iRenderRecurive = 0;
};
