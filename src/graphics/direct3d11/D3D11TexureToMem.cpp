#include "D3D11TexureToMem.h"
#include <d3d11.h>
#include <atlbase.h>

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

D3D11TexureToMem::D3D11TexureToMem()
{

}

bool D3D11TexureToMem::ensureStage(ID3D11Texture2D* frame)
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

void D3D11TexureToMem::download(ID3D11Texture2D* tex, int index, MemFrameCb frameCb)
{
	CComPtr<ID3D11Device> device;
	tex->GetDevice(&device);

	ensureStage(tex);

	CComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);

	context->CopyResource(m_stageFrame, tex);

	CComQIPtr<IDXGISurface> spDXGISurface = m_stageFrame;
	if (!spDXGISurface)
		return;

	DXGI_SURFACE_DESC desc = { 0 };
	spDXGISurface->GetDesc(&desc);

	DXGI_MAPPED_RECT map;
	auto hr = spDXGISurface->Map(&map, DXGI_MAP_READ);
	if (FAILED(hr))
		return;
	frameCb(desc.Width, desc.Height, desc.Format, map.pBits, map.Pitch);
	spDXGISurface->Unmap();
}