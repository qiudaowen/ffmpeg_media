#include "D3D11Texture.h"
#include <d3d11.h>
#include <string>
#include "QsVideodef.h"
#include "utils/libtime.h"
#include <utils/LogTimeElapsed.h>

D3D11Texture::D3D11Texture(ID3D11Device* pDevice, ID3D11DeviceContext* deviceCtx)
	: m_d3d11Device(pDevice)
	, m_d3d11DeviceContext(deviceCtx)
{
	if (!m_d3d11DeviceContext)
	{
		m_d3d11DeviceContext = nullptr;
		m_d3d11Device->GetImmediateContext(&m_d3d11DeviceContext);
	}
}

D3D11Texture::~D3D11Texture()
{

}

ID3D11ShaderResourceView* D3D11Texture::resourceView(int index)
{
	if (index >= 0 && index < 3)
		return m_resourceViewPlanes[index];
	return nullptr;
}

CComPtr<ID3D11ShaderResourceView> createTex2DResourceView(ID3D11Device* device, ID3D11Texture2D* texture, int subResouce, int format)
{
	CComPtr<ID3D11ShaderResourceView> resView;
	D3D11_SHADER_RESOURCE_VIEW_DESC const luminancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
		texture,
		D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
		(DXGI_FORMAT)format
	);
	HRESULT hr = device->CreateShaderResourceView(
		texture,
		&luminancePlaneDesc,
		&resView
	);
	if (FAILED(hr))
	{
		return nullptr;
	}
	return resView;
}

CComPtr<ID3D11ShaderResourceView> D3D11Texture::createTex2DResourceView(ID3D11Device* device, ID3D11Texture2D* texture, int format)
{
	CComPtr<ID3D11ShaderResourceView> resView;
	D3D11_SHADER_RESOURCE_VIEW_DESC const luminancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
		texture,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		(DXGI_FORMAT)format
	);
	HRESULT hr = device->CreateShaderResourceView(
		texture,
		&luminancePlaneDesc,
		&resView
	);
	if (FAILED(hr))
	{
		return nullptr;
	}
	return resView;
}

void D3D11Texture::clear()
{
	for (int i = 0; i < 3; ++i)
	{
		m_texturePlanes[i] = nullptr;
		m_resourceViewPlanes[i] = nullptr;
	}
	m_width = 0;
	m_height = 0;
	m_dxgiFormat = DXGI_FORMAT_UNKNOWN;
}

bool D3D11Texture::updateFromTexArray(ID3D11Texture2D* tex, int index)
{
	D3D11_TEXTURE2D_DESC desc;
	tex->GetDesc(&desc);
	if (m_dxgiFormat != desc.Format || desc.Width != m_width || desc.Height != m_height)
	{
		clear();
		switch (desc.Format)
		{
		case DXGI_FORMAT_NV12:
		{
			createNV12Texture(desc.Width, desc.Height);
			break;
		}
		}
	}
	if (m_dxgiFormat == DXGI_FORMAT_UNKNOWN)
		return false;
	if (m_texturePlanes[1])
		return false;


	m_d3d11DeviceContext->CopySubresourceRegion(m_texturePlanes[0], 0,0,0,0, tex, index, NULL);
	return true;
}

void D3D11Texture::createYUVTexture(int width, int height)
{
	if (width <= 0 || height <= 0)
		return;

	CD3D11_TEXTURE2D_DESC textureDesc(DXGI_FORMAT_R8_UNORM, width, height);
	textureDesc.MipLevels = 1;
	textureDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[0]);
	if (FAILED(hr))
	{
		return;
	}

	textureDesc.Width = width / 2; textureDesc.Height = height / 2;
	hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[1]);
	if (FAILED(hr))
	{
		return;
	}
	hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[2]);
	if (FAILED(hr))
	{
		return;
	}
	for (int i = 0; i < 3; ++i)
	{
		m_resourceViewPlanes[i] = createTex2DResourceView(m_d3d11Device, m_texturePlanes[i], DXGI_FORMAT_R8_UNORM);
	}

	m_width = width;
	m_height = height;
	m_dxgiFormat = DXGI_FORMAT_420_OPAQUE;
}

bool D3D11Texture::updateYUV(const uint8_t* const datas[], const int dataSlice[], int w, int h)
{
	if (m_dxgiFormat != DXGI_FORMAT_420_OPAQUE || w != m_width || h != m_height)
	{
		clear();
		createYUVTexture(w, h);
	}
	if (m_dxgiFormat == DXGI_FORMAT_UNKNOWN)
		return false;

	D3D11_MAPPED_SUBRESOURCE res[3];
	{
		for (int i = 0; i < 3; ++i)
		{
			auto const hr = m_d3d11DeviceContext->Map(m_texturePlanes[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &res[i]);
			if (FAILED(hr))
			{
				while (--i >= 0)
				{
					m_d3d11DeviceContext->Unmap(m_texturePlanes[i], 0);
				}
				return false;
			}	
		}

		video::CopyPlane((uint8_t*)res[0].pData, (int)res[0].RowPitch, datas[0], dataSlice[0], w, h);
		video::CopyPlane((uint8_t*)res[1].pData, (int)res[1].RowPitch, datas[1], dataSlice[1], w/2, h/2);
		video::CopyPlane((uint8_t*)res[2].pData, (int)res[2].RowPitch, datas[2], dataSlice[2], w/2, h/2);

		for (int i = 0; i < 3; ++i)
		{
			m_d3d11DeviceContext->Unmap(m_texturePlanes[i], 0);
		}
	}
	return true;
}

void D3D11Texture::createNV12Texture(int width, int height)
{
	if (width <= 0 || height <= 0)
		return;

	CD3D11_TEXTURE2D_DESC textureDesc(DXGI_FORMAT_NV12, width, height);
	textureDesc.MipLevels = 1;
	textureDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[0]);
	if (FAILED(hr))
	{
		textureDesc.Format = DXGI_FORMAT_R8_UNORM;
		HRESULT hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[0]);
		if (FAILED(hr))
			return;

		textureDesc.Format = DXGI_FORMAT_R8G8_UNORM;
		textureDesc.Width = width / 2; textureDesc.Height = height / 2;
		hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[1]);
		if (FAILED(hr))
			return;

		m_resourceViewPlanes[0] = createTex2DResourceView(m_d3d11Device, m_texturePlanes[0], DXGI_FORMAT_R8_UNORM);
		m_resourceViewPlanes[1] = createTex2DResourceView(m_d3d11Device, m_texturePlanes[1], DXGI_FORMAT_R8G8_UNORM);
	}
	else
	{
		m_texturePlanes[1] = nullptr;
		m_resourceViewPlanes[0] = createTex2DResourceView(m_d3d11Device, m_texturePlanes[0], DXGI_FORMAT_R8_UNORM);
		m_resourceViewPlanes[1] = createTex2DResourceView(m_d3d11Device, m_texturePlanes[0], DXGI_FORMAT_R8G8_UNORM);
	}

	m_width = width;
	m_height = height;
	m_dxgiFormat = DXGI_FORMAT_NV12;
}

bool D3D11Texture::updateNV12(const uint8_t* const datas[], const int dataSlice[], int w, int h)
{
	if (m_dxgiFormat != DXGI_FORMAT_NV12 || w != m_width || h != m_height)
	{
		clear();
		createNV12Texture(w, h);
	}
	if (m_dxgiFormat == DXGI_FORMAT_UNKNOWN)
		return false;

	if (m_texturePlanes[1])
	{
			D3D11_MAPPED_SUBRESOURCE res;
			HRESULT hr = m_d3d11DeviceContext->Map(m_texturePlanes[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			if (FAILED(hr))
				return false;
			video::CopyPlane((uint8_t*)res.pData, (int)res.RowPitch, datas[0], dataSlice[0], w, h);
			m_d3d11DeviceContext->Unmap(m_texturePlanes[0], 0);


			hr = m_d3d11DeviceContext->Map(m_texturePlanes[1], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
			if (FAILED(hr))
				return false;
			video::CopyPlane((uint8_t*)res.pData, (int)res.RowPitch, datas[1], dataSlice[1], w/2, h/2);
			m_d3d11DeviceContext->Unmap(m_texturePlanes[1], 0);
	}
	else
	{
		D3D11_MAPPED_SUBRESOURCE res;
		auto const hr = m_d3d11DeviceContext->Map(m_texturePlanes[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
		if (FAILED(hr))
			return false;

		video::CopyPlane((uint8_t*)res.pData, (int)res.RowPitch, datas[0], dataSlice[0], w, h);
		video::CopyPlane(((uint8_t*)res.pData) + h * res.RowPitch, (int)res.RowPitch, datas[1], dataSlice[1], w, h/2);

		m_d3d11DeviceContext->Unmap(m_texturePlanes[0], 0);
	}
	return true;
}

void D3D11Texture::createRGBTexture(int width, int height, int dxgiFormat)
{
	if (width <= 0 || height <= 0)
		return;

	m_texturePlanes[0] = nullptr;
	CD3D11_TEXTURE2D_DESC textureDesc((DXGI_FORMAT)dxgiFormat, width, height);
	textureDesc.MipLevels = 1;
	textureDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = m_d3d11Device->CreateTexture2D(&textureDesc, NULL, &m_texturePlanes[0]);
	if (FAILED(hr))
		return;

	m_resourceViewPlanes[0] = createTex2DResourceView(m_d3d11Device, m_texturePlanes[0], dxgiFormat);

	m_width = width;
	m_height = height;
	m_dxgiFormat = dxgiFormat;
}

bool D3D11Texture::updateRGB32(const uint8_t* srcData, int srcSlice, int w, int h, int dxgiFormat)
{
	return lockRGB32(w, h, dxgiFormat, [&](uint8_t* dstData, int dstSlice){
		video::CopyPlane(dstData, dstSlice, srcData, srcSlice, w, h);
	});
}

bool D3D11Texture::lockRGB32(int w, int h, int dxgiFormat, std::function<void(uint8_t* dstData, int dataSlice)> cb)
{
	if (dxgiFormat == 0)
		dxgiFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	if (m_dxgiFormat != dxgiFormat || w != m_width || h != m_height)
	{
		clear();
		createRGBTexture(w, h, dxgiFormat);
	}
	if (m_dxgiFormat == DXGI_FORMAT_UNKNOWN)
		return false;

	D3D11_MAPPED_SUBRESOURCE res;
	auto const hr = m_d3d11DeviceContext->Map(m_texturePlanes[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	if (FAILED(hr))
		return false;

	cb((uint8_t*)res.pData, (int)res.RowPitch);
	//video::CopyPlane((uint8_t*)res.pData, (int)res.RowPitch, data, dataSlice, w, h);

	m_d3d11DeviceContext->Unmap(m_texturePlanes[0], 0);
}
