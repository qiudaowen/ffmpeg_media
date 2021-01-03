#include "Duplicator.h"
#include <dxgi1_2.h>
#include <d3d11.h>
#include "libgraphics/DxgiUtils.h"
#include "libgraphics/D3D11Device.h"


HRESULT InitializeStage(ID3D11Texture2D* pDx11Tex, ID3D11Texture2D** ppStageTex)
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	pDx11Tex->GetDesc(&desc);

	CComPtr<ID3D11Device> device;
	pDx11Tex->GetDevice(&device);

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Width = desc.Width;
	texDesc.Height = desc.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.Format = desc.Format;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.MiscFlags = 0;

	return device->CreateTexture2D(&texDesc, NULL, ppStageTex);
}

Duplicator::Duplicator()
{

}

Duplicator::~Duplicator()
{
	unInit();
}

bool Duplicator::init(int iMonitor)
{
	unInit();
	m_iMonitor = iMonitor;
	return true;
}

void Duplicator::unInit()
{
	if (m_hasFrame)
	{
		m_hasFrame = false;
		m_duplicator->ReleaseFrame();
	}
	m_frame = nullptr;
	m_duplicator = nullptr;
	m_captureDevice = nullptr;
}

bool Duplicator::recreateDuplicator()
{
	if (m_hasFrame)
	{
		m_hasFrame = false;
		m_duplicator->ReleaseFrame();
	}
	m_frame = nullptr;
	m_duplicator = nullptr;

	CComPtr<IDXGIOutputDuplication> dup;
	auto hr = DxgiUtils::createDuplicator(m_iMonitor, m_captureDevice, &dup, NULL, NULL);
	if (hr != S_OK)
		return false;

	m_duplicator = dup;
	return !!m_duplicator;
}

ID3D11Texture2D* Duplicator::capture(ID3D11Device* pDevice)
{
	if (pDevice != m_destDevice)
	{
		unInit();
	}

	if (!m_captureDevice)
	{
		CComPtr<ID3D11Device> device;
		CComPtr<IDXGIOutputDuplication> dup;
		auto hr = DxgiUtils::createDuplicator(m_iMonitor, pDevice, &dup, &device, NULL);
		if (hr != S_OK)
			return nullptr;

		m_duplicator = dup;
		m_captureDevice = device;
		m_destDevice = pDevice;
	}

	if (!m_duplicator)
	{
		if (!recreateDuplicator())
			return nullptr;
	}

	DXGI_OUTDUPL_FRAME_INFO info;
	CComPtr<ID3D11Texture2D> tex;
	CComPtr<IDXGIResource> res;
	HRESULT hr;

	if (m_hasFrame)
	{
		m_hasFrame = false;
		m_duplicator->ReleaseFrame();
	}
	hr = m_duplicator->AcquireNextFrame(0, &info, &res);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		// return m_frame;
		return nullptr;
	}
	else if (FAILED(hr))
	{
		recreateDuplicator();
		return nullptr;
	}
	CComPtr<ID3D11Texture2D> frame;
	hr = res->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&frame);
	if (FAILED(hr)) {
		m_duplicator->ReleaseFrame();
		return nullptr;
	}

	m_frame = frame;
	m_hasFrame = true;
	return m_frame;
}

bool Duplicator::ensureStage(ID3D11Texture2D* frame)
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	frame->GetDesc(&desc);

	D3D11_TEXTURE2D_DESC stageDesc = { 0 };
	if (m_stageFrame)
		m_stageFrame->GetDesc(&stageDesc);

	if (m_stageFrame == nullptr || desc.Width != stageDesc.Width || desc.Height != stageDesc.Height)
	{
		m_stageFrame = nullptr;
		InitializeStage(frame, &m_stageFrame);
	}
	return !!m_stageFrame;
}

bool Duplicator::ensureOutputTexture(ID3D11Device* pDevice, ID3D11Texture2D* frame)
{
	D3D11_TEXTURE2D_DESC desc = { 0 };
	frame->GetDesc(&desc);

	D3D11_TEXTURE2D_DESC outputDesc = { 0 };
	if (m_destFrame)
		m_destFrame->GetDesc(&outputDesc);

	if (m_destFrame == nullptr || desc.Width != outputDesc.Width || desc.Height != outputDesc.Height)
	{
		m_destFrame = nullptr;

		D3D11_TEXTURE2D_DESC texDesc;
		ZeroMemory(&texDesc, sizeof(texDesc));
		texDesc.Width = desc.Width;
		texDesc.Height = desc.Height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DYNAMIC;
		texDesc.Format = desc.Format;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
		pDevice->CreateTexture2D(&texDesc, NULL, &m_destFrame);
	}
	return !!m_destFrame;
}

bool Duplicator::captureToMemFrame(const MemFrameCb& frameCb)
{
	ID3D11Texture2D* frame = capture(NULL);
	if (frame == nullptr)
		return false;

	DXGI_OUTDUPL_DESC outputDesc;
	m_duplicator->GetDesc(&outputDesc);
	if (outputDesc.DesktopImageInSystemMemory)
	{
		DXGI_MAPPED_RECT map;
		auto hr = m_duplicator->MapDesktopSurface(&map);
		if (hr == S_OK)
		{
			frameCb(outputDesc.ModeDesc.Width, outputDesc.ModeDesc.Height, outputDesc.ModeDesc.Format, map.pBits, map.Pitch);
			m_duplicator->UnMapDesktopSurface();
			return true;
		}
		m_duplicator->UnMapDesktopSurface();
	}
	if (!ensureStage(frame))
		return false;

	//TODO: GetFrameDirtyRects GetFrameMoveRects

	CComPtr<ID3D11DeviceContext> context;
	m_captureDevice->GetImmediateContext(&context);
	context->CopyResource(m_stageFrame, frame);

	CComQIPtr<IDXGISurface> spDXGISurface = m_stageFrame;
	if (!spDXGISurface)
		return false;

	DXGI_SURFACE_DESC desc = { 0 };
	spDXGISurface->GetDesc(&desc);

	DXGI_MAPPED_RECT map;
	auto hr = spDXGISurface->Map(&map, DXGI_MAP_READ);
	if (FAILED(hr))
		return false;
	frameCb(desc.Width, desc.Height, desc.Format, map.pBits, map.Pitch);
	spDXGISurface->Unmap();
	return true;
}
bool Duplicator::captureToGPUFrame(ID3D11Device* pOutputDevice, const GPUFrameCb& frameCb)
{
	if (pOutputDevice == nullptr)
		return false;

	ID3D11Texture2D* frame = capture(pOutputDevice);
	if (frame == nullptr)
		return false;

	if (m_captureDevice != pOutputDevice)
	{
		if (!ensureStage(frame))
			return false;

		if (!ensureOutputTexture(pOutputDevice, frame))
			return false;

		D3D11_TEXTURE2D_DESC desc = { 0 };
		frame->GetDesc(&desc);

		//TODO: GetFrameDirtyRects GetFrameMoveRects
		{
			CComPtr<ID3D11DeviceContext> context;
			m_captureDevice->GetImmediateContext(&context);
			context->CopyResource(m_stageFrame, frame);

			CComQIPtr<IDXGISurface> destSurface = m_destFrame;
			if (!destSurface)
				return false;
			CComQIPtr<IDXGISurface> srcSurface = m_stageFrame;
			if (!srcSurface)
				return false;

			DXGI_MAPPED_RECT destMap;
			auto hr = destSurface->Map(&destMap, DXGI_MAP_DISCARD);
			if (FAILED(hr))
				return false;

			DXGI_MAPPED_RECT srcMap;
			hr = srcSurface->Map(&srcMap, DXGI_MAP_DISCARD);
			if (FAILED(hr))
			{
				destSurface->Unmap();
				return false;
			}
			if (destMap.Pitch == srcMap.Pitch) {
				memcpy(destMap.pBits, srcMap.pBits, srcMap.Pitch * desc.Height);
			} else {
				int row_copy = (srcMap.Pitch < destMap.Pitch) ? srcMap.Pitch : destMap.Pitch;
				for (int y = 0; y < (int)desc.Height; ++y){
					memcpy(destMap.pBits + (uint32_t)y * destMap.Pitch, srcMap.pBits + (uint32_t)y * srcMap.Pitch, row_copy);
				}	
			}
			srcSurface->Unmap();
			destSurface->Unmap();
		}
		frameCb(m_destFrame);
	}
	else
	{
		frameCb(frame);
	}
	return true;
}

