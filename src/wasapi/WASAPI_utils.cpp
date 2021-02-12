#include <Windows.h>
#include "WASAPI_utils.h"
#include "QsAudiodef.h"
#include <mmdeviceapi.h>
#include <Mmreg.h>

bool WASAPI_utils::toWAVEFORMATPCMEX(const QsAudioParam& paras, WAVEFORMATEX* pWaveFormat)
{
    WAVEFORMATEX* format = pWaveFormat;
    switch (paras.sampleFormat)
    {
    case kSampleFormatU8:
        format->wFormatTag = WAVE_FORMAT_PCM;
        format->wBitsPerSample = 8;
        break;
    case kSampleFormatS16:
        format->wFormatTag = WAVE_FORMAT_PCM;
        format->wBitsPerSample = 16;
        break;
    case kSampleFormatFloat:
        format->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format->wBitsPerSample = 32;
        break;
    default:
        return false;
    }
    format->nChannels = paras.nChannels;
    format->nSamplesPerSec = paras.sampleRate;
    format->nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;

    return true;
}

bool WASAPI_utils::fromWAVEFORMATPCMEX(QsAudioParam& paras, const WAVEFORMATEX* pUseFormat)
{
    bool bOk = true;
    switch (pUseFormat->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        if (pUseFormat->wBitsPerSample == 8)
            paras.sampleFormat = kSampleFormatU8;
        else
            paras.sampleFormat = kSampleFormatS16;
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        paras.sampleFormat = kSampleFormatFloat;
        break;
    case WAVE_FORMAT_EXTENSIBLE:
    {
        WAVEFORMATEXTENSIBLE* pExFormat = (WAVEFORMATEXTENSIBLE*)pUseFormat;
        if (pExFormat->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
        {
            if (pUseFormat->wBitsPerSample == 8)
                paras.sampleFormat = kSampleFormatU8;
            else 
                paras.sampleFormat = kSampleFormatS16;
        }
        else if (pExFormat->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        {
            paras.sampleFormat = kSampleFormatFloat;
        }
        else
        {
            bOk = false;
        }
        break;
    }
    default:
        bOk = false;
        break;
    }
    if (bOk)
    {
        paras.nChannels = pUseFormat->nChannels;
        paras.sampleRate = pUseFormat->nSamplesPerSec;
    }
    return bOk;
}
