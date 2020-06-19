#include "WASAPICapture.h"
#include "QcRingBuffer.h"
#include "QsAudiodef.h"
#include "QcComInit.h"
#include "CoTaskMemPtr.hpp"
#include "WASAPI_utils.h"
#include <mmdeviceapi.h>
#include <Audioclient.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

WASAPICapture::WASAPICapture()
{
    m_stopEvent.init();
    m_captureReadyEvent.init();
}

WASAPICapture::~WASAPICapture()
{
    stop();
}

void WASAPICapture::setCaptureCb(WSAPICaptureCb&& cb)
{
    m_audioCb = cb;
}

bool WASAPICapture::init(const wchar_t* deviceID, bool bInputDevice, const QsAudioPara* para, QsAudioPara* pClosestMatch)
{
    stop();
    m_bInputDevice = bInputDevice;

    ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT res;

    do
    {
        res = CoCreateInstance(__uuidof(MMDeviceEnumerator),
            nullptr, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void**)enumerator.Assign());
        if (FAILED(res))
            break;

        res = InitDevice(deviceID, bInputDevice, enumerator);
        if (FAILED(res))
            break;

        res = InitClient(para, pClosestMatch);
        if (FAILED(res))
            break;

        return res == S_OK;
    } while (0);
    return false;
}

HRESULT WASAPICapture::InitDevice(const wchar_t* deviceID, bool isInputDevice, IMMDeviceEnumerator *enumerator)
{
    HRESULT res;

    if (deviceID == nullptr || wcslen(deviceID) == 0) {
        res = enumerator->GetDefaultAudioEndpoint(isInputDevice ? eCapture : eRender, eConsole, m_device.Assign());
    }
    else {
        res = enumerator->GetDevice(deviceID, m_device.Assign());
    }
    return res;
}


HRESULT WASAPICapture::InitClient(const QsAudioPara* para, QsAudioPara* pClosestPara)
{
    HRESULT hr = S_FALSE;
    if (para)
    {
        do
        {
            QsAudioPara audioParams = *para;
            if (IsAudioPlanarFormat(audioParams.sampleFormat) && pClosestPara)
                audioParams.sampleFormat = toNonPlanarFormat(audioParams.sampleFormat);

            CoTaskMemPtr<WAVEFORMATEX> audioFormat((WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
            audioFormat->cbSize = 0;
            if (!WASAPI_utils::toWAVEFORMATPCMEX(audioParams, audioFormat))
                break;

            hr = _InitClient(audioFormat, pClosestPara);
            if (FAILED(hr))
                break;
        } while (0);
    }
    if (hr != S_OK && pClosestPara)
    {
        do
        {
            ComPtr<IAudioClient>   client;
            hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client.Assign());
            if (FAILED(hr))
                break;

            CoTaskMemPtr<WAVEFORMATEX> audioFormat;
            hr = client->GetMixFormat(&audioFormat);
            if (FAILED(hr))
                break;

            hr = _InitClient(audioFormat, pClosestPara);
            if (FAILED(hr))
                break;
        } while (0);

        if (hr != S_OK)
        {
            CoTaskMemPtr<WAVEFORMATEXTENSIBLE> audioFormat((WAVEFORMATEXTENSIBLE*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE)));
            WAVEFORMATEX* format = &(audioFormat->Format);
            format->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            format->nChannels = 2;
            format->nSamplesPerSec = 44100;
            format->wBitsPerSample = 32;
            format->nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
            format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
            format->cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

            audioFormat->Samples.wValidBitsPerSample = format->wBitsPerSample;
            audioFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
            audioFormat->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

            hr = _InitClient((WAVEFORMATEX*)audioFormat.Get(), pClosestPara);
        }
    }
    if (hr == S_OK)
    {
        m_audioPara = pClosestPara ? *pClosestPara : *para;
    }
    return hr;
}

HRESULT WASAPICapture::_InitClient(const WAVEFORMATEX* pcmFormat, QsAudioPara* pClosestPara)
{
    HRESULT              hr = S_FALSE;
    do
    {
        ComPtr<IAudioClient>        client;
        ComPtr<IAudioCaptureClient>  capture;
        ComPtr<IAudioClient> audio_render_client_for_loopback;
        ComPtr<IAudioStreamVolume> volControl;

        hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client.Assign());
        if (FAILED(hr))
            break;

        CoTaskMemPtr<WAVEFORMATEX> closestPara;
        hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pcmFormat, pClosestPara ? &closestPara : NULL);
        if (hr != S_OK && hr != S_FALSE)
            break;
        const WAVEFORMATEX* pUseFormat = hr == S_OK ? pcmFormat : closestPara.Get();

        DWORD stream_flags = 0;
        if (!m_bInputDevice)
            stream_flags = AUDCLNT_STREAMFLAGS_LOOPBACK;
        else
            stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, pUseFormat, 0);
        if (FAILED(hr))
            break;

        hr = client->GetService(__uuidof(IAudioRenderClient), (void**)&capture);
        if (FAILED(hr))
            break;
        hr = client->GetService(__uuidof(IAudioStreamVolume), (void**)&volControl);
        if (FAILED(hr))
            break;

        if (pClosestPara)
        {
            if (!WASAPI_utils::fromWAVEFORMATPCMEX(*pClosestPara, pUseFormat))
            {
                hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
                break;
            }
        }

        if (!m_bInputDevice)
        {
            hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audio_render_client_for_loopback);
            if (FAILED(hr))
                break;
            hr = audio_render_client_for_loopback->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, pUseFormat, NULL);
            if (FAILED(hr))
                break;
            audio_render_client_for_loopback->SetEventHandle(m_captureReadyEvent);
        }
        else
        {
            hr = client->SetEventHandle(m_captureReadyEvent);
            if (FAILED(hr))
                break;
        }

        m_client = client;
        m_capture = capture;
        m_volControl = volControl;
        m_audio_render_client_for_loopback = audio_render_client_for_loopback;
    } while (0);
    return hr;
}

bool WASAPICapture::start()
{
    if (!m_capture)
        return false;

    m_stopEvent.resetEvent();
    m_captureReadyEvent.resetEvent();
    m_captureThread = std::thread([this]() { captureThread(); });
    HRESULT hr = m_client->Start();
    if (m_audio_render_client_for_loopback)
        m_audio_render_client_for_loopback->Start();
    return hr == S_OK;
}

void WASAPICapture::stop()
{
    if (!m_capture)
        return;

    if (m_client)
        m_client->Stop();
    if (m_captureThread.joinable())
    {
        m_stopEvent.setEvent();
        m_captureThread.join();
    }
    m_client = nullptr;
}

void WASAPICapture::SetVolume(float fVolume)
{
    m_volFloat = fVolume;
    if (m_volControl)
    {
        uint32_t nChannelCount = 0;
        m_volControl->GetChannelCount(&nChannelCount);
        for (int i = 0; i < (int)nChannelCount; ++i)
            m_volControl->SetChannelVolume(i, m_volFloat);
    }
}

float WASAPICapture::Volume() const
{
    return m_volFloat;
}

void WASAPICapture::captureThread()
{
    QmComInit();

    /* Output devices don't signal, so just make it check every 10 ms */
    DWORD        dur = m_bInputDevice ? 3000 : 100;
    HANDLE wait_array[] = { m_stopEvent, m_captureReadyEvent };
    for (;;)
    {
        DWORD wait_result = WaitForMultipleObjects(sizeof(wait_array) / sizeof(HANDLE), wait_array, FALSE, dur);
        if (wait_result == WAIT_OBJECT_0 + 0)
        {
            break;
        }
        else if (wait_result == WAIT_OBJECT_0 + 1 || wait_result == WAIT_TIMEOUT)
        {
            captureData();
        }
        else
        {
            //error.
        }
    }
}

void WASAPICapture::captureData()
{
    UINT    captureSize = 0;
    for (;;)
    {
        HRESULT res = m_capture->GetNextPacketSize(&captureSize);
        if (FAILED(res)) {
            break;
        }
        if (!captureSize)
            break;

        LPBYTE  buffer;
        UINT32  frames;
        DWORD   flags;
        UINT64  pos, ts;
        res = m_capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
        if (FAILED(res)) {
            break;
        }

        //callback.  TODO
        if (m_audioCb)
        {
            QsAudioData data;
            data.sampleRate = m_audioPara.sampleRate;
            data.sampleFormat = m_audioPara.sampleFormat;
            data.nChannels = m_audioPara.nChannels;

            data.data[0] = (const uint8_t *)buffer;
            data.frames = (uint32_t)frames;

            data.timestamp = GetTickCount();

            m_audioCb(&data);
        }
        m_capture->ReleaseBuffer(frames);
    }
}
