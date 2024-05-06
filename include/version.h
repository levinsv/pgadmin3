//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
//
// Copyright (C) 2002 - 2016, The pgAdmin Development Team
// This software is released under the PostgreSQL Licence
//
// version.h - pgAdmin version info
//
//////////////////////////////////////////////////////////////////////////

#ifndef VERSION_H
#define VERSION_H

// Application Versions
#ifdef CORP
#define VERSION_STR     wxT("1.22")
#define VERSION_NUM     1,22,0,0
#define VERSION_PACKAGE 1.22.2
#else
#define VERSION_STR     wxT("1.26 Dev ASUTP support PG16")
#define VERSION_NUM     1,26,0,0
#define VERSION_PACKAGE 1.26.0-dev

#endif

#define PRERELEASE 1
// #define BUILD "..."

#ifdef RC_INVOKED

#define wxT(str) str
#define _(str)   str

#include "winver.h"
#ifdef __WXDEBUG__
#define VER_DEBUG               VS_FF_DEBUG
#else
#define VER_DEBUG               0
#endif

#if PRERELEASE
#define VER_PRERELEASE          VS_FF_PRERELEASE
#else
#define VER_PRERELEASE          0
#endif
#endif
#define VERSION_WITH_DATE       wxT("Version ") VERSION_STR wxT(" (") __TDATE__ wxT(")")
#define VERSION_WITHOUT_DATE    wxT("Version ") VERSION_STR

#endif
