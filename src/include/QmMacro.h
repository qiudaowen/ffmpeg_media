#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define QmDebug
#endif

/* Define offsetof macro */
#ifdef __cplusplus
#ifdef  _WIN64
#define QmOffsetof(s,m)   (size_t)( (ptrdiff_t)&reinterpret_cast<const volatile char&>((((s *)0)->m)) )
#else
#define QmOffsetof(s,m)   (size_t)&reinterpret_cast<const volatile char&>((((s *)0)->m))
#endif
#else
#ifdef  _WIN64
#define QmOffsetof(s,m)   (size_t)( (ptrdiff_t)&(((s *)0)->m) )
#else
#define QmOffsetof(s,m)   (size_t)&(((s *)0)->m)
#endif
#endif	/* __cplusplus */


#ifdef __cplusplus
#define QmExternC extern "C"
#else
#define QmExternC extern
#endif

/* Define QmCountOf macro */
#define QmCountOf(ar) (sizeof(ar)/sizeof(ar[0]))

#define QmDisableCopy(TypeName)  \
private: \
	TypeName(const TypeName&); \
	TypeName operator=(const TypeName&);

//macor str
#define _QmMakeMacorStr(macor) #macor
#define QmMakeMacorStr(macor) _QmMakeMacorStr(macor)

//unique name.
#ifndef QmUniqueVarName
#define __QmUniqueVarName(name, line) name##line
#define _QmUniqueVarName(name, line) __QmUniqueVarName(name, line)
#define QmUniqueVarName _QmUniqueVarName(defaultName, __LINE__)
#endif

//run before main
#define __QmRunBeforeMain(line, Fun, ...) static const bool QmUniqueVarName = (Fun(__VA_ARGS__),1) ? true : false
#define _QmRunBeforeMain(line, Fun, ...) __QmRunBeforeMain(__LINE__, Fun, __VA_ARGS__)
#define QmRunBeforeMain(Fun,...) _QmRunBeforeMain(__LINE__, Fun, __VA_ARGS__)

//Run Once
#define LibC_PTHREAD_ONCE_INIT 0
#define QmRunOnce(ponce_control, init_routine, ...) \
	do \
			{	\
		if (InterlockedCompareExchange(ponce_control, LibC_PTHREAD_ONCE_INIT + 1, LibC_PTHREAD_ONCE_INIT) == LibC_PTHREAD_ONCE_INIT) \
						{	\
			init_routine(__VA_ARGS__);	\
			*ponce_control = LibC_PTHREAD_ONCE_INIT + 2;	\
						}	\
								else \
								{ \
															while (*ponce_control != LibC_PTHREAD_ONCE_INIT + 2)	\
				Sleep(0);											\
								}															\
				} while (0)


#define QmAssert(c) if(!(c)) {__asm int 3}
#define QmSafeDelete(p) do {if (p){ delete p; p=NULL;} } while(0,0)
#define QmSafeDeleteArray(p) do {if (p){ delete [] p; p=NULL;} } while(0,0)
#define QmSafeRelease(p) do {if (p){ p->Release(); p=NULL;} } while(0,0)
#define QmSafeCloseHandle(p)  do {if (p){ CloseHandle(p); p=NULL;} } while(0,0)

#define QmAlignSize(size, align) ( ((size+align-1)/align) * align)
#define QmAlignSize2(size, align) (((size + align - 1) & (~(align - 1))))

#ifndef QmDebugBreak
#define QmDebugBreak() {{__asm int 3}; }
#endif

#define QmBitTest(bit, ptr) ( ((unsigned char*)ptr)[bit/8] & (1<<(bit%8)))
#define QmBitSet(bit, ptr)  ((unsigned char*)ptr)[bit/8] |= (1 << (bit & (8 - 1)))

#ifndef LOWORD
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#endif
#ifndef HIWORD
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#endif

#define QmStdMutexLocker(mutex1) std::lock_guard<std::mutex> QmUniqueVarName(mutex1)

