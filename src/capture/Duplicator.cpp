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

	CComPtr<IDXGIOutput> dxgiOutput;
	CComPtr<IDXGIAdapter> adapter;
	if (DxgiUtils::monitorToOutputIndex(iMonitor, NULL, &dxgiOutput, &adapter) != S_OK)
		return false;

	CComPtr<IDXGIOutput1> output1;
	auto hr  = dxgiOutput->QueryInterface(IID_PPV_ARGS(&output1));
	if (FAILED(hr))
		return false;

	CComPtr<ID3D11Device> captureDevice;
	CComPtr<IDXGIAdapter> curAdapter;
	hr = DxgiUtils::getGetAdapter(pDevice, &curAdapter);
	if (pDevice == nullptr || !DxgiUtils::isSameAdapter(curAdapter, adapter))
	{
		hr = D3D11Device::createDevice(adapter, &captureDevice, nullptr);
		if (FAILED(hr))
			return false;
	}
	else
	{
		captureDevice = pDevice;
	}
	CComPtr<IDXGIOutputDuplication> dup;
	hr = output1->DuplicateOutput(captureDevice, &dup);
	if (FAILED(hr))
		return false;

	m_duplicator = dup;
	m_captureDevice = captureDevice;
	m_iMonitor = iMonitor;
}

void Duplicator::unInit()
{
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

	//TODO
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

