#pragma once

#include <atlbase.h>

struct ID3D11Device;
struct ID3D11Texture2D;

struct IDXGIOutputDuplication;

using MemFrameCb = std::function<void(int w, int h, int dxgiFormat, uint8_t* data, int stride)>;
using GPUFrameCb = std::function<void(ID3D11Texture2D* frame)>;

class Duplicator
{
public:
	Duplicator();
	~Duplicator();

	bool init(int iMonitor, ID3D11Device* pDevice = nullptr);
	void unInit();

	bool captureToMemFrame(int timeOut, MemFrameCb&& frameCb);
	bool captureToGPUFrame(int timeOut, GPUFrameCb&& frameCb);
protected:
	ID3D11Texture2D* capture(int timeoutInMilliseconds = 0);
protected:
	bool recreateDuplicator();
protected:
	bool m_hasFrame = false;
	CComPtr<ID3D11Texture2D> m_frame;
	CComPtr<ID3D11Texture2D> m_stageFrame;
	CComPtr<ID3D11Device> m_captureDevice;

	CComPtr<ID3D11Device> m_destDevice;

	
	CComPtr<IDXGIOutputDuplication> m_duplicator;
	int m_iMonitor = 0;
};