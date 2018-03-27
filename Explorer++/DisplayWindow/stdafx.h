// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "..\targetver.h"

#ifdef UNICODE
#define _itot_s _itow_s
#else
#define _itot_s _itoa_s
#endif

#define STRICT

#define ISOLATION_AWARE_ENABLED	1

#define OEMRESOURCE

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <winbase.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <shlwapi.h>
#include <dbt.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <tchar.h>
#include <shellapi.h>
#include <prsht.h>
#include <math.h>
#include <richedit.h>
#include <objidl.h>
#include <shlguid.h>
#include <strsafe.h>
#include <psapi.h>

/* This workaround is
required for VS 2015. */
#pragma warning(push)
#pragma warning(disable:4091)
#include <Imagehlp.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)
