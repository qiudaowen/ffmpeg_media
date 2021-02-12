#pragma once

#include <thread>
#include <mutex>
#include "ComPtr.hpp"
#include "QcEvent.h"

typedef struct tWAVEFORMATEX WAVEFORMATEX;
struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioClient;
struct IAudioRenderClient;
struct IAudioStreamVolume;
struct QsAudioParam;
class QcRingBuffer;

class WASAPIPlayer
{
public:
    WASAPIPlayer();
    ~WASAPIPlayer();

    bool init(const wchar_t* deviceID, const QsAudioParam* para = nullptr, QsAudioParam* pClosestMatch = nullptr);
    bool start();
    void stop();

    void playAudio(const uint8_t* pcm, int nLen);
    void SetVolume(float fVolume);
    float Volume() const;
protected:
    HRESULT InitDevice(const wchar_t* deviceID, IMMDeviceEnumerator *enumerator);
    HRESULT InitClient(const QsAudioParam* para, QsAudioParam* pClosestMatch = nullptr);
    HRESULT _InitClient(const WAVEFORMATEX* pFormat, QsAudioParam* pClosestPara);

    void playThread();
    void fillPcmData();
    bool FillRenderEndpointBufferWithSilence(IAudioClient* client, IAudioRenderClient* render_client);
protected:
    ComPtr<IMMDevice>           m_device;
    ComPtr<IAudioClient>        m_client;
    ComPtr<IAudioRenderClient>  m_render;
    // IAudioEndpointVolume (�豸����)  
    // ISimpleAudioVolume ������?
    // IAudioStreamVolume ������ ?
    // IChannelAudioVolume ��������
    ComPtr<IAudioStreamVolume> m_volControl;
    float m_volFloat;

    uint32_t m_bufferFrameCount = 0;
    uint32_t packet_size_frames_ = 0;
    uint32_t packet_size_bytes_ = 0;

    QcEvent m_stopEvent;
    QcEvent m_readyPlayEvent;
    std::thread m_playingThread;

    std::mutex m_bufferMutex;
    std::unique_ptr<QcRingBuffer> m_pRingBuffer;
};
