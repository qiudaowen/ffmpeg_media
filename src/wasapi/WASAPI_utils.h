#pragma once

struct QsAudioParam;

namespace WASAPI_utils {
    extern bool toWAVEFORMATPCMEX(const QsAudioParam& paras, WAVEFORMATEX* pWaveFormat);
    extern bool fromWAVEFORMATPCMEX(QsAudioParam& paras, const WAVEFORMATEX* pWaveFormat);
}
