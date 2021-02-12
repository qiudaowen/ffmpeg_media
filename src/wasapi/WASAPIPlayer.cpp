#include "WASAPIPlayer.h"
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include "QcRingBuffer.h"
#include "QsAudiodef.h"
#include "QcComInit.h"
#include "CoTaskMemPtr.hpp"
#include "WASAPI_utils.h"


WASAPIPlayer::WASAPIPlayer()
{
    m_stopEvent.init();
    m_readyPlayEvent.init();
}

WASAPIPlayer::~WASAPIPlayer()
{
    stop();
}

bool WASAPIPlayer::init(const wchar_t* deviceID, const QsAudioParam* para, QsAudioParam* pClosestMatch)
{
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

        res = InitDevice(deviceID, enumerator);
        if (FAILED(res))
            break;

        res = InitClient(para, pClosestMatch);
        if (FAILED(res))
            break;

        return res == S_OK;
    } while (0);
    return false;
}

HRESULT WASAPIPlayer::InitDevice(const wchar_t* deviceID, IMMDeviceEnumerator *enumerator)
{
    HRESULT res;

    if (deviceID == nullptr || wcslen(deviceID) == 0) {
        res = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, m_device.Assign());
    }
    else {
        res = enumerator->GetDevice(deviceID, m_device.Assign());
    }
    return res;
}

HRESULT WASAPIPlayer::InitClient(const QsAudioParam* para, QsAudioParam* pClosestPara)
{
    HRESULT hr = S_FALSE;
    if (para)
    {
        do 
        {
            QsAudioParam audioParams = *para;
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
    return hr;
}

HRESULT WASAPIPlayer::_InitClient(const WAVEFORMATEX* pcmFormat, QsAudioParam* pClosestPara)
{
    HRESULT hr;
    do 
    {
        ComPtr<IAudioClient>        client;
        ComPtr<IAudioRenderClient>  render;
        ComPtr<IAudioStreamVolume> volControl;
        
        hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)client.Assign());
        if (FAILED(hr))
            break;

        CoTaskMemPtr<WAVEFORMATEX> closestPara;
        hr = client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pcmFormat, pClosestPara ? &closestPara : NULL);
        if (hr != S_OK && hr != S_FALSE)
            break;

        const WAVEFORMATEX* pUseFormat = hr == S_OK ? pcmFormat : closestPara.Get();

        DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
        // Initialize the shared mode client for minimal delay.
        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, pUseFormat, 0);
        if (FAILED(hr))
            break;
        hr = client->SetEventHandle(m_readyPlayEvent);
        if (FAILED(hr))
            break;

        hr = client->GetService(__uuidof(IAudioRenderClient), (void**)&render);
        if (FAILED(hr))
            break;
        hr = client->GetService(__uuidof(IAudioStreamVolume), (void**)&volControl);
        if (FAILED(hr))
            break;

        hr = client->GetBufferSize(&m_bufferFrameCount);
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

        m_client = client;
        m_render = render;
        m_volControl = volControl;

        m_pRingBuffer.reset(new QcRingBuffer(m_bufferFrameCount * pUseFormat->nBlockAlign * 4));
        packet_size_frames_ = pUseFormat->nSamplesPerSec / 100;
        packet_size_bytes_ = pUseFormat->nBlockAlign * packet_size_frames_;

    } while (0);

    return hr;
}

bool WASAPIPlayer::FillRenderEndpointBufferWithSilence(IAudioClient* client, IAudioRenderClient* render_client) 
{
    UINT32 endpoint_buffer_size = 0;
    if (FAILED(client->GetBufferSize(&endpoint_buffer_size)))
        return false;

    UINT32 num_queued_frames = 0;
    if (FAILED(client->GetCurrentPadding(&num_queued_frames)))
        return false;

    BYTE* data = NULL;
    int num_frames_to_fill = endpoint_buffer_size - num_queued_frames;
    if (FAILED(render_client->GetBuffer(num_frames_to_fill, &data)))
        return false;

    // Using the AUDCLNT_BUFFERFLAGS_SILENT flag eliminates the need to
    // explicitly write silence data to the rendering buffer.
    return SUCCEEDED(render_client->ReleaseBuffer(num_frames_to_fill, AUDCLNT_BUFFERFLAGS_SILENT));
}

bool WASAPIPlayer::start()
{
    if (!m_render)
        return false;

    // Ensure that the endpoint buffer is prepared with silence.
    // see audio_low_latency_output_win.cc  WASAPIAudioOutputStream::Start
    FillRenderEndpointBufferWithSilence(m_client, m_render);
    m_stopEvent.resetEvent();
	m_readyPlayEvent.resetEvent();
    m_playingThread = std::thread([this]() {playThread(); });
    HRESULT hr = m_client->Start();
	return hr == S_OK;
}

void WASAPIPlayer::stop()
{
    if (!m_render)
        return;

	if (m_client)
		m_client->Stop();
	if (m_playingThread.joinable())
	{
		m_stopEvent.setEvent();
		m_playingThread.join();
	}
    m_client = nullptr;
}

void WASAPIPlayer::playAudio(const uint8_t* pcm, int nLen)
{
    std::unique_lock<std::mutex> lock(m_bufferMutex);
    int usableSize = m_pRingBuffer->usableSize();
    if (nLen > usableSize)
        m_pRingBuffer->read(nullptr, nLen - usableSize);
    m_pRingBuffer->write((const char*)pcm, nLen);
}

void WASAPIPlayer::SetVolume(float fVolume)
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

float WASAPIPlayer::Volume() const
{
    return m_volFloat;
}

void WASAPIPlayer::playThread()
{
    HANDLE wait_array[] = { m_stopEvent, m_readyPlayEvent };
    for (;;)
    {
        DWORD wait_result = WaitForMultipleObjects(sizeof(wait_array)/sizeof(HANDLE), wait_array, FALSE,INFINITE);
        if (wait_result == WAIT_OBJECT_0 + 0)
        {
            break;
        }
        else if (wait_result == WAIT_OBJECT_0 + 1)
        {
            fillPcmData();
        }
        else
        {
            //error.
        }
    }
}

void WASAPIPlayer::fillPcmData()
{
    HRESULT hr = S_FALSE;
    UINT32 num_queued_frames = 0;
    size_t num_available_frames = 0;

    hr = m_client->GetCurrentPadding(&num_queued_frames);
    num_available_frames = m_bufferFrameCount - num_queued_frames;

    if (num_available_frames < packet_size_frames_)
        return;

    uint8_t* audio_data = NULL;
    const size_t num_packets = num_available_frames / packet_size_frames_;
    for (size_t n = 0; n < num_packets; ++n) {
        hr = m_render->GetBuffer(packet_size_frames_, &audio_data);
        if (FAILED(hr)) {
            return;
        }

        DWORD flags = AUDCLNT_BUFFERFLAGS_SILENT;
        {
            std::unique_lock<std::mutex> lock(m_bufferMutex);
            if (m_pRingBuffer->size() > packet_size_bytes_)
            {
                m_pRingBuffer->read((char*)audio_data, packet_size_bytes_);
                flags = 0;
            }
			else
			{
				flags = flags;
			}
        }
        m_render->ReleaseBuffer(packet_size_frames_, flags);
    }
}
