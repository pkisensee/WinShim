///////////////////////////////////////////////////////////////////////////////
//
//  WaveOut.cpp
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
#include <algorithm>
#include <cassert>
#include <climits>

#include "Util.h"
#include "PcmData.h"
#include "WaveOut.h"
#include "WinWaveOut.h"

#define NOMINMAX 1
#include "Windows.h"

///////////////////////////////////////////////////////////////////////////////
//
// Windows-specific implementation of PCM playback
//
// https://chromium.googlesource.com/chromium/src/media/+/master/audio/win/waveout_output_win.cc

namespace PKIsensee
{

constexpr size_t kWaveBufferBytes = sizeof( uint16_t ) * 2 * 44100; // 1 second for 16-bit stereo 44.1 KHz
[[maybe_unused]] constexpr size_t kMaxWaveBuffers = 16;

class WaveOut::Impl
{
public:
  std::vector<WAVEHDR> waveHdr;
  WinWaveOut           waveOut;
  PcmData              pcmData;
  const uint8_t*       nextPcm = nullptr;
  uint32_t             lastStartOffsetBytes = 0;
  bool                 isPlaying = false;
  bool                 hasEnded = false;

  WaveOut::Impl() = default;
  WaveOut::Impl( const WaveOut::Impl& ) = delete;
  WaveOut::Impl( WaveOut::Impl&& ) = delete;
  WaveOut::Impl& operator=( const WaveOut::Impl& ) = delete;
  WaveOut::Impl& operator=( WaveOut::Impl&& ) = delete;

  void Clear()
  {
    waveHdr.clear();
    waveOut.Close();
    nextPcm = nullptr;
    lastStartOffsetBytes = 0;
    isPlaying = false;
    hasEnded = false;
  }
};

///////////////////////////////////////////////////////////////////////////////
//
// Handy helper sets the pointer/len and returns number of bytes filled

size_t SetWaveHeader( WAVEHDR& wh, const uint8_t* pcmPtr, size_t bytes )
{
  assert( pcmPtr != nullptr );
  auto bytesFilled = std::min( kWaveBufferBytes, bytes );
  wh.dwBufferLength = static_cast<DWORD>( bytesFilled );
  wh.lpData = reinterpret_cast<LPSTR>( const_cast<uint8_t*>( pcmPtr ) );
  return bytesFilled;
}

///////////////////////////////////////////////////////////////////////////////

WaveOut::WaveOut()
  : impl_( new WaveOut::Impl, []( WaveOut::Impl* w ) { delete w; } )
{
}

bool WaveOut::Open( const PcmData& pcmData, Util::Event& callbackEvent )
{
  Close();
  impl_->pcmData = pcmData;

  WAVEFORMATEX wfx = { 0 };
  wfx.wFormatTag      = WAVE_FORMAT_PCM;
  wfx.nChannels       = static_cast<uint16_t>( pcmData.GetChannelCountAsInt() );
  wfx.wBitsPerSample  = static_cast<uint16_t>( pcmData.GetBitsPerSample() );
  wfx.nSamplesPerSec  = pcmData.GetSamplesPerSecond();
  wfx.nBlockAlign     = static_cast<uint16_t>( pcmData.GetBlockAlignment() );
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  wfx.cbSize          = 0; // not used for PCM

  // callbackEvent is signalled when it's time to refill the next audio buffer
  return impl_->waveOut.Open( wfx, callbackEvent.GetHandle() );
}

// Chromium (link above) supports a minimum of 2 and a maximum of 4 buffers (waveBufferCount)

void WaveOut::Prepare( uint32_t positionMs, size_t waveBufferCount )
{
  assert( waveBufferCount > 1 );
  assert( waveBufferCount <= kMaxWaveBuffers );
  static_cast<void>(kMaxWaveBuffers); // [[maybe_unused]] above not working
  impl_->waveHdr.resize( waveBufferCount );
  impl_->waveOut.Reset();
  Pause(); // pause so no events are fired

  auto* pcmPtr = impl_->pcmData.GetPtr();
  auto pcmBytes = impl_->pcmData.GetSize();

  auto byteOffset = impl_->pcmData.MillisecondsToBytes( positionMs );
  assert( byteOffset <= pcmBytes );
  auto bytesLeft = pcmBytes - byteOffset;
  impl_->nextPcm = pcmPtr + byteOffset;
  for( auto i = 0u; i < impl_->waveHdr.size(); ++i )
  {
    // Set buffers to point at audio data
    auto& wh = impl_->waveHdr[ i ];
    wh = { 0 };
    auto bytesFilled = SetWaveHeader( wh, impl_->nextPcm, bytesLeft );

    bytesLeft -= bytesFilled;
    assert( bytesLeft < pcmBytes );
    impl_->nextPcm += bytesFilled;

    // Inform OS about this WAVEHDR and send buffer to audio driver
    impl_->waveOut.Prepare( wh ); 
    impl_->waveOut.Write( wh );
  }
  impl_->lastStartOffsetBytes = byteOffset;
}

void WaveOut::Start()
{
  impl_->waveOut.Restart();
  impl_->isPlaying = true;
  impl_->hasEnded = false;
}

void WaveOut::Pause()
{
  impl_->waveOut.Pause();
  impl_->isPlaying = false;
}

void WaveOut::Update() // invoke when callbackEvent is signalled
{
  auto* pcmPtr = impl_->pcmData.GetPtr();
  auto pcmBytes = impl_->pcmData.GetSize();
  auto waveBufferCount = impl_->waveHdr.size();

  // No more data to queue
  if( impl_->nextPcm >= pcmPtr + pcmBytes )
  {
    // If all buffers are complete, wave is done playing
    uint32_t isWaveDonePlaying = WHDR_DONE;
    for( auto i = 0u; i < waveBufferCount; ++i )
      isWaveDonePlaying &= (impl_->waveHdr[ i ].dwFlags & WHDR_DONE);
    if( isWaveDonePlaying )
      impl_->hasEnded = true;
    return;
  }

  // Data remains to queue
  auto bytesLeft = pcmPtr - impl_->nextPcm + pcmBytes;
  assert( bytesLeft );
  for( auto i = 0u; i < waveBufferCount; ++i )
  {
    auto& wh = impl_->waveHdr[ i ];
    if( wh.dwFlags & WHDR_DONE ) // if this is the buffer that was signalled
    {
      // Refill it with new data
      auto bytesFilled = SetWaveHeader( wh, impl_->nextPcm, bytesLeft );
      bytesLeft -= bytesFilled;
      assert( bytesLeft < pcmBytes );
      impl_->nextPcm += bytesFilled;

      // waveOut.Prepare() is not necessary since we're reusing the buffers
      impl_->waveOut.Write( wh );
      break;
    }
  }
}

bool WaveOut::IsPlaying() const
{
  return impl_->isPlaying;
}

bool WaveOut::HasEnded() const
{
  return impl_->hasEnded;
}

void WaveOut::Close()
{
  impl_->waveOut.Reset();
  for( auto i = 0u; i < impl_->waveHdr.size(); ++i )
  {
    auto& wh = impl_->waveHdr[ i ];
    // waveOutReset() leaves buffers in unpredictable state; fix it here
    // before calling waveOutUnprepare()
    wh.dwFlags = WHDR_PREPARED; 
    impl_->waveOut.Unprepare( wh );
  }
  impl_->Clear();
}

WaveOut::Volume WaveOut::GetVolume() const // left, right
{
  return impl_->waveOut.GetVolume();
}

void WaveOut::SetVolume( const WaveOut::Volume& volume ) // left, right
{
  impl_->waveOut.SetVolume( volume );
}

uint32_t WaveOut::GetPositionMs() const
{
  auto bytePosition = impl_->lastStartOffsetBytes;
  bytePosition += impl_->waveOut.GetPositionBytes();
  return impl_->pcmData.BytesToMilliseconds( bytePosition );
}

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
