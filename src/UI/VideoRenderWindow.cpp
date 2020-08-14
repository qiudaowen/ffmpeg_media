#include "VideoRenderWindow.h"
#include "libgraphics/D3D11Device.h"
#include "libgraphics/D3D11Texture.h"
#include "QmMacro.h"
#include "win/MsgWnd.h"
#include "libmedia/FFmpegVideoTransformat.h"
#include "libmedia/FFmpegUtils.h"
#include "utils/LogTimeElapsed.h"
#include <d3d11.h>


#define DxRender 1


IMPLEMENT_DYNAMIC(VideoRenderWindow, CWnd)
BEGIN_MESSAGE_MAP(VideoRenderWindow, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()

VideoRenderWindow::VideoRenderWindow()
{
#if DxRender
	m_d3d11Device = std::make_shared<D3D11Device>();
	m_d3d11Device->create(0);
	m_videoTex = std::make_shared<D3D11Texture>(m_d3d11Device->device());
#else
	m_memorySurface = std::make_shared<QcDIBSection>();
#endif

	m_transFormat = std::make_unique<FFmpegVideoTransformat>();
}

VideoRenderWindow::~VideoRenderWindow()
{

}

void VideoRenderWindow::init(int x, int y, int w, int h, HWND hParent)
{
	static const wchar_t* className = []() {
		static const wchar_t* className = L"VideoWindow";
		WNDCLASSEXW wcx = {0};
		wcx.cbSize = sizeof(wcx);
		wcx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcx.lpfnWndProc = ::DefWindowProcW;
		wcx.hCursor = LoadCursor(NULL,IDC_ARROW);
		wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		wcx.lpszClassName = className;

		// Register the window class. 
		return RegisterClassEx(&wcx) != FALSE ? className : nullptr;
	}();
	CreateEx(0, className, L"VideoRenderWindow", hParent ? WS_CHILD : WS_POPUP , { x, y, w, y }, CWnd::FromHandle(hParent), 0);

	if (m_d3d11Device)
		m_d3d11Device->createSwapChain(m_hWnd, w, h);
}


ID3D11Device* VideoRenderWindow::device() const
{
	return m_d3d11Device ? m_d3d11Device->device() : NULL;
}

void VideoRenderWindow::OnPaint()
{
	CWnd::OnPaint();

	onRender();
}
void VideoRenderWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	if (m_memorySurface)
		m_memorySurface->resize(cx, cy);
	if (m_d3d11Device)
		m_d3d11Device->resize(cx, cy);
}

void VideoRenderWindow::onRender()
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	int w = rc.right - rc.left;
	int h = rc.bottom - rc.top;

	if (m_memorySurface)
	{
		m_memorySurface->resize(w, h);
		AVFrameRef frame;
		{
			QmStdMutexLocker(m_lastFrameMutex);
			frame = m_lastFrame;
		}
		frame = AVFrameRef::fromHWFrame(frame);
		m_transFormat->transformat(frame.width(), frame.height(), frame.format(), frame.data(), frame.linesize()
			, w, h, FFmpegUtils::fourccToFFmpegFormat(m_memorySurface->format()), m_memorySurface->datas(), m_memorySurface->lineSizes());

		HDC hDC = ::GetDC(m_hWnd);
		::BitBlt(hDC, 0, 0, w, h, m_memorySurface->dcHandle(), 0, 0, SRCCOPY);
		::ReleaseDC(m_hWnd, hDC);
	}
	if (m_d3d11Device)
	{
		AVFrameRef frame;
		{
			QmStdMutexLocker(m_lastFrameMutex);
			frame = m_lastFrame;
		}
		m_d3d11Device->begin();


		//LogTimeElapsed logRender(L"renderFrameD3d11");
		do 
		{
			if (frame.isHWFormat())
			{
				ID3D11Texture2D* tex2D = (ID3D11Texture2D*)frame.data(0);
				int subResouce = (int)frame.data(1);
				int frameW = frame.width();
				int frameH = frame.height();

 				if (m_d3d11Device->drawTexture(tex2D, subResouce, { 0, 0, w, h }))
 				{
 					break;
 				}
				if (m_videoTex->updateFromTexArray(tex2D, subResouce))
				{
					m_d3d11Device->drawTexture(m_videoTex.get(), { 0, 0, w, h });
					break;
				}
				frame = AVFrameRef::fromHWFrame(frame);
			}

			//update load to gpu
			int iVideoFormat = FFmpegUtils::ffmpegFormatToFourcc(frame.format());
			switch (iVideoFormat)
			{
  			case FOURCC_I420:
  			{
  				m_videoTex->updateYUV(frame.data(), frame.linesize(), frame.width(), frame.height());
  				break;
  			}
			case FOURCC_NV12:
			{
				m_videoTex->updateNV12(frame.data(), frame.linesize(), frame.width(), frame.height());
				break;
			}
			case FOURCC_BGRA:
			case FOURCC_RGBA:
			{
				m_videoTex->updateRGB32(frame.data(0), frame.linesize(0), frame.width(), frame.height());
				break;
			}
			default:
			{
				m_videoTex->lockRGB32(frame.width(), frame.height(), 0, [&](uint8_t* dstData, int dataSlice) {
					uint8_t * const dstDatas[] = { dstData };
					int dstSlice[] = { dataSlice };

					m_transFormat->transformat(frame.width(), frame.height(), frame.format(), frame.data(), frame.linesize()
						, frame.width(), frame.height(), FFmpegUtils::fourccToFFmpegFormat(FOURCC_BGRA), dstDatas, dstSlice);
				});
				break;
			}
			}
			m_d3d11Device->drawTexture(m_videoTex.get(), { 0, 0, w, h });
		} while (0);

		m_d3d11Device->present();
	}
}

bool VideoRenderWindow::OnVideoFrame(const AVFrameRef& frame)
{
	{
		QmStdMutexLocker(m_lastFrameMutex);
		m_lastFrame = frame;
	}
	if (!m_bUpdating)
	{
		m_bUpdating = true;
		std::weak_ptr<VideoRenderWindow> weakThis = shared_from_this();
#if 0
		auto pThis = weakThis.lock();
			if (pThis)
			{
				pThis->m_bUpdating = false;
				pThis->onRender();
			}
#else
		MsgWnd::mainMsgWnd()->post([weakThis]() {
			auto pThis = weakThis.lock();
			if (pThis)
			{
				pThis->m_bUpdating = false;
				pThis->onRender();
			}
		});
#endif
	}
	return true;
}
