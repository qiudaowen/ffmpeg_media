#pragma once

#ifdef MEDIA_DLL
#define MEDIA_API __declspec(dllexport)
#define EXPIMP_TEMPLATE
#else
#define MEDIA_API __declspec(dllimport)
#define EXPIMP_TEMPLATE extern
#endif
