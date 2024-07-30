///////////////////////////////////////////////////////////////////////////////
//
//  WinMediaFoundation.h
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
#include <filesystem>

#include "ComPtr.h"

#define NOMINMAX 1
#include "mfapi.h"
#include "mfidl.h"
#include "mfreadwrite.h"

// Link with these media libraries
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "mfuuid.lib")

#ifdef _DEBUG
#define CHECK_HR(e) assert(!FAILED(e))
#else
#define CHECK_HR(e) static_cast<void>(e);
#endif

namespace PKIsensee
{

constexpr DWORD kAllStreams = static_cast<DWORD>( MF_SOURCE_READER_ALL_STREAMS );
constexpr DWORD kFirstAudioStream = static_cast<DWORD>( MF_SOURCE_READER_FIRST_AUDIO_STREAM );

///////////////////////////////////////////////////////////////////////////////

class WinMediaFoundation
{
public:

  WinMediaFoundation()
  {
    HRESULT hr;
    CHECK_HR( hr = CoInitializeEx( 0, COINIT_MULTITHREADED ) );
    CHECK_HR( hr = MFStartup( MF_VERSION ) );
  }

  ~WinMediaFoundation()
  {
    MFShutdown();
    CoUninitialize();
  }

};

///////////////////////////////////////////////////////////////////////////////

class WinMediaType : public ComPtr< IMFMediaType >
{
public:

  WinMediaType()
  {
    HRESULT hr;
    CHECK_HR( hr = MFCreateMediaType( &( *this ) ) );
  }

  void SetGuid( const GUID& key, const GUID& value )
  {
    HRESULT hr;
    CHECK_HR( hr = Get()->SetGUID( key, value ) );
  }
};

///////////////////////////////////////////////////////////////////////////////

class WinMediaBuffer : public ComPtr< IMFMediaBuffer >
{
private:
  friend class WinMediaBufferLock;

  uint8_t* Lock( uint32_t& bytesLocked )
  {
    HRESULT hr;
    DWORD bytes = 0;
    CHECK_HR( hr = Get()->Lock( &mpData, NULL, &bytes ) );
    bytesLocked = bytes;
    return mpData;
  }

  void Unlock()
  {
    HRESULT hr;
    CHECK_HR( hr = Get()->Unlock() );
  }

  uint8_t* mpData = nullptr;
};

class WinMediaBufferLock
{
public:
  WinMediaBufferLock() = delete;

  explicit WinMediaBufferLock( WinMediaBuffer& mediaBuffer )
    :
    mMediaBuffer( mediaBuffer )
  {
    mpData = mMediaBuffer.Lock( mBytesLocked );
  }

  ~WinMediaBufferLock()
  {
    mMediaBuffer.Unlock();
  }

  uint8_t* GetData() const {
    return mpData;
  }
  uint32_t GetSize() const {
    return mBytesLocked;
  }

private:
  WinMediaBuffer& mMediaBuffer;
  uint8_t* mpData = nullptr;
  uint32_t mBytesLocked = 0;

};

///////////////////////////////////////////////////////////////////////////////

class WinMediaSample : public ComPtr< IMFSample >
{
public:

  using ComPtr<IMFSample>::operator=; // See ReadSample

  WinMediaBuffer GetMediaBuffer()
  {
    HRESULT hr;
    WinMediaBuffer mediaBuffer;
    CHECK_HR( hr = Get()->ConvertToContiguousBuffer( &mediaBuffer ) );
    return mediaBuffer;
  }
};

///////////////////////////////////////////////////////////////////////////////

enum class WinMediaOutputType
{
  PCM
};

class WinMediaSourceReader : public ComPtr< IMFSourceReader >
{
public:

  explicit WinMediaSourceReader( const std::filesystem::path& songFile )
  {
    std::filesystem::path song = songFile;
    std::wstring songWide = song.make_preferred().generic_wstring();
    HRESULT hr;
    CHECK_HR( hr = MFCreateSourceReaderFromURL( songWide.c_str(), NULL, &( *this ) ) );
  }

  void SelectStream( DWORD streamIndex ) {
    SetStreamSelection( streamIndex, TRUE );
  }
  void UnselectStream( DWORD streamIndex ) {
    SetStreamSelection( streamIndex, FALSE );
  }

  void SetCurrentMediaType( DWORD streamIndex, WinMediaType& mediaType )
  {
    HRESULT hr;
    CHECK_HR( hr = Get()->SetCurrentMediaType( streamIndex, NULL, mediaType.Get() ) );
  }

  void SelectOutput( DWORD streamIndex, WinMediaOutputType outputType )
  {
    WinMediaType mediaType;
    mediaType.SetGuid( MF_MT_MAJOR_TYPE, MFMediaType_Audio );

    switch( outputType )
    {
    case WinMediaOutputType::PCM: mediaType.SetGuid( MF_MT_SUBTYPE, MFAudioFormat_PCM ); break;
    default: assert( false ); break;
    }

    SetCurrentMediaType( streamIndex, mediaType );
  }

  bool ReadSample( DWORD streamIndex, WinMediaSample& mediaSample ) // false if end of stream
  {
    HRESULT hr = 0;
    DWORD controlFlags = 0;
    DWORD streamFlags = 0;
    IMFSample* pSample = NULL;
    CHECK_HR( hr = Get()->ReadSample( streamIndex, controlFlags, NULL, &streamFlags, NULL, &pSample ) );
    mediaSample = pSample;
    assert( !( streamFlags & MF_SOURCE_READERF_ERROR ) );
    assert( !( streamFlags & MF_SOURCE_READERF_NEWSTREAM ) );
    assert( !( streamFlags & MF_SOURCE_READERF_NATIVEMEDIATYPECHANGED ) );
    assert( !( streamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED ) );
    assert( !( streamFlags & MF_SOURCE_READERF_STREAMTICK ) );
    assert( !( streamFlags & MF_SOURCE_READERF_ALLEFFECTSREMOVED ) );

    return !( streamFlags & MF_SOURCE_READERF_ENDOFSTREAM );
  }

private:

  void SetStreamSelection( DWORD streamIndex, BOOL enabled )
  {
    HRESULT hr;
    CHECK_HR( hr = Get()->SetStreamSelection( streamIndex, enabled ) );
  }
};

///////////////////////////////////////////////////////////////////////////////

class WinMediaSourceResolver : public ComPtr< IMFSourceResolver >
{
public:

  WinMediaSourceResolver()
  {
    HRESULT hr;
    CHECK_HR( hr = MFCreateSourceResolver( &( *this ) ) );
  }
};

///////////////////////////////////////////////////////////////////////////////

class WinMediaSource : public ComPtr< IMFMediaSource >
{
public:

  WinMediaSource( WinMediaSourceResolver& sourceResolver, const std::filesystem::path& songFile )
  {
    std::filesystem::path song = songFile;
    std::wstring songWide = song.make_preferred().generic_wstring();
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
    HRESULT hr;
    CHECK_HR( hr = sourceResolver->CreateObjectFromURL( songWide.c_str(), MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &mpSource ) );
    CHECK_HR( hr = mpSource->QueryInterface( __uuidof( IMFMediaSource ), (void**)&( *this ) ) );
  }

private:

  ComPtr<IUnknown> mpSource;

};

///////////////////////////////////////////////////////////////////////////////

class WinPresentationDescriptor : public ComPtr<IMFPresentationDescriptor>
{
public:

  explicit WinPresentationDescriptor( WinMediaSource& mediaSource )
  {
    HRESULT hr;
    CHECK_HR( hr = mediaSource->CreatePresentationDescriptor( &( *this ) ) );
  }

  uint64_t GetDurationInMilliseconds() const
  {
    // get duration in 100-nanosecond units
    MFTIME duration;
    HRESULT hr;
    CHECK_HR( hr = ( *this )->GetUINT64( MF_PD_DURATION, (UINT64*)&duration ) );
    uint64_t milliSeconds = static_cast<uint64_t>(duration) / 10000;
    return milliSeconds;
  }
};

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
