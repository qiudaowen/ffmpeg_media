#include "WSAPICapture.h"
#include "QcRingBuffer.h"
#include "QsAudiodef.h"
#include <mmdeviceapi.h>
#include <Audioclient.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

WSAPICapture::WSAPICapture()
{
    m_stopEvent.init();
    m_readyPlayEvent.init();
}

WSAPICapture::~WSAPICapture()
{
    stop();
}

bool WSAPICapture::init(const wchar_t* deviceID, const QsAudioPara* para, QsAudioPara* pClosestMatch)
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

HRESULT WSAPICapture::InitDevice(const wchar_t* deviceID, IMMDeviceEnumerator *enumerator)
{
    HRESULT res;

    if (deviceID == nullptr || wcslen(deviceID) == 0) {
        res = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, m_device.Assign());
    }
    else {
        res = enumerator->GetDevice(deviceID, m_device.Assign());
    }
    return res;
}

static bool toWAVEFORMATPCMEX(const QsAudioPara& paras, WAVEFORMATEX* pWaveFormat)
{
    WAVEFORMATEX* format = pWaveFormat;
    switch (paras.eSample_fmt)
    {
    case eSampleFormatFloat:
        format->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format->wBitsPerSample = 32;
        break;
    }
    format->nChannels = paras.nChannel;
    format->nSamplesPerSec = paras.iSamplingFreq;
    format->nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;
    format->cbSize = 0;

    return true;
}

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

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
        WAVEFORMATEXPtr autioFormat(CoTaskMemAlloc(sizeof(WAVEFORMATEX)), coTaskMemFree);
        WASPI_utils::toWAVEFORMATPCMEX(*para, audioFormat);

        tryFormatList.emplace_back(autioFormat);
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
            WAVEFORMATPCMEX* audioFormat = CoTaskMemAlloc(sizeof(WAVEFORMATPCMEX));
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
            audioFormat=>SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

            tryFormatList.emplace_back(WAVEFORMATEXPtr(audioFormat, coTaskMemFree));
        }
    }
    
    
    for (const auto& autioFormat : tryFormatList)
    {
        res = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pClient.Assign());
        if (FAILED(res))
            break;

        WAVEFORMATEX* pClosest = nullptr;
        res = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, audioFormat, &pClosest);
        if (res != S_OK && res != S_FALSE)
            continue;

        WAVEFORMATEXPtr autioFormat(pClosest, coTaskMemFree);

        WAVEFORMATEX* pUseFormat = pClosest == nullptr ? audioFormat : pClosest;
        DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
        res = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, pUseFormat, 0);
        if (FAILED(res))
            continue;

        pClient->SetEventHandle(m_readyPlayEvent);

        res = pClient->GetBufferSize(&m_bufferFrameCount);
        if (FAILED(res))
            break;

        res = pClient->GetService(__uuidof(IAudioRenderClient),(void**)&m_capture);
        if (FAILED(res))
            break;
        res = pClient->GetService(__uuidof(IAudioStreamVolume), (void**)&m_volControl);
		if (FAILED(res))
			break;

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

bool WSAPICapture::FillRenderEndpointBufferWithSilence(IAudioClient* client, IAudioCaptureClient* render_client)
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

bool WSAPICapture::start()
{
    if (!m_render)
        return false;

    FillRenderEndpointBufferWithSilence(m_client, m_render);
    m_stopEvent.resetEvent();
	m_readyPlayEvent.resetEvent();
    m_playingThread = std::thread([this]() {playThread(); });
    HRESULT hr = m_client->Start();
	return hr == S_OK;
}

void WSAPICapture::stop()
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
	if (m_client)
		m_client->Reset();
}

void WSAPICapture::playAudio(const uint8_t* pcm, int nLen)
{
    std::unique_lock<std::mutex> lock(m_bufferMutex);
    int usableSize = m_pRingBuffer->usableSize();
    if (nLen > usableSize)
        m_pRingBuffer->read(nullptr, nLen - usableSize);
    m_pRingBuffer->write((const char*)pcm, nLen);
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

void WSAPICapture::playThread()
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

void WSAPICapture::fillPcmData()
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
