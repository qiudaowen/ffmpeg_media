#pragma once

#include <atlbase.h>

struct ID3D11Device;
struct ID3D11Texture2D;

struct IDXGIOutputDuplication;

class Duplicator
{
public:
	Duplicator();
	~Duplicator();

	bool init(int iMonitor, ID3D11Device* pDevice = nullptr);
	void unInit();

	ID3D11Texture2D* capture(int timeoutInMilliseconds = 0);
protected:
	bool recreateDuplicator();
protected:
	bool m_hasFrame = false;
	CComPtr<ID3D11Texture2D> m_frame;
	CComPtr<ID3D11Device> m_captureDevice;
	CComPtr<IDXGIOutputDuplication> m_duplicator;
	int m_iMonitor = 0;
};