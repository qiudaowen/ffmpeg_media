#pragma once

#include <stdint.h>
#include <d3d.h>
#include <atlbase.h>
#include <memory>

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11PixelShader;

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11SamplerState;

class ShaderResource;
class VertexBuffers;
class D3D11Texture;

class D3D11Device
{
public:
	D3D11Device();
	~D3D11Device();

	bool create(uint64_t adapterLUID);
	bool createSwapChain(HWND hWnd, int w, int h);
	
	void begin();
	bool drawTexture(ID3D11Texture2D* texture, const RECT& dstRect);
	void drawTexture(D3D11Texture* pTexture, const RECT& dstRect);
	void present();
	void resize(int w, int h);

	ID3D11Device* device() const;
	ID3D11DeviceContext* deviceContext() const;
protected:
	void _resize(int w, int h);
	bool initShaderResource();
	bool initVertexBuffer();
	void makeRenderTargetView();
	void setViewPort(UINT Width, UINT Height);

	void _draw(ID3D11PixelShader* psShader, ID3D11ShaderResourceView** textureViews, int nViewCount, const RECT& dstRect);
private:
	CComPtr<ID3D11Device> m_d3d11Device;
	CComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;

	CComPtr<ID3D11SamplerState> m_samplerLinear;
	std::shared_ptr<ShaderResource> m_shaderResource;
	std::shared_ptr<VertexBuffers> m_vertexBuffers;

	CComPtr<IDXGISwapChain> m_swapChain;
	CComPtr<ID3D11RenderTargetView> m_renderTargetView;
	int m_width = 0;
	int m_height = 0;
};