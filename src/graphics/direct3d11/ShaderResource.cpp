#include "ShaderResource.h"
#include <d3d11.h>
#include <windows.h>
#include "PS_Colors.h"
#include "PS_Textures.h"
#include "PS_YUV.h"
#include "VS.h"
#include "PS_NV12.h"

static struct
{
	int iType;
	const BYTE *shader_data;
	SIZE_T shader_size;
} gShaders[] = {
	{ ShaderResource::kPS_Color, PS_Colors, sizeof(PS_Colors) },
	{ ShaderResource::kPS_Textures, PS_Textures, sizeof(PS_Textures) },
	{ ShaderResource::kPS_YUV, PS_YUV, sizeof(PS_YUV) },
	{ ShaderResource::kPS_NV12, PS_NV12, sizeof(PS_NV12) },
};

ShaderResource::ShaderResource()
{

}

ShaderResource::~ShaderResource()
{

}

bool ShaderResource::init(ID3D11Device* d3dDevice)
{
	/* Declare how the input layout for SDL's vertex shader will be setup: */
	const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	HRESULT result;

	int iSize = sizeof(VS);
	/* Load in SDL's one and only vertex shader: */
	result = d3dDevice->CreateVertexShader(
		VS,
		sizeof(VS),
		NULL,
		&m_vertexShader
	);
	if (FAILED(result)) {
		return false;
	}

	/* Create an input layout for SDL's vertex shader: */
	result = d3dDevice->CreateInputLayout(
		vertexDesc,
		ARRAYSIZE(vertexDesc),
		VS,
		sizeof(VS),
		&m_inputLayout
	);
	if (FAILED(result)) {
		return false;
	}


	for (int i = 0; i < _countof(gShaders); ++i)
	{
		CComPtr<ID3D11PixelShader> pixelShader;
		result = d3dDevice->CreatePixelShader(
			gShaders[i].shader_data,
			gShaders[i].shader_size,
			NULL,
			&pixelShader
		);
		if (FAILED(result)) {
			return false;
		}
		m_pixelShaderMaps.push_back(pixelShader);
	}

	return true;
}

ATL::CComPtr<ID3D11InputLayout> ShaderResource::inputLayout()
{
	return m_inputLayout;
}

ATL::CComPtr<ID3D11VertexShader> ShaderResource::vertexShader()
{
	return m_vertexShader;
}

ATL::CComPtr<ID3D11PixelShader> ShaderResource::pixelShader(int kType)
{
	if (kType < kPS_Begin || kType >= kPS_Max)
		return nullptr;

	return m_pixelShaderMaps[kType];
}

