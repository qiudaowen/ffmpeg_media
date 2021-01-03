#include "CaptureModel.h"
#include "libcapture/duplicator.h"

CaptureModel::CaptureModel()
{
	m_capture = new Duplicator();
}

CaptureModel::~CaptureModel()
{
	delete m_capture;
}

void CaptureModel::init(int iMonitor, const CapturCallback& cb)
{
	m_capture->unInit();
	m_capture->init(iMonitor);
	m_cb = cb;
}

bool CaptureModel::captureFrameSync()
{
	if (!m_cb) 
		return false;

	if (!m_cb.device()) {
		return m_capture->captureToMemFrame(m_cb.memCb());
	}
	return m_capture->captureToGPUFrame(m_cb.device(), m_cb.gpuCb());
}