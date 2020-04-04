#include "WSAPIPlayer.h"
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

WSAPIPlayer::WSAPIPlayer()
{
    m_stopEvent.init();
    m_readyPlayEvent.init();
}

WSAPIPlayer::~WSAPIPlayer()
{
    stop();
}

bool WSAPIPlayer::init(const wchar_t* deviceID, const QsAudioPara& para, QsAudioPara* pClosestMatch)
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

HRESULT WSAPIPlayer::InitDevice(const wchar_t* deviceID, IMMDeviceEnumerator *enumerator)
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

static QsAudioPara toSupportFormat(const QsAudioPara& paras)
{
    QsAudioPara ret = paras;
    switch (paras.eSample_fmt)
    {
    case eSampleFormatDouble:
    case eSampleFormatDoubleP:
        ret.eSample_fmt = eSampleFormatFloat;
        break;
    case eSampleFormatS32:
    case eSampleFormatS32P:
        ret.eSample_fmt = eSampleFormatS16;
        break;
    default:
        ret.eSample_fmt = toNonPlanarFormat(paras.eSample_fmt);
        break;
    }
    ret.nChannel = std::min(paras.nChannel, 2);
    return ret;
}

static bool toWAVEFORMATPCMEX(const QsAudioPara& paras, WAVEFORMATEX* pWaveFormat)
{
    if (paras.nChannel > 2)
        return false;
    if (IsAudioPlanarFormat(paras.eSample_fmt) 
        || paras.eSample_fmt == eSampleFormatS32
        || paras.eSample_fmt == eSampleFormatDouble)
        return false;

    WAVEFORMATEX* format = pWaveFormat;
    switch (paras.eSample_fmt)
    {
    case eSampleFormatU8:
        format->wFormatTag = WAVE_FORMAT_PCM;
        format->wBitsPerSample = 8;
        break;
    case eSampleFormatS16:
        format->wFormatTag = WAVE_FORMAT_PCM;
        format->wBitsPerSample = 16;
        break;
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

HRESULT WSAPIPlayer::InitClient(const QsAudioPara& para, QsAudioPara* pClosestPara)
{
    ComPtr<IAudioClient> pClient = nullptr;
    HRESULT                res;
    //DWORD                  flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

    WAVEFORMATEX* pClosest = nullptr;
    WAVEFORMATEX pcmFormat = { 0 };
    do 
    {
        res = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)pClient.Assign());
        if (FAILED(res))
            break;

        if (!toWAVEFORMATPCMEX(para, &pcmFormat))
        {
            if (pClosestPara == nullptr) {
                res = AUDCLNT_E_UNSUPPORTED_FORMAT;
                break;
            }
            else
            {
                toWAVEFORMATPCMEX(toSupportFormat(para), &pcmFormat);
            }
        }

        res = pClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &pcmFormat, &pClosest);
        if (res == AUDCLNT_E_UNSUPPORTED_FORMAT)
            break;

        WAVEFORMATEX* pUseFormat = pClosest == nullptr ? &pcmFormat : pClosest;
        DWORD stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
        // Initialize the shared mode client for minimal delay.
        res = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED, stream_flags, 0, 0, pUseFormat, 0);
        if (FAILED(res))
            break;
        pClient->SetEventHandle(m_readyPlayEvent);

        res = pClient->GetBufferSize(&m_bufferFrameCount);
        if (FAILED(res))
            break;
        m_pRingBuffer.reset(new QcRingBuffer(m_bufferFrameCount * pUseFormat->nBlockAlign));

        res = m_client->GetService(__uuidof(IAudioRenderClient),(void**)&m_render);
        if (FAILED(res))
            break;
        res = m_client->GetService(__uuidof(IAudioStreamVolume), (void**)&m_volControl);

        if (pClosestPara)
        {
            switch (pUseFormat->wFormatTag)
            {
            case WAVE_FORMAT_PCM:
                if (pUseFormat->wBitsPerSample == 8)
                    pClosestPara->eSample_fmt = eSampleFormatU8;
                else
                    pClosestPara->eSample_fmt = eSampleFormatS16;
                break;
            case WAVE_FORMAT_IEEE_FLOAT:
                pClosestPara->eSample_fmt = eSampleFormatFloat;
                break;
            default:
                res = AUDCLNT_E_UNSUPPORTED_FORMAT;
                break;
            }
            if (res == S_OK)
            {
                pClosestPara->nChannel = pUseFormat->nChannels;
                pClosestPara->iSamplingFreq = pUseFormat->nSamplesPerSec;
            }
        }

        packet_size_frames_ = pUseFormat->nSamplesPerSec / 100;
        packet_size_bytes_ = pUseFormat->nBlockAlign * packet_size_frames_;
    } while (0);

    if (pClosest)
    {
        CoTaskMemFree(pClosest);
    }

    if (S_OK == res)
    {
        m_client = pClient;
    }
    return res;
}

bool WSAPIPlayer::FillRenderEndpointBufferWithSilence(IAudioClient* client, IAudioRenderClient* render_client) 
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

void WSAPIPlayer::start()
{
    if (!m_render)
        return;

    FillRenderEndpointBufferWithSilence(m_client, m_render);
    m_stopEvent.resetEvent();
    m_playingThread = std::thread([this]() {playThread(); });
    HRESULT hr = m_client->Start();
}

void WSAPIPlayer::stop()
{
    if (!m_render)
        return;

    m_client->Stop();
    m_stopEvent.setEvent();
    m_playingThread.join();
    m_client->Reset();
}

void WSAPIPlayer::playAudio(const char* pcm, int nLen)
{
    std::unique_lock<std::mutex> lock(m_bufferMutex);
    int usableSize = m_pRingBuffer->usableSize();
    if (nLen > usableSize)
        m_pRingBuffer->read(nullptr, nLen - usableSize);
    m_pRingBuffer->write(pcm, nLen);
}

void WSAPIPlayer::SetVolume(float fVolume)
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

float WSAPIPlayer::Volume() const
{
    return m_volFloat;
}

void WSAPIPlayer::playThread()
{
    HANDLE wait_array[] = { m_stopEvent, m_readyPlayEvent };
    for (;;)
    {
        DWORD wait_result = WaitForMultipleObjects(sizeof(wait_array), wait_array, FALSE,INFINITE);
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

void WSAPIPlayer::fillPcmData()
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
        }
        m_render->ReleaseBuffer(packet_size_frames_, flags);
    }
}
