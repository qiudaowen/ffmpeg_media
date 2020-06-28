#pragma once

#include <stdint.h>
#include <d3d.h>
#include <atlbase.h>
#include <memory>
#include <functional>

struct ID3D11Device;
struct ID3D11Texture2D;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

class D3D11Texture
{
public:
	enum KTextureType
	{
		kNone,
		kYUV420,
		kNV12,
		kBGRA32,
	};
	D3D11Texture(ID3D11Device* pDevice);
	~D3D11Texture();

	bool updateYUV(const uint8_t* const datas[], const int dataSlice[], int w, int h);
	bool updateNV12(const uint8_t* const datas[], const int dataSlice[], int w, int h);
	bool updateRGB32(const uint8_t* data, int dataSlice, int w, int h, int dxgiFormat);
	//TODO map


	int width() const { return m_width; }
	int height() const { return m_height; }
	int format() const { return m_dxgiFormat; }
	ID3D11ShaderResourceView* resourceView(int index);

	static CComPtr<ID3D11ShaderResourceView> createTex2DResourceView(ID3D11Device* device, ID3D11Texture2D* texture, int format);
protected:
	void createYUVTexture(int w, int h);
	void createNV12Texture(int w, int h);
	void createRGBTexture(int w, int h, int dxgiFormat);
	void clear();
protected:
	CComPtr<ID3D11Device> m_d3d11Device;

	int m_dxgiFormat = 0;
	int m_curTexType = kNone;
	int m_width = 0;
	int m_height = 0;
	CComPtr<ID3D11Texture2D> m_texturePlanes[3];
	CComPtr<ID3D11ShaderResourceView> m_resourceViewPlanes[3];
};