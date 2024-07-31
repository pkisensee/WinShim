///////////////////////////////////////////////////////////////////////////////
//
//  WinFileOpen.h
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
#include "ComPtr.h"
#include "Windows.h"
#include "ShObjIDL.h"

#ifdef _DEBUG
#define CHECK_HR(e) assert(!FAILED(e))
#else
#define CHECK_HR(e) static_cast<void>(e);
#endif

namespace PKIsensee
{

///////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable: 4626 5026 5027) // constructor/assignment implicitly defined as deleted
class WinShellItem : public ComPtr< IShellItem >
{
public:

  WinShellItem() = default;
  WinShellItem( const WinShellItem& ) = default;
  WinShellItem( WinShellItem&& ) = default;
  WinShellItem& operator=( const WinShellItem& ) = default;
  WinShellItem& operator=( WinShellItem&& ) = default;

  std::wstring GetDisplayName()
  {
    PWSTR filePath;
    HRESULT hr;
    CHECK_HR( hr = Get()->GetDisplayName( SIGDN_FILESYSPATH, &filePath ) );
    if( FAILED( hr ) )
      return {};

    std::wstring displayName = filePath;
    ::CoTaskMemFree( filePath );
    return displayName;
  }

};

///////////////////////////////////////////////////////////////////////////////

class WinShellItemArray : public ComPtr< IShellItemArray >
{
public:

  WinShellItemArray() = default;
  WinShellItemArray( const WinShellItemArray& ) = default;
  WinShellItemArray( WinShellItemArray&& ) = default;

  WinShellItemArray& operator=( const WinShellItemArray& ) = delete;
  WinShellItemArray& operator=( WinShellItemArray&& ) = delete;

  size_t GetCount()
  {
    HRESULT hr;
    DWORD itemCount;
    CHECK_HR( hr = Get()->GetCount( &itemCount ) );
    return itemCount;
  }

  WinShellItem GetItemAt( size_t index )
  {
    HRESULT hr;
    WinShellItem shellItem;
    auto i = static_cast<DWORD>( index );
    CHECK_HR( hr = Get()->GetItemAt( i, &shellItem ) );
    return shellItem;
  }

};
#pragma warning(pop)

///////////////////////////////////////////////////////////////////////////////

class WinFileOpenDialog : public ComPtr< IFileOpenDialog >
{
public:

  WinFileOpenDialog( const WinFileOpenDialog& ) = delete;
  WinFileOpenDialog( WinFileOpenDialog&& ) = delete;
  WinFileOpenDialog& operator=( const WinFileOpenDialog& ) = delete;
  WinFileOpenDialog& operator=( WinFileOpenDialog&& ) = delete;

  WinFileOpenDialog()
  {
    HRESULT hr;
    CHECK_HR( hr = ::CoInitializeEx( NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE ) );
    CHECK_HR( hr = ::CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                 IID_IFileOpenDialog, reinterpret_cast<void**>( &( *this ) ) ) );
  }

  ~WinFileOpenDialog()
  {
    ::CoUninitialize();
  }

  void SetOptions( FILEOPENDIALOGOPTIONS fileOpenDialogOptions )
  {
    // Typical call: SetOptions( GetOptions() | FOS_ALLOWMULTISELECT );
    HRESULT hr;
    CHECK_HR( hr = Get()->SetOptions( fileOpenDialogOptions ) );
  }

  FILEOPENDIALOGOPTIONS GetOptions()
  {
    HRESULT hr;
    FILEOPENDIALOGOPTIONS fileOpenDialogOptions;
    CHECK_HR( hr = Get()->GetOptions( &fileOpenDialogOptions ) );
    return fileOpenDialogOptions;
  }

  bool Show( HWND owner = NULL ) // true if item(s) selected; false if cancelled
  {
    HRESULT hr;
    hr = Get()->Show( owner );
    assert( hr == S_OK || hr == HRESULT_FROM_WIN32( ERROR_CANCELLED ) );
    isItemSelected_ = ( hr == S_OK );
    return isItemSelected_;
  }

  WinShellItem GetResult()
  {
    assert( isItemSelected_ );
    HRESULT hr;
    WinShellItem shellItem;
    CHECK_HR( hr = Get()->GetResult( &shellItem ) );
    return shellItem;
  }

  WinShellItemArray GetResults()
  {
    assert( isItemSelected_ );
    HRESULT hr;
    WinShellItemArray shellItemArray;
    CHECK_HR( hr = Get()->GetResults( &shellItemArray ) );
    return shellItemArray;
  }

private:
  bool isItemSelected_ = false;

}; // end class WinFileOpenDialog

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
