#ifndef QC_EFFICIENCY_PROFILER_HPP
#define QC_EFFICIENCY_PROFILER_HPP

#define __QmUniqueVarName(name, line) name##line
#define _QmUniqueVarName(name, line) __QmUniqueVarName(name, line)
#define QmUniqueVarName _QmUniqueVarName(defaultName, __LINE__)

#define QmLocker(cs) QMutexLocker QmUniqueVarName(&cs)

//#define QmProfilerOn
#include <string>
#include <vector>


struct QsProfilerResult
{
	int* _pId;
	std::string _psName;
	int _iCount;
	double _iTotalTime;
	double _iMaxTime;
	double _iMinTime;
};
typedef std::vector<QsProfilerResult> ProfileResultList;


extern void QfProfileBegin();
extern void QfProfileEnd();
extern void QfProfileResult(ProfileResultList& profileData);
extern void QfProfileClear();

class QcEfficiencyProfiler
{
	QcEfficiencyProfiler(const QcEfficiencyProfiler&);
	void operator=(const QcEfficiencyProfiler&);
public:
	explicit QcEfficiencyProfiler(int* pId, const char* psName, ...);
	~QcEfficiencyProfiler();
protected:
	int m_iFragmentID;
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
