#pragma once

#ifdef PROFILE_DLL
#define PROFILE_API __declspec(dllexport)
#define EXPIMP_TEMPLATE
#else
#define PROFILE_API __declspec(dllimport)
#define EXPIMP_TEMPLATE extern
#endif
