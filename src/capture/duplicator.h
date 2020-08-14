#pragma once

#include "capture_global.h"
#include <atlbase.h>
#include <functional>

struct ID3D11Device;
struct ID3D11Texture2D;

struct IDXGIOutputDuplication;

using MemFrameCb = std::function<void(int w, int h, int dxgiFormat, uint8_t* data, int stride)>;
using GPUFrameCb = std::function<void(ID3D11Texture2D* frame)>;

class CAPTURE_API Duplicator
{
public:
	Duplicator();
	~Duplicator();

	bool init(int iMonitor);
	void unInit();

	bool captureToMemFrame(MemFrameCb&& frameCb);
	bool captureToGPUFrame(ID3D11Device* pDevice, GPUFrameCb&& frameCb);
protected:
	ID3D11Texture2D* capture(ID3D11Device* pDevice);
	bool recreateDuplicator();
	bool ensureStage(ID3D11Texture2D* pDx11Tex);
	bool ensureOutputTexture(ID3D11Device* pDevice, ID3D11Texture2D* pDx11Tex);
protected:
	bool m_hasFrame = false;
	CComPtr<ID3D11Texture2D> m_frame;
	CComPtr<ID3D11Texture2D> m_stageFrame;
	CComPtr<ID3D11Device> m_captureDevice;

	CComPtr<ID3D11Texture2D> m_destFrame;
	CComPtr<ID3D11Device> m_destDevice;

	
	bool m_bInSystemMemory = false;
	CComPtr<IDXGIOutputDuplication> m_duplicator;
	int m_iMonitor = 0;
};