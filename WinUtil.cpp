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
// Launch another process
// 
//    e.g. "notepad.exe foo.log"
// 
// If the first parameter has spaces, it must be enclosed in quotes
//    e.g. "\"c:\\Program Files\\MyApp.exe\" -C -S"
//
// Command line must be null-terminated, so std::string_view not supported

bool StartProcess( const std::string& commandLine )
{
  LPCSTR appName = NULL; // appName is specified in command line
  LPSECURITY_ATTRIBUTES processAttribs = NULL;
  LPSECURITY_ATTRIBUTES threadAttribs = NULL;
  BOOL inheritHandles = FALSE;
  DWORD creationFlags = 0;
  LPVOID environment = NULL;
  LPCSTR currDir = NULL;
  STARTUPINFOA si = { sizeof( si ), NULL };
  PROCESS_INFORMATION pi = { 0 };
  LPSTR cmdLine = const_cast<LPSTR>( commandLine.c_str() );

  auto result = ::CreateProcessA( appName, cmdLine,
      processAttribs, threadAttribs, inheritHandles, creationFlags,
      environment, currDir, &si, &pi );

  // useful for debugging failure conditions
  [[maybe_unused]] auto lastError = ::GetLastError();

  ::CloseHandle( pi.hProcess );
  ::CloseHandle( pi.hThread );

  return ( result != 0 );
}

///////////////////////////////////////////////////////////////////////////////
//
// Invoke the shell
//
// Examples:
// 
//    Verb    File          Result
//------------------------------------------------------------------------------
//            file.pdf      opens file.pdf in default PDF viewer
//    open    list.txt      opens list.txt in the default editor, e.g. Notepad
//    play    playlist.wpl  plays playlist.wpl using default media player
//    print   file.docx     prints file.docx using Word
// 
// See ShellExecuteEx in Windows documentation for all verbs. If nothing
// specified, the default is "open".
//
// String params must be null-terminated, so std::string_view not supported

bool StartShell( const std::string& verb, const std::string& file )
{
  assert( !file.empty() );

  SHELLEXECUTEINFOA shellExec = { sizeof( SHELLEXECUTEINFOA ) };
  shellExec.hwnd = ::GetDesktopWindow();
  shellExec.fMask = SEE_MASK_FLAG_NO_UI | // no error dialogs
                    SEE_MASK_NOASYNC;     // req when no message loop
  shellExec.lpVerb = verb.empty() ? NULL : verb.c_str();
  shellExec.lpFile = file.c_str();
  shellExec.lpParameters = NULL; // not currently supported, but could be added as a param
  shellExec.lpDirectory = NULL;  // start in current directory
  shellExec.nShow = SW_SHOWNORMAL;

  // Recommended by Windows documentation
  ::CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE );

  auto result = ::ShellExecuteExA( &shellExec ); // Start the shell

  // useful for debugging failure conditions
  [[maybe_unused]] auto lastError = ::GetLastError();

  return result != FALSE;
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

