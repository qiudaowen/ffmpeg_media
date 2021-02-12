#pragma once

#include <functional>
#include <d3d11.h>
#include <atlbase.h>

using MemFrameCb = std::function<void(int w, int h, int dxgiFormat, uint8_t* data, int stride)>;
class D3D11TexureToMem
{
public:
	D3D11TexureToMem();

	void download(ID3D11Texture2D* tex, int index, MemFrameCb frameCb);
protected:
	bool ensureStage(ID3D11Texture2D* frame);
protected:
	CComPtr<ID3D11Texture2D> m_stageFrame;
};