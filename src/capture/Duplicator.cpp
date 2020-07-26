#include "Duplicator.h"
#include <dxgi1_2.h>
#include <d3d11.h>
#include "libgraphics/DxgiUtils.h"
#include "libgraphics/D3D11Device.h"

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

	CComPtr<ID3D11Device> device;
	CComPtr<IDXGIOutputDuplication> dup;
	auto hr = DxgiUtils::createDuplicator(iMonitor, pDevice, &dup, &device, NULL);
	if (hr != S_OK)
		return false;

	m_duplicator = dup;
	m_captureDevice = device;
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

