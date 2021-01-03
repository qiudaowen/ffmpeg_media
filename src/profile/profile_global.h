#pragma once

#ifdef CAPTURE_DLL
#define CAPTURE_API __declspec(dllexport)
#define EXPIMP_TEMPLATE
#else
#define CAPTURE_API __declspec(dllimport)
#define EXPIMP_TEMPLATE extern
#endif

#include "capture_def.h"
