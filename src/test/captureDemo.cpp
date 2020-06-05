#include "CaptureDemo.h"
#include "wasapi/WASAPICapture.h"
#include "libmedia/QcAudioTransformat.h"

CaptureDemo::CaptureDemo()
{
    m_audioCapture = std::make_unique<WASAPICapture>();
    m_audioCapture->setCaptureCb([](const QsAudioData*) {

    });
}

void CaptureDemo::run()
{

}

