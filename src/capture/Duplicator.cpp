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

bool Duplicator::init(int iMonitor, ID3D11Device* pDevice)
{
	unInit();

	CComPtr<D3D11Device> device;
	CComPtr<IDXGIOutputDuplication> dup;
	auto hr = DxgiUtils::createDuplicator(iMonitor, pDevice, &dup, &device, NULL);
	if (hr != S_OK)
		return false;

	m_duplicator = dup;
	m_captureDevice = device;
	m_iMonitor = iMonitor;

	m_destDevice = pDevice;

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
	return m_duplicator;
}

ID3D11Texture2D* Duplicator::capture(int timeoutInMilliseconds)
{
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
	hr = m_duplicator->AcquireNextFrame(timeoutInMilliseconds, &info, &res);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		return m_frame;
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

bool Duplicator::captureToMemFrame(int timeoutInMilliseconds, MemFrameCb&& frameCb)
{
	ID3D11Texture2D* frame = capture(timeoutInMilliseconds);
	if (frame == nullptr)
		return false;

	if (m_stageFrame == nullptr)
	{
		auto hr = InitializeStage(frame, &m_stageFrame);
		if (FAILED(hr))
			return false;
	}
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
		return;
	frameCb(desc.Width, desc.Height, desc.Format, map.pBits, map.Pitch);
	spDXGISurface->Unmap();
}
bool Duplicator::captureToGPUFrame(int timeoutInMilliseconds, GPUFrameCb&& frameCb)
{
	if (m_destDevice == nullptr)
		return;

	ID3D11Texture2D* frame = capture(timeoutInMilliseconds);
	if (frame == nullptr)
		return false;

	if (m_captureDevice != m_destDevice)
	{
		if (DxgiUtils::isSameAdapter(m_destDevice, m_captureDevice))
		{

		}
		else
		{

		}
	}
	else
	{
		frameCb(frame);
	}
}

