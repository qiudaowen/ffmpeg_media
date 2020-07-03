#pragma once

#include "media_global.h"
#include <memory>
struct AVBufferRef;
struct ID3D11Device;

class MEDIA_API FFmpegHwDevice
{
public:
	FFmpegHwDevice();
	~FFmpegHwDevice();

	void attach(ID3D11Device* pDevcie);

	std::shared_ptr<AVBufferRef> hwDevice() const;
protected:
	std::shared_ptr<AVBufferRef> allocHwDeviceBuffer(int type);
	void detach();
protected:
	std::shared_ptr<AVBufferRef> m_hwDevice;
	std::shared_ptr<AVBufferRef> m_hw_frames_ctx;
};