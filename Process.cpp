///////////////////////////////////////////////////////////////////////////////
//
//  WinUtil.h
//
//  Copyright © Pete Isensee (PKIsensee@msn.com).
//  All rights reserved worldwide.
//
//  Permission to copy, modify, reproduce or redistribute this source code is
//  granted provided the above copyright notice is retained in the resulting 
//  source code.
// 
//  This software is provided "as is" and without any express or implied
//  warranties.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#define NOMINMAX 1
#include <windows.h>
#include "Process.h"

namespace PKIsensee
{

///////////////////////////////////////////////////////////////////////////////

namespace WinUtil
{

void StartProcess( const std::string& commandLine ) // e.g. "notepad.exe foo.log"
{
    LPCSTR appName = NULL;
    LPSECURITY_ATTRIBUTES processAttribs = NULL;
    LPSECURITY_ATTRIBUTES threadAttribs = NULL;
    BOOL inheritHandles = FALSE;
    DWORD creationFlags = 0;
    LPVOID environment = NULL;
    LPCSTR currDir = NULL;
    STARTUPINFOA si = { sizeof( si ), NULL };
    PROCESS_INFORMATION pi = { 0 };

    ::CreateProcessA( appName, const_cast<LPSTR>( commandLine.c_str() ),
        processAttribs, threadAttribs, inheritHandles, creationFlags,
        environment, currDir, &si, &pi );

    ::CloseHandle( pi.hProcess );
    ::CloseHandle( pi.hThread );
}

} // namespace WinUtil

} // namespace PKIsensee

