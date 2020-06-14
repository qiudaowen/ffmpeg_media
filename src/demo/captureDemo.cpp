#include "CaptureDemo.h"
#include "wasapi/WASAPICapture.h"
#include "utils/CoreRunloop.h"
#include "libmedia/QcAudioTransformat.h"

CaptureDemo::CaptureDemo()
{
    m_runloop = std::make_unique<CoreRunloop>();
    m_audioCapture = std::make_unique<WASAPICapture>();
    m_audioCapture->setCaptureCb([](const QsAudioData*) {

    });
}

void CaptureDemo::run()
{
    m_runloop->run();
}

