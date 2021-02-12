#ifndef QC_EFFICIENCY_PROFILER_HPP
#define QC_EFFICIENCY_PROFILER_HPP

#include "profile_global.h"
#include <vector>

#define __QmUniqueVarName(name, line) name##line
#define _QmUniqueVarName(name, line) __QmUniqueVarName(name, line)
#define QmUniqueVarName _QmUniqueVarName(defaultName, __LINE__)

//#define QmProfilerOn

struct QsProfilerResult
{
	int* _pId;
	char _psName[256];
	int _iCount;
	double _iTotalTime;
	double _iMaxTime;
	double _iMinTime;
};
typedef std::vector<QsProfilerResult> ProfileResultList;


PROFILE_API void QfProfileBegin();
PROFILE_API void QfProfileEnd();
PROFILE_API void QfProfileResult(ProfileResultList& profileData);
PROFILE_API void QfProfileClear();

class PROFILE_API QcEfficiencyProfiler
{
	QcEfficiencyProfiler(const QcEfficiencyProfiler&);
	void operator=(const QcEfficiencyProfiler&);
public:
	explicit QcEfficiencyProfiler(int* pId, const char* psName, ...);
	~QcEfficiencyProfiler();
protected:
	int64_t m_iBeginTime;
	int m_profileID;
};

#ifdef QmProfilerOn
#define __ProfileFragment(name, line, ...)\
    static int ___id_##line = 0;\
    QcEfficiencyProfiler ___obj_##line(&___id_##line, name, ##__VA_ARGS__)
#define _ProfileFragment(name, line, ...) __ProfileFragment(name, line, ##__VA_ARGS__)
#define QmProfileFragment(name, ...) _ProfileFragment(name, __LINE__, ##__VA_ARGS__)

#define QmProfileBegin() QfProfileBegin()
#define QmProfileEnd() QfProfileEnd()
#define QmProfileResult(result) QfProfileResult(result)
#define QmProfileClear() QfProfileClear()

#else
#define QmProfileFragment(name, ...) (void(0))
#define QmProfileBegin() (void(0))
#define QmProfileEnd() (void(0))
#define QmProfileResult(result) (void(0))
#define QmProfileClear() (void(0))
#endif

#endif
