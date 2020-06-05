#pragma once

#include "ComPtr.hpp"
#include "QcEvent.h"
#include "QsAudiodef.h"
#include <thread>
#include <mutex>
#include <functional>

struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioClient;
struct IAudioCaptureClient;
struct IAudioStreamVolume;
struct QsAudioPara;
struct QsAudioData;
class QcRingBuffer;

using WSAPICaptureCb = std::function<void(const QsAudioData*)>;

class WASAPICapture
{
public:
    WASAPICapture();
    ~WASAPICapture();

    void setCaptureCb(WSAPICaptureCb&& cb);
    bool init(const wchar_t* deviceID, bool bInputDevice, const QsAudioPara* para = nullptr, QsAudioPara* pClosestMatch = nullptr);
    bool start();
    void stop();

    void SetVolume(float fVolume);
    float Volume() const;
protected:
    HRESULT InitDevice(const wchar_t* deviceID, bool isInputDevice, IMMDeviceEnumerator *enumerator);
    HRESULT InitClient(const QsAudioPara* para, QsAudioPara* pClosestMatch = nullptr);
    HRESULT _InitClient(const WAVEFORMATEX* pFormat, QsAudioPara* pClosestPara);

    void captureThread();
    void captureData();
protected:
    ComPtr<IMMDevice>           m_device;
    ComPtr<IAudioClient>        m_client;
    ComPtr<IAudioClient>        m_audio_render_client_for_loopback;
    ComPtr<IAudioCaptureClient>  m_capture;
    // IAudioEndpointVolume (设备音量)  
    // ISimpleAudioVolume 主音量?
    // IAudioStreamVolume 流声音 ?
    // IChannelAudioVolume 声道声音
    ComPtr<IAudioStreamVolume> m_volControl;
    float m_volFloat;

    QcEvent m_stopEvent;
    QcEvent m_captureReadyEvent;
    std::thread m_captureThread;
    QsAudioPara m_audioPara;
    WSAPICaptureCb m_audioCb;

    bool m_bInputDevice = false;
};
