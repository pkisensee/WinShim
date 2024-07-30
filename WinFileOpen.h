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
#include "ComPtr.h"

#define NOMINMAX 1
#include "windows.h"
#include "shobjidl.h"

#ifdef _DEBUG
#define CHECK_HR(e) assert(!FAILED(e))
#else
#define CHECK_HR(e) static_cast<void>(e);
#endif

namespace PKIsensee
{

///////////////////////////////////////////////////////////////////////////////

class WinShellItem : public ComPtr< IShellItem >
{
public:

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

///////////////////////////////////////////////////////////////////////////////

class WinFileOpenDialog : public ComPtr< IFileOpenDialog >
{
public:

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
        mIsItemSelected = ( hr == S_OK );
        return mIsItemSelected;
    }

    WinShellItem GetResult()
    {
        assert( mIsItemSelected );
        HRESULT hr;
        WinShellItem shellItem;
        CHECK_HR( hr = Get()->GetResult( &shellItem ) );
        return shellItem;
    }

    WinShellItemArray GetResults()
    {
        assert( mIsItemSelected );
        HRESULT hr;
        WinShellItemArray shellItemArray;
        CHECK_HR( hr = Get()->GetResults( &shellItemArray ) );
        return shellItemArray;
    }

private:
    bool mIsItemSelected = false;

}; // end class WinFileOpenDialog

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
