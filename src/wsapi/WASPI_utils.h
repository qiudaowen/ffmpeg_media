#pragma once

struct QsAudioPara;

namespace WASPI_utils {
    extern bool toWAVEFORMATPCMEX(const QsAudioPara& paras, WAVEFORMATEX* pWaveFormat);
    extern bool fromWAVEFORMATPCMEX(QsAudioPara& paras, const WAVEFORMATEX* pWaveFormat);
}
