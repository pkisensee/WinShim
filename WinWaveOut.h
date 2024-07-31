///////////////////////////////////////////////////////////////////////////////
//
//  WinWaveOut.h
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
#include <utility>

// Windows-specific
#define NOMINMAX 1
#include "windows.h"
#include "mmeapi.h"
#pragma comment(lib, "winmm.lib")

namespace PKIsensee
{

#ifdef _DEBUG
#define CHECK_MM(mm) assert( (mm) == MMSYSERR_NOERROR );
#else
#define CHECK_MM(mm) static_cast<void>(mm);
#endif

class WinWaveOut
{
  static constexpr uint32_t kVolChannelBits = ( sizeof( uint16_t ) * CHAR_BIT );  // 16
  static constexpr uint32_t kVolChannelLeftMask = ~( ( -1 ) << kVolChannelBits ); // 0x0000FFFF

public:
  WinWaveOut() = default;

  // Disable copy/move
  WinWaveOut( const WinWaveOut& ) = delete;
  WinWaveOut& operator=( const WinWaveOut& ) = delete;
  WinWaveOut( WinWaveOut&& ) = delete;
  WinWaveOut& operator=( WinWaveOut&& ) = delete;

  ~WinWaveOut()
  {
    Reset();
    Close();
  }

  bool Open( const WAVEFORMATEX& wfx, HANDLE hEvent )
  {
    assert( waveOutHandle_ == NULL );
    CHECK_MM( mm_ = ::waveOutOpen( &waveOutHandle_, WAVE_MAPPER, &wfx, (DWORD_PTR)( hEvent ), NULL, CALLBACK_EVENT ) );
    return mm_ == MMSYSERR_NOERROR;
  }

  void Prepare( WAVEHDR& wh )
  {
    if( wh.dwFlags & WHDR_PREPARED )
      Unprepare( wh );

    // Tell the OS to keep a reference to the memory pages so audio driver can use them safely
    // Only needs to be called once per WAVEHDR
    assert( waveOutHandle_ != NULL );
    CHECK_MM( mm_ = ::waveOutPrepareHeader( waveOutHandle_, &wh, sizeof( wh ) ) );
    assert( wh.dwFlags & WHDR_PREPARED );
  }

  void Unprepare( WAVEHDR& wh )
  {
    // Tell the OS to remove the reference to the memory pages
    assert( waveOutHandle_ != NULL );
    CHECK_MM( mm_ = ::waveOutUnprepareHeader( waveOutHandle_, &wh, sizeof( wh ) ) );
  }

  void Write( WAVEHDR& wh )
  {
    // Send buffer to audio driver
    assert( waveOutHandle_ != NULL );
    CHECK_MM( mm_ = ::waveOutWrite( waveOutHandle_, &wh, sizeof( wh ) ) );
  }

  uint32_t GetPositionBytes() const
  {
    assert( waveOutHandle_ != NULL );

    // Although waveOutGetPosition technically supports multiple output types, 
    // including milliseconds, there's no guarantee the requests will actually
    // produce what we want. Docs: "if the format is not supported, wType will 
    // specify an alternative format." The only reliable position type appears to be
    // bytes, so that's what we request.

    MMTIME mmTime = { TIME_BYTES };
    CHECK_MM( mm_ = ::waveOutGetPosition( waveOutHandle_, &mmTime, sizeof( mmTime ) ) );
    assert( mmTime.wType == TIME_BYTES );
    uint32_t bytes = mmTime.u.cb;
    return bytes;
  }

  WaveOut::Volume GetVolume() const
  {
    assert( waveOutHandle_ != NULL );
    // Left channel is in the low WORD and right channel is in the high WORD
    DWORD vol = 0;
    CHECK_MM( mm_ = ::waveOutGetVolume( waveOutHandle_, &vol ) );

    WaveOut::VolumeType leftChannel = vol & kVolChannelLeftMask;
    WaveOut::VolumeType rightChannel = vol >> kVolChannelBits;
    return std::make_pair( leftChannel, rightChannel );
  }

  void SetVolume( const WaveOut::Volume& volume )
  {
    assert( waveOutHandle_ != NULL );
    auto leftChannel = volume.first;
    auto rightChannel = volume.second;

    // Left channel is in the low WORD and right channel is in the high WORD
    DWORD vol = rightChannel;
    vol <<= kVolChannelBits;
    vol |= leftChannel;
    CHECK_MM( mm_ = ::waveOutSetVolume( waveOutHandle_, vol ) );
  }

  void Reset()
  {
    if( waveOutHandle_ != NULL )
      CHECK_MM( mm_ = ::waveOutReset( waveOutHandle_ ) );
  }

  void Close()
  {
    if( waveOutHandle_ != NULL )
    {
      CHECK_MM( mm_ = ::waveOutClose( waveOutHandle_ ) );
      waveOutHandle_ = NULL;
    }
  }

  void Restart()
  {
    assert( waveOutHandle_ != NULL );
    CHECK_MM( mm_ = ::waveOutRestart( waveOutHandle_ ) );
  }

  void Pause()
  {
    assert( waveOutHandle_ != NULL );
    CHECK_MM( mm_ = ::waveOutPause( waveOutHandle_ ) );
  }

private:
  HWAVEOUT waveOutHandle_ = NULL;
  mutable MMRESULT mm_ = MMSYSERR_NOERROR;
};

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////

