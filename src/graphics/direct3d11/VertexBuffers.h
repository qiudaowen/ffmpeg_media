#pragma once

#include <DirectXMath.h>
#include <stdint.h>
#include <d3d.h>
#include <atlbase.h>
#include <memory>
#include <vector>

using namespace DirectX;
struct ID3D11Device;
struct ID3D11Buffer;

struct VertexIn
{
	XMFLOAT3 position;
	XMFLOAT2 tex;
	LONG color;
};

class VertexBuffers
{

public:
	VertexBuffers(int poolSize);
	~VertexBuffers();

	CComPtr<ID3D11Buffer> updateVertex(ID3D11Device* pDevice, const uint8_t* data, int dataSize);
protected:
	std::vector<CComPtr<ID3D11Buffer>> m_bufferList;
	std::vector<int> m_bufferSizeList;
	int m_currentIndex = 0;
};