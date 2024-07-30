///////////////////////////////////////////////////////////////////////////////
//
//  WinUtil.cpp
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

#include <cassert>
#include <string>

#define NOMINMAX 1
#include "Windows.h"
#include "WinFileOpen.h"
#include "Util.h"

namespace PKIsensee
{

///////////////////////////////////////////////////////////////////////////////

namespace Util
{

uint32_t GetLastError()
{
  return ::GetLastError();
}

void DebugBreak()
{
  ::DebugBreak();
}

///////////////////////////////////////////////////////////////////////////////
// 
// Currently assumes HKEY_LOCAL_MACHINE; if different keys required need to
// define platform-specific enum as new first param. Assume REG_SZ string value.

std::string GetRegistryValue( const std::string& registryPath, 
                              const std::string& registryEntry )
{
  HKEY registryKey = NULL;
  DWORD optionsDefault = 0;
  REGSAM accessRights = KEY_QUERY_VALUE;
  LSTATUS result = ::RegOpenKeyExA( HKEY_LOCAL_MACHINE, registryPath.c_str(), 
                                    optionsDefault, accessRights, &registryKey );
  if( result != ERROR_SUCCESS )
    return {};

  // Determine the size of the string, which may not include trailing null char
  DWORD dataTypeString = REG_SZ;
  DWORD charCount = 0;
  result = ::RegQueryValueExA( registryKey, registryEntry.c_str(),
                               NULL, &dataTypeString, NULL, &charCount);
  if( result == ERROR_SUCCESS )
  {
    std::string registryValue( charCount, '\0' );
    ::RegQueryValueExA( registryKey, registryEntry.c_str(), NULL, &dataTypeString, 
                        reinterpret_cast<LPBYTE>(registryValue.data()), &charCount);
    ::RegCloseKey( registryKey );
    size_t addlNullcharPos = size_t( charCount ) - 1;
    if( charCount && registryValue[ addlNullcharPos ] == '\0' ) // remove any extra nullchar
      registryValue.resize( addlNullcharPos );
    return registryValue;
  }
  return {};
}

///////////////////////////////////////////////////////////////////////////////
// 
// e.g. "notepad.exe foo.log"

void StartProcess( const std::string& commandLine )
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

///////////////////////////////////////////////////////////////////////////////
//
// Non-blocking keyboard input; returns 0 if no key events

char GetKeyReleased()
{
  HANDLE hConsoleInput = GetStdHandle( STD_INPUT_HANDLE );
  DWORD numEvents;
  if( !GetNumberOfConsoleInputEvents( hConsoleInput, &numEvents ) || numEvents == 0 )
    return 0;

  INPUT_RECORD inputRecord;
  [[maybe_unused]] DWORD eventsRead = 0;
  [[maybe_unused]] BOOL inputResult = ReadConsoleInput( hConsoleInput, &inputRecord, 1, &eventsRead );
  assert( inputResult );
  assert( eventsRead == 1 );

  // only look for key release
  if( inputRecord.EventType != KEY_EVENT )
    return 0;
  if( inputRecord.Event.KeyEvent.bKeyDown )
    return 0;

  // key is released
  return inputRecord.Event.KeyEvent.uChar.AsciiChar;
}

///////////////////////////////////////////////////////////////////////////////
//
// Windows standard open file dialog 

FileList GetFileDialog( const Window& parentWindow )
{
  WinFileOpenDialog fileOpenDialog;
  fileOpenDialog.SetOptions( fileOpenDialog.GetOptions() | FOS_ALLOWMULTISELECT | FOS_FORCEFILESYSTEM );
  FileList fileList;
  if( fileOpenDialog.Show( parentWindow.GetHandle<HWND>() ) )
  {
    WinShellItemArray shellItemArray = fileOpenDialog.GetResults();
    size_t itemCount = shellItemArray.GetCount();
    for( size_t i = 0; i < itemCount; ++i )
      fileList.push_back( shellItemArray.GetItemAt( i ).GetDisplayName() );
  }
  return fileList;
}

} // namespace Util

} // namespace PKIsensee

