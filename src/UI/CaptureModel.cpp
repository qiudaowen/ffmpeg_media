#include "CaptureModel.h"
#include "libcapture/duplicator.h"
#include "utils/libtime.h"
#include "utils/WorkThread.h"
#include "libgraphics/D3D11TexureToMem.h"

CaptureModel::CaptureModel()
{
}

CaptureModel::~CaptureModel()
{
	waitStop();
}

void CaptureModel::setDevice(ID3D11Device* pDevice)
{
	m_device = pDevice;
}
void CaptureModel::setCaptureParam(int w, int h, int fps)
{
	m_width = w;
	m_height = h;
	m_fps = fps;
	m_frameTime = 1000 / fps;
}
void CaptureModel::start()
{
	waitStop();

	m_pDestopSource = new Duplicator();
	m_pDestopSource->init(0);
	m_texToMem = new D3D11TexureToMem();

	m_bStop = false;
	m_thread = new WorkThread();
	m_thread->setRunFunc(std::bind(&CaptureModel::captureLoop, this));
	m_thread->start();
}
void CaptureModel::stop()
{
	m_bStop = true;
}

void CaptureModel::waitStop()
{
	stop();
	if (m_thread) {
		delete m_thread;
		m_thread = nullptr;
	}
	if (m_pDestopSource)
	{
		delete m_pDestopSource;
		m_pDestopSource = nullptr;
	}
	if (m_texToMem) {
		delete m_texToMem;
		m_texToMem = nullptr;
	}
}

int32_t CaptureModel::addMemFrameNotify(MemFrameCb cb)
{
	return m_memCb.addNotify(std::move(cb)) & 0xFFFF;
}
int32_t CaptureModel::addGPUFrameNotify(GPUFrameCb cb)
{
	return m_gpuCb.addNotify(std::move(cb)) << 16;
}
void CaptureModel::removeNotify(int32_t notifyID)
{
	if (notifyID <= 0)
		return;

	int32_t memNotifyID = notifyID & 0xFFFF;
	int32_t gpuNotifyID = notifyID >> 16;
	if (memNotifyID > 0){
		m_memCb.removeNotify(memNotifyID);
	}
	else if (gpuNotifyID > 0){
		m_gpuCb.removeNotify(gpuNotifyID);
	}
}

void CaptureModel::captureLoop() {

	libtime::Time curTime;
	while (!m_bStop)
	{
		if (m_device)
		{
			m_pDestopSource->captureToGPUFrame(m_device, [this](ID3D11Texture2D* frame) {
				onCaptureFrame(frame);
				});
		}
		else {
			m_pDestopSource->captureToMemFrame([this](int w, int h, int dxgiFormat, uint8_t* data, int stride) {
				onCaptureMemFrame(w, h, dxgiFormat, data, stride);
				});
		}
		int32_t waitTime = m_frameTime - curTime.elapsedMS(true);
		if (waitTime > 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
		}
	}
}

void CaptureModel::onCaptureFrame(ID3D11Texture2D* frame)
{
	m_gpuCb.invoke(frame);

	m_texToMem->download(frame, 0, [this](int w, int h, int dxgiFormat, uint8_t* data, int stride) {
		m_memCb.invoke(w, h, dxgiFormat, data, stride);
		});
}

void CaptureModel::onCaptureMemFrame(int w, int h, int dxgiFormat, uint8_t* data, int stride)
{
	//TODO m_gpuCb
	m_memCb.invoke(w, h, dxgiFormat, data, stride);
}