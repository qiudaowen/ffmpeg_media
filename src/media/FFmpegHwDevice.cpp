#include "FFmpegHwDevice.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/hwcontext.h>
#ifdef __cplusplus
};
#endif
#include <libavutil/hwcontext_d3d11va.h>


FFmpegHwDevice::FFmpegHwDevice()
{

}

FFmpegHwDevice::~FFmpegHwDevice()
{

}

void FFmpegHwDevice::attach(ID3D11Device* pDevcie)
{
	m_hwDevice = allocHwDeviceBuffer(AV_HWDEVICE_TYPE_D3D11VA);

	AVHWDeviceContext* device_ctx = (AVHWDeviceContext*)m_hwDevice->data;
	AVD3D11VADeviceContext * d3d11_device_hwctx = (AVD3D11VADeviceContext*)device_ctx->hwctx;
	d3d11_device_hwctx->device = pDevcie;
	av_hwdevice_ctx_init(m_hwDevice.get());
}

std::shared_ptr<AVBufferRef> FFmpegHwDevice::hwDevice() const
{
	return m_hwDevice;
}

std::shared_ptr<AVBufferRef> FFmpegHwDevice::allocHwDeviceBuffer(int type)
{
	std::shared_ptr<AVBufferRef> ret;
	AVBufferRef* deviceBuffer = av_hwdevice_ctx_alloc((AVHWDeviceType)type);
	ret.reset(deviceBuffer, [](AVBufferRef* ref) {
		av_buffer_unref(&ref);
	});
	return ret;
}

void FFmpegHwDevice::detach()
{
	m_hwDevice = nullptr;
}

