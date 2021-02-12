#include "QcEfficiencyProfiler.h"
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <stdarg.h>

#define QmLocker(cs) std::unique_lock<decltype(cs)> QmUniqueVarName(cs)


static int64_t currentMilliSecsSinceEpoch()
{
	using namespace std::chrono;
	auto timePoint = steady_clock::now();
	auto cur = timePoint.time_since_epoch();
	microseconds microSeconds = duration_cast<microseconds>(cur);
	return (int)microSeconds.count();
}

struct QsProfilerData
{
	QsProfilerData(int* pId)
		: _pId(pId)
		, _iCount(0)
		, _iTotalTime(0)
		, _iMaxTime(1)
		, _iMinTime(0x7fffffffffffffff)
	{}
	int* _pId;
	char _psName[256];
	int _iCount;
	int64_t _iTotalTime;
	int64_t _iMaxTime;
	int64_t _iMinTime;
};
typedef std::vector<QsProfilerData> ProfileDataList;

static volatile int _gbProfilerOn = 0;
class QcProfilerHelper
{
private:
	QcProfilerHelper()
		: giBaseID(0)
	{
		_profilerDataList.reserve(50);
	}
	~QcProfilerHelper() {}
public:
	inline static QcProfilerHelper& GetHelper()
	{
		return _gsProfilerHelper;
	}
	inline void AllocProfiler(int* pId, const char* format, va_list argList)
	{
		QmLocker(m_mutex);
		if (*pId == 0)
		{
			_profilerDataList.emplace_back(QsProfilerData(pId));
			vsnprintf_s(_profilerDataList.back()._psName, sizeof(_profilerDataList.back()._psName), format, argList);
			*pId = _profilerDataList.size() + giBaseID;
		}
	}
	inline void FramePop(int id, int64_t beginTime)
	{
		QmLocker(m_mutex);
		int profileDataIndex = id - giBaseID - 1;
		if (profileDataIndex >= 0 && profileDataIndex < (int)_profilerDataList.size())
		{
			QsProfilerData* pProfiler = &(_profilerDataList[profileDataIndex]);
			int64_t thisTime = currentMilliSecsSinceEpoch() - beginTime;
			++pProfiler->_iCount;
			pProfiler->_iTotalTime += thisTime;
			pProfiler->_iMaxTime = std::max(pProfiler->_iMaxTime, thisTime);
			pProfiler->_iMinTime = std::min(pProfiler->_iMinTime, thisTime);
		}
	}
	inline void GetProfileResult(ProfileResultList& result)
	{
		const double milliSecs = 0.000001;
		QmLocker(m_mutex);
		result.resize(_profilerDataList.size());

		ProfileResultList::iterator destIter(result.begin());
		ProfileResultList::iterator destEndIter(result.end());
		ProfileDataList::const_iterator srcIter(_profilerDataList.begin());
		for (; destIter != destEndIter; ++destIter, ++srcIter)
		{
			destIter->_pId = srcIter->_pId;
			memcpy(destIter->_psName, srcIter->_psName, sizeof(destIter->_psName));
			destIter->_iCount = srcIter->_iCount;
			destIter->_iTotalTime = srcIter->_iTotalTime * milliSecs;
			destIter->_iMaxTime = srcIter->_iMaxTime * milliSecs;
			destIter->_iMinTime = srcIter->_iMinTime * milliSecs;
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
		giBaseID = _profilerDataList.size();
		_profilerDataList.clear();
	}
private:
	std::recursive_mutex m_mutex;
	ProfileDataList _profilerDataList;
	int giBaseID;
	static QcProfilerHelper _gsProfilerHelper;
};
QcProfilerHelper QcProfilerHelper::_gsProfilerHelper;
#define QmProfiler QcProfilerHelper::GetHelper()


QcEfficiencyProfiler::QcEfficiencyProfiler(int* pId, const char* psName, ...)
	: m_iBeginTime(0)
	, m_profileID(0)
{
	if (_gbProfilerOn)
	{
		if (*pId == 0)
		{
			va_list arglist;
			va_start(arglist, psName);
			QmProfiler.AllocProfiler(pId, psName, arglist);
			va_end(arglist);
		}
		m_iBeginTime = currentMilliSecsSinceEpoch();
		m_profileID = *pId;
	}
}
QcEfficiencyProfiler::~QcEfficiencyProfiler()
{
	if (m_iBeginTime > 0)
		QmProfiler.FramePop(m_profileID, m_iBeginTime);
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
