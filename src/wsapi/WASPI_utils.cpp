#include <Windows.h>
#include "WASPI_utils.h"
#include "QsAudiodef.h"
#include <mmdeviceapi.h>
#include <Mmreg.h>

bool WASPI_utils::toWAVEFORMATPCMEX(const QsAudioPara& paras, WAVEFORMATEX* pWaveFormat)
{
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
    default:
        return false;
    }
    format->nChannels = paras.nChannel;
    format->nSamplesPerSec = paras.iSamplingFreq;
    format->nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
    format->nAvgBytesPerSec = format->nSamplesPerSec * format->nBlockAlign;

    return true;
}

bool WASPI_utils::fromWAVEFORMATPCMEX(QsAudioPara& paras, const WAVEFORMATEX* pUseFormat)
{
    bool bOk = true;
    switch (pUseFormat->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        if (pUseFormat->wBitsPerSample == 8)
            paras.eSample_fmt = eSampleFormatU8;
        else
            paras.eSample_fmt = eSampleFormatS16;
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        paras.eSample_fmt = eSampleFormatFloat;
        break;
    case WAVE_FORMAT_EXTENSIBLE:
    {
        WAVEFORMATEXTENSIBLE* pExFormat = (WAVEFORMATEXTENSIBLE*)pUseFormat;
        if (pExFormat->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
        {
            if (pUseFormat->wBitsPerSample == 8)
                paras.eSample_fmt = eSampleFormatU8;
            else 
                paras.eSample_fmt = eSampleFormatS16;
        }
        else if (pExFormat->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
        {
            paras.eSample_fmt = eSampleFormatFloat;
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
        paras.nChannel = pUseFormat->nChannels;
        paras.iSamplingFreq = pUseFormat->nSamplesPerSec;
    }
    return bOk;
}
