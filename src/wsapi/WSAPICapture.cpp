#include "WSAPICapture.h"
#include "QcRingBuffer.h"
#include "QsAudiodef.h"
#include "QcComInit.h"
#include "WASPI_utils.h"
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <vector>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

WSAPICapture::WSAPICapture()
{
    m_stopEvent.init();
    m_captureReadyEvent.init();
}

WSAPICapture::~WSAPICapture()
{
    stop();
}

bool WSAPICapture::init(const wchar_t* deviceID, bool bInputDevice, const QsAudioPara* para, QsAudioPara* pClosestMatch)
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

HRESULT WSAPICapture::InitDevice(const wchar_t* deviceID, bool isInputDevice, IMMDeviceEnumerator *enumerator)
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

#define BUFFER_TIME_100NS (5 * 10000000)

static auto coTaskMemFree = [](LPVOID pv) { if (pv) CoTaskMemFree(pv); };
using WAVEFORMATEXPtr = std::unique_ptr<WAVEFORMATEX, decltype(coTaskMemFree)>;

HRESULT WSAPICapture::InitClient(const QsAudioPara* para, QsAudioPara* pClosestPara)
{
    ComPtr<IAudioClient> pClient = nullptr;
    HRESULT              res;
    res = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pClient.Assign());
    if (FAILED(res))
        return res;

    std::vector<WAVEFORMATEXPtr> tryFormatList;
    if (para)
    {
        WAVEFORMATEXPtr audioFormat((WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX)), coTaskMemFree);
        WASPI_utils::toWAVEFORMATPCMEX(*para, audioFormat.get());

        tryFormatList.emplace_back(audioFormat);
    }
    if (pClosestPara)
    {
        {
            WAVEFORMATEX* audioFormat = nullptr;
            res = pClient->GetMixFormat(&audioFormat);
            if (res == S_OK)
            {
                tryFormatList.emplace_back(WAVEFORMATEXPtr(audioFormat, coTaskMemFree));
            }
        }

        {
            WAVEFORMATEXTENSIBLE* audioFormat = (WAVEFORMATEXTENSIBLE*)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
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

            tryFormatList.emplace_back(WAVEFORMATEXPtr((WAVEFORMATEX*)audioFormat, coTaskMemFree));
        }
    }
    
    
    for (const auto& audioFormat : tryFormatList)
    {
        res = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pClient.Assign());
        if (FAILED(res))
            break;

        WAVEFORMATEX* pClosest = nullptr;
        res = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, audioFormat.get(), &pClosest);
        if (res != S_OK && res != S_FALSE)
            continue;

        WAVEFORMATEXPtr autioFormat(pClosest, coTaskMemFree);

        WAVEFORMATEX* pUseFormat = pClosest == nullptr ? audioFormat.get() : pClosest;
        DWORD stream_flags = 0;
        if (!m_bInputDevice)
            stream_flags = AUDCLNT_STREAMFLAGS_LOOPBACK;
        else
            stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

        res = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, pUseFormat, 0);
        if (FAILED(res))
            continue;

        res = pClient->GetBufferSize(&m_bufferFrameCount);
        if (FAILED(res))
            break;

        res = pClient->GetService(__uuidof(IAudioRenderClient),(void**)&m_capture);
        if (FAILED(res))
            break;
        res = pClient->GetService(__uuidof(IAudioStreamVolume), (void**)&m_volControl);
		if (FAILED(res))
			break;

        if (!m_bInputDevice)
        {
                HRESULT hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audio_render_client_for_loopback);
                hr = m_audio_render_client_for_loopback->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, pUseFormat, NULL);
                m_audio_render_client_for_loopback->SetEventHandle(m_captureReadyEvent);
        }
        else
        {
            pClient->SetEventHandle(m_captureReadyEvent);
        }

        if (pClosestPara)
        {
            if (!WASPI_utils::fromWAVEFORMATPCMEX(*pClosestPara, pUseFormat))
            {
                res = AUDCLNT_E_UNSUPPORTED_FORMAT;
                continue;
            }
        }
        packet_size_frames_ = pUseFormat->nSamplesPerSec / 100;
        packet_size_bytes_ = pUseFormat->nBlockAlign * packet_size_frames_;
        break;
    };

    if (S_OK == res)
    {
        m_client = pClient;
    }
    return res;
}

bool WSAPICapture::start()
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

void WSAPICapture::stop()
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

void WSAPICapture::SetVolume(float fVolume)
{
    m_volFloat = fVolume;
    if (m_volControl)
    {
        uint32_t nChannelCount = 0;
        m_volControl->GetChannelCount(&nChannelCount);
        for (int i=0; i< (int)nChannelCount; ++i)
            m_volControl->SetChannelVolume(i, m_volFloat);
    }
}

float WSAPICapture::Volume() const
{
    return m_volFloat;
}

void WSAPICapture::captureThread()
{
    QmComInit();

    /* Output devices don't signal, so just make it check every 10 ms */
    DWORD        dur = m_bInputDevice ? 3000 : 100;
    HANDLE wait_array[] = { m_stopEvent, m_captureReadyEvent };
    for (;;)
    {
        DWORD wait_result = WaitForMultipleObjects(sizeof(wait_array)/sizeof(HANDLE), wait_array, FALSE, dur);
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

void WSAPICapture::captureData()
{
    UINT    captureSize = 0;
    for(;;)
    {
        res = m_capture->GetNextPacketSize(&captureSize);
        if (FAILED(res)) {
            break;
        }
        if (!captureSize)
            break;

        HRESULT res;
        LPBYTE  buffer;
        UINT32  frames;
        DWORD   flags;
        UINT64  pos, ts;
        res = m_capture->GetBuffer(&buffer, &frames, &flags, &pos, &ts);
        if (FAILED(res)) {
            break;
        }

        //callback.  TODO

        m_capture->ReleaseBuffer(frames);
}
