#include "VertexBuffers.h"
#include <d3d11.h>

VertexBuffers::VertexBuffers(int poolSize)
{
	m_bufferList.resize(poolSize);
	m_bufferSizeList.resize(poolSize);
}


VertexBuffers::~VertexBuffers()
{

}

CComPtr<ID3D11Buffer> VertexBuffers::updateVertex(ID3D11Device* pDevice, const uint8_t* vertexData, int dataSizeInBytes)
{
	CComPtr<ID3D11DeviceContext> deviceContext;
	pDevice->GetImmediateContext(&deviceContext);

	const int vbidx = m_currentIndex;

	if (m_bufferList[vbidx] && m_bufferSizeList[vbidx] >= dataSizeInBytes) {
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT result = deviceContext->Map(m_bufferList[vbidx], 0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource
		);
		if (FAILED(result)) {
			return nullptr;
		}
		memcpy(mappedResource.pData, vertexData, dataSizeInBytes);
		deviceContext->Unmap(m_bufferList[vbidx], 0);
	}
	else {
		D3D11_BUFFER_DESC vertexBufferDesc = {0};
		
		const UINT stride = sizeof(VertexIn);
		const UINT offset = 0;

		vertexBufferDesc.ByteWidth = (UINT)dataSizeInBytes;
		vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = vertexData;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;

		m_bufferList[vbidx] = nullptr;
		HRESULT result = pDevice->CreateBuffer(&vertexBufferDesc,
			&vertexBufferData,
			&m_bufferList[vbidx]
		);
		if (FAILED(result)) {
			return nullptr;
		}

		m_bufferSizeList[vbidx] = dataSizeInBytes;
	}

	m_currentIndex++;
	if (m_currentIndex >= (int)m_bufferList.size()) {
		m_currentIndex = 0;
	}
	return m_bufferList[vbidx];
}

