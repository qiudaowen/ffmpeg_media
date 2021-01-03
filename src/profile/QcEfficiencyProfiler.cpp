#include "QcEfficiencyProfiler.h"
#include <chrono>

static int64_t currentMilliSecsSinceEpoch()
{
	using namespace std::chrono;
	auto timePoint = steady_clock::now();
	auto cur = timePoint.time_since_epoch();
	microseconds microSeconds = duration_cast<microseconds>(cur);
	return (int)microSeconds.count();
}


struct QsFrameData
{
	int64_t _iBeginTime;
	int _iId;
};
typedef std::vector<QsFrameData> ProfilerStack;
QThreadStorage<ProfilerStack*> gsFrameStack;

struct QsProfilerData
{
	QsProfilerData(int* pId, const QString& psName)
		: _pId(pId)
		, _iCount(0)
		, _iTotalTime(0)
		, _iMaxTime(1)
		, _iMinTime(0x7fffffffffffffff)
		, _psName(psName)
	{}
	int* _pId;
	QString _psName;
	int _iCount;
	Int64 _iTotalTime;
	Int64 _iMaxTime;
	Int64 _iMinTime;
};
typedef std::vector<QsProfilerData> ProfileDataList;

static volatile int _gbProfilerOn = 0;
class QcProfilerHelper
{
private:
	QcProfilerHelper()
		: m_mutex(QMutex::Recursive)
        , giBaseID(0)
	{
		_profilerDataList.reserve(50);
	}
	~QcProfilerHelper(){}
	

	ProfilerStack* GetFrameStack()
	{
		if (!gsFrameStack.hasLocalData())
		{
			gsFrameStack.setLocalData(new ProfilerStack());
		}
		return gsFrameStack.localData();
	}
public:
	inline static QcProfilerHelper& GetHelper()
	{
		return _gsProfilerHelper;
	}
	inline void AllocProfiler(int* pId, const QString& psName)
	{
		QmLocker(m_mutex);
		if (*pId == 0)
		{
			_profilerDataList.push_back(QsProfilerData(pId, psName));
            *pId = _profilerDataList.size() + giBaseID;
		}
	}
	inline int FramePush(int id)
	{
		QsFrameData data;
		data._iId = id;
		ProfilerStack* pRet = GetFrameStack();
		pRet->push_back(data);
		pRet->back()._iBeginTime = QfGetCPUTicks();
		return pRet->size();
	}
	inline void FramePop(int iFrameID)
	{
		ProfilerStack* pRet = GetFrameStack();
		if (pRet->size())
		{
            QmAssert(pRet->size() == iFrameID);
			const QsFrameData& frame = pRet->back();
			{
				QmLocker(m_mutex);
                int profileID = frame._iId - giBaseID - 1;
                if (profileID >= 0 && profileID < _profilerDataList.size())
				{
					QsProfilerData* pProfiler = &(_profilerDataList[profileID]);
                    Int64 thisTime = QfGetCPUTicks() - frame._iBeginTime;
					++pProfiler->_iCount;
					pProfiler->_iTotalTime += thisTime;
					pProfiler->_iMaxTime = max(pProfiler->_iMaxTime, thisTime);
					pProfiler->_iMinTime = min(pProfiler->_iMinTime, thisTime);
				}
			}
			pRet->pop_back();
		}
	}
	inline void GetProfileResult(ProfileResultList& result)
	{
		static double gfCPUFreq = 1.0 / QfGetCPUFrequency();
		QmLocker(m_mutex);
		result.resize(_profilerDataList.size());

		ProfileResultList::iterator destIter(result.begin());
		ProfileResultList::iterator destEndIter(result.end());
		ProfileDataList::const_iterator srcIter(_profilerDataList.begin());
		for (; destIter != destEndIter; ++destIter, ++srcIter)
		{
			destIter->_pId = srcIter->_pId;
			destIter->_psName = srcIter->_psName;
			destIter->_iCount = srcIter->_iCount;
			destIter->_iTotalTime = srcIter->_iTotalTime * gfCPUFreq;
			destIter->_iMaxTime = srcIter->_iMaxTime * gfCPUFreq;
			destIter->_iMinTime = srcIter->_iMinTime * gfCPUFreq;
		}
	}
    inline void Clear()
    {
        QmLocker(m_mutex);
        ProfileDataList::const_iterator iter(_profilerDataList.begin());
        ProfileDataList::const_iterator endIter(_profilerDataList.end());
        for (; iter != endIter; ++iter)
        {
            *(iter->_pId) = 0;
        }
        giBaseID =_profilerDataList.size();
        _profilerDataList.clear();
    }
private:
	QMutex m_mutex;
	ProfileDataList _profilerDataList;
    int giBaseID;
	static QcProfilerHelper _gsProfilerHelper;
};
QcProfilerHelper QcProfilerHelper::_gsProfilerHelper;
#define QmProfiler QcProfilerHelper::GetHelper()


QcEfficiencyProfiler::QcEfficiencyProfiler(int* pId, const char* psName, ...)
	: m_iFragmentID(0)
{
	if (_gbProfilerOn)
	{
		if (*pId == 0)
		{
			QString sName;
			va_list arglist;
			va_start(arglist, psName);
			sName.vsprintf(psName, arglist);
			va_end(arglist);
			QmProfiler.AllocProfiler(pId, sName);
		}
		m_iFragmentID = QmProfiler.FramePush(*pId);
	}
}
QcEfficiencyProfiler::~QcEfficiencyProfiler()
{
	if (m_iFragmentID > 0)
		QmProfiler.FramePop(m_iFragmentID);
}

void QfProfileBegin()
{
	_gbProfilerOn = 1;
}
void QfProfileEnd()
{
	_gbProfilerOn = 0;
}

void QfProfileResult(ProfileResultList& profileData)
{
	QmProfiler.GetProfileResult(profileData);
}

void QfProfileClear()
{
    QmProfiler.Clear();
}
