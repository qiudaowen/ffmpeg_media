#pragma once

#include <stdint.h>
#include <d3d.h>
#include <atlbase.h>
#include <vector>

struct ID3D11Device;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11VertexShader;

class ShaderResource
{
public:
	enum
	{
		kPS_Begin = 0,
		kPS_Color = kPS_Begin,
		kPS_Textures,
		kPS_YUV,
		kPS_NV12,

		kPS_Max,
	};
	ShaderResource();
	~ShaderResource();

	bool init(ID3D11Device* pDevice);


	CComPtr<ID3D11InputLayout> inputLayout();
	CComPtr<ID3D11VertexShader> vertexShader();

	CComPtr<ID3D11PixelShader> pixelShader(int kType);
protected:
	CComPtr<ID3D11InputLayout> m_inputLayout;
	CComPtr<ID3D11VertexShader> m_vertexShader;

	std::vector<CComPtr<ID3D11PixelShader>> m_pixelShaderMaps;
};