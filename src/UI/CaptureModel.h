#pragma once

#include <memory>
#include <functional>

struct ID3D11Device;
struct ID3D11Texture2D;
class Duplicator;

class CapturCallback {
public:
	using MemFrameCb = std::function<void(int w, int h, int dxgiFormat, uint8_t* data, int stride)>;
	using GPUFrameCb = std::function<void(ID3D11Texture2D* frame)>;
	CapturCallback() {}
	CapturCallback(MemFrameCb&& cb)
		: m_memCb(std::move(cb))
	{

	}
	CapturCallback(ID3D11Device* pDevice, GPUFrameCb&& cb)
		: m_pDevice(pDevice)
		, m_gpuCb(std::move(cb))
	{

	}
	operator bool() {
		return m_gpuCb || m_memCb;
	}

	ID3D11Device* device() const { return m_pDevice; }
	const MemFrameCb& memCb() const { return m_memCb; }
	const GPUFrameCb& gpuCb() const { return m_gpuCb; }
protected:
	friend class CaptureModel;
	ID3D11Device* m_pDevice = nullptr;
	MemFrameCb m_memCb;
	GPUFrameCb m_gpuCb;
};

class CaptureModel : public std::enable_shared_from_this<CaptureModel>
{
public:
	CaptureModel();
	~CaptureModel();

	void init(int iMonitor, const CapturCallback& cb);
	bool captureFrameSync();
protected:
	Duplicator* m_capture = nullptr;
	CapturCallback m_cb;
};