#pragma once

#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include "Utils/QcNotify.h"

struct ID3D11Device;
struct ID3D11Texture2D;
class Duplicator;
class WorkThread;
class D3D11TexureToMem;

using MemFrameCb = std::function<void(int w, int h, int dxgiFormat, uint8_t* data, int stride)>;
using GPUFrameCb = std::function<void(ID3D11Texture2D* frame)>;

class CaptureModel : public std::enable_shared_from_this<CaptureModel>
{
public:
	CaptureModel();
	~CaptureModel();

	void setDevice(ID3D11Device* pDevice);
	void setCaptureParam(int w, int h, int fps);
	void start();
	void stop();
	void waitStop();
	bool isStop() const { return m_bStop; }

	int32_t addMemFrameNotify(MemFrameCb cb);
	int32_t addGPUFrameNotify(GPUFrameCb cb);
	void removeNotify(int32_t notify);
protected:
	void captureLoop();

	void onCaptureFrame(ID3D11Texture2D* frame);
	void onCaptureMemFrame(int w, int h, int dxgiFormat, uint8_t* data, int stride);
protected:
	ID3D11Device* m_device = nullptr;
	int m_width = 0;
	int m_height = 0;
	std::atomic<uint32_t> m_fps = 0;
	std::atomic<uint32_t> m_frameTime = 0;
	bool m_bStop = true;

	//video source
	Duplicator* m_pDestopSource = nullptr;
	D3D11TexureToMem* m_texToMem = nullptr;

	WorkThread* m_thread = nullptr;
	QcNotify<MemFrameCb> m_memCb;
	QcNotify<GPUFrameCb> m_gpuCb;	
};