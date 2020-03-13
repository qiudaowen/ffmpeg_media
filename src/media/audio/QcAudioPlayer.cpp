#include "QcAudioPlayer.h"
#include "Data/AudioDevice/QsAudioPara.h"
#include "QcLog.hpp"
#include "QcRingBuffer.h"
#include "QcExceptionContextHelper.h"
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QThread>


QcAudioPlayer::QcAudioPlayer()
	: m_audioOutput(NULL)
	, m_pRingBuffer(NULL)
	, m_pReadBuffer(NULL)
{

}

QcAudioPlayer::~QcAudioPlayer()
{
	Close();
}

void QcAudioPlayer::Create(QsAudioPara& para)
{
	Close();

	int iSampleSize = 0;
	QAudioFormat m_format;
	switch (para.eSample_fmt)
	{
	case eSampleFormatU8:
	case eSampleFormatU8P:
	{
		m_format.setSampleType(QAudioFormat::UnSignedInt);
		m_format.setSampleSize(8);
		break;
	}
	case eSampleFormatS16:
	case eSampleFormatS16P:
	{
		m_format.setSampleType(QAudioFormat::SignedInt);
		m_format.setSampleSize(16);
		break;
	}
	case eSampleFormatS32:
	case eSampleFormatS32P:
	{
		m_format.setSampleType(QAudioFormat::SignedInt);
		m_format.setSampleSize(32);
		break;
	}
	case eSampleFormatFloat:
	case eSampleFormatFloatP:
	{
		m_format.setSampleType(QAudioFormat::Float);
		m_format.setSampleSize(32);
		break;
	}
	case eSampleFormatDouble:
	case eSampleFormatDoubleP:
	{
		m_format.setSampleType(QAudioFormat::Float);
		m_format.setSampleSize(64);
		break;
	}
	}
	m_format.setSampleRate(para.iSamplingFreq);
	m_format.setChannelCount(para.nChannel);
	m_format.setCodec("audio/pcm");
	m_format.setByteOrder(QAudioFormat::LittleEndian);

	QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
	if (!info.isFormatSupported(m_format)) 
	{
		QmLogNormal("Default format not supported - trying to use nearest");
		m_format = info.nearestFormat(m_format);
	}
	m_audioOutput = new QAudioOutput(QAudioDeviceInfo::defaultOutputDevice(), m_format);
	m_audioOutput->setBufferSize(m_format.sampleRate() * m_format.channelCount() * m_format.sampleSize() / 8);
	m_output = m_audioOutput->start();
	m_pRingBuffer = new QcRingBuffer(m_audioOutput->bufferSize());
	m_pReadBuffer = new char[m_audioOutput->periodSize()];

	para.iSamplingFreq = m_format.sampleRate();
	para.nChannel = m_format.channelCount();
	switch (m_format.sampleSize())
	{
	case 8:
	{
		para.eSample_fmt = eSampleFormatU8;
		break;
	}
	case 16:
	{
		para.eSample_fmt = eSampleFormatS16;
		break;
	}
	case 32:
	{
		if (m_format.sampleType() == QAudioFormat::SignedInt)
			para.eSample_fmt = eSampleFormatS32;
		else
			para.eSample_fmt = eSampleFormatFloat;
		break;
	}
	case 64:
	{
		para.eSample_fmt = eSampleFormatDouble;
		break;
	}
	}
}

void QcAudioPlayer::Close()
{
	if (m_audioOutput)
	{
		delete m_audioOutput;
		m_audioOutput = 0;
	}
	if (m_pRingBuffer)
    {
		delete m_pRingBuffer;
		m_pRingBuffer = NULL;
    }
	if (m_pReadBuffer)
	{
		delete [] m_pReadBuffer;
		m_pReadBuffer = NULL;
	}
}

void QcAudioPlayer::Play(bool bPlay)
{
    if (m_audioOutput)
    {
        m_audioOutput->resume();
    }
    else
    {
        m_audioOutput->suspend();
    }
}

void QcAudioPlayer::PlayAudio(const char* pcm, int nLen)
{
    QmExceptionCatch();

	if (pcm == 0 || nLen == 0)
	{
		do
		{
			QThread::msleep(10);
		} while (m_audioOutput->bytesFree() != m_audioOutput->bufferSize());
		m_pRingBuffer->Clear();
		return;
	}

	int nChunkSize = m_audioOutput->periodSize();
	while (m_audioOutput->bytesFree() >= nChunkSize  && nLen > 0)
	{
		int nBufferSize = m_pRingBuffer->size();
		if (nBufferSize >= nChunkSize)
		{
			int nSize = m_pRingBuffer->getContinueDataSize();
			if (nSize >= nChunkSize)
			{
				const char* pData = m_pRingBuffer->readContinueData(nChunkSize);
				m_output->write(pData, nChunkSize);
			}
			else
			{
				m_pRingBuffer->readData(m_pReadBuffer, nChunkSize);
				m_output->write(m_pReadBuffer, nChunkSize);
			}
		}
		else if (nBufferSize > 0)
		{
			int nCopySize = nChunkSize - nBufferSize;
			if (nLen >= nCopySize)
			{
				m_pRingBuffer->readData(m_pReadBuffer, nBufferSize);
				memcpy(m_pReadBuffer + nBufferSize, pcm, nCopySize);
				m_output->write(m_pReadBuffer, nChunkSize);

				pcm += nCopySize;
				nLen -= nCopySize;
			}
			else
			{
				//save to ring buffer;
				int nWrite = m_pRingBuffer->writeData(pcm, nLen);
				if (nWrite != nLen)
				{
                    QmAssert(false);
					QmLogLEVEL1("Fail to Write Ring Buffer");
				}
				pcm += nWrite;
				nLen -= nWrite;
			}
		}
		else
		{
			if (nLen >= nChunkSize)
			{
				m_output->write(pcm, nChunkSize);
				pcm += nChunkSize;
				nLen -= nChunkSize;
			}
			else
			{
#if 0
				//save to ring buffer;
				int nWrite = m_pRingBuffer->writeData(pcm, nLen);
#else
				int nWrite = m_output->write(pcm, nLen);
#endif
				if (nWrite != nLen)
				{
					QmAssert2(false, "Fail to Write Ring Buffer");
				}
				pcm += nWrite;
				nLen -= nWrite;
			}
		}
	}
	if (nLen > 0)
	{
		m_pRingBuffer->writeData(pcm, nLen);
	}
}

void QcAudioPlayer::SetVolume(float fVolume)
{
    qWarning("QcAudioPlayer::SetVolume  %f",fVolume);
	if (m_audioOutput)
		m_audioOutput->setVolume(fVolume);
}

float QcAudioPlayer::Volume() const
{
    if (m_audioOutput)
        m_audioOutput->volume();
    return 0.5f;
}
