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
#include "WaveOut.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include "Util.h"
#include "PcmData.h"
#include "WinWaveOut.h"

// Windows-specific
#define NOMINMAX 1
#include "windows.h"

///////////////////////////////////////////////////////////////////////////////
//
// Windows-specific implementation of PCM playback
//
// https://chromium.googlesource.com/chromium/src/media/+/master/audio/win/waveout_output_win.cc

namespace PKIsensee
{

constexpr size_t kWaveBufferBytes = sizeof( uint16_t ) * 2 * 44100; // 1 second for 16-bit stereo 44.1 KHz
constexpr size_t kMaxWaveBuffers = 16;
constexpr double kMillisecondsPerSecond = 1000.0;

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
// Also hides nasty Windows cast

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
    : mImpl( new WaveOut::Impl, []( WaveOut::Impl* w ) { delete w; } )
{
}

bool WaveOut::Open( const PcmData& pcmData, Util::Event& callbackEvent )
{
    Close();
    mImpl->pcmData = pcmData;

    WAVEFORMATEX wfx;
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = static_cast<uint16_t>( pcmData.GetChannelCountAsInt() );
    wfx.wBitsPerSample  = static_cast<uint16_t>( pcmData.GetBitsPerSample() );
    wfx.nSamplesPerSec  = pcmData.GetSamplesPerSecond();
    wfx.nBlockAlign     = static_cast<uint16_t>( pcmData.GetBlockAlignment() );
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize          = 0; // not used for PCM

    // callbackEvent is signalled when it's time to refill the next audio buffer
    return mImpl->waveOut.Open( wfx, callbackEvent.GetHandle() );
}

void WaveOut::Prepare( uint32_t positionMs, size_t waveBufferCount )
{
    assert( waveBufferCount > 1 );
    assert( waveBufferCount <= kMaxWaveBuffers );
    mImpl->waveHdr.resize( waveBufferCount );
    mImpl->waveOut.Reset();
    Pause(); // pause so no events are fired

    auto* pcmPtr = mImpl->pcmData.GetPtr();
    auto pcmBytes = mImpl->pcmData.GetSize();

    auto byteOffset = mImpl->pcmData.MillisecondsToBytes( positionMs );
    assert( byteOffset <= pcmBytes );
    auto bytesLeft = pcmBytes - byteOffset;
    mImpl->nextPcm = pcmPtr + byteOffset;
    for( auto i = 0u; i < mImpl->waveHdr.size(); ++i )
    {
        // Set buffers to point at audio data
        auto& wh = mImpl->waveHdr[ i ];
        wh = { 0 };
        auto bytesFilled = SetWaveHeader( wh, mImpl->nextPcm, bytesLeft );

        bytesLeft -= bytesFilled;
        assert( bytesLeft < pcmBytes );
        mImpl->nextPcm += bytesFilled;

        // Inform OS about this WAVEHDR and send buffer to audio driver
        mImpl->waveOut.Prepare( wh ); 
        mImpl->waveOut.Write( wh );
    }
    mImpl->lastStartOffsetBytes = byteOffset;
}

void WaveOut::Start()
{
    mImpl->waveOut.Restart();
    mImpl->isPlaying = true;
    mImpl->hasEnded = false;
}

void WaveOut::Pause()
{
    mImpl->waveOut.Pause();
    mImpl->isPlaying = false;
}

void WaveOut::Update() // invoke when callbackEvent is signalled
{
    auto* pcmPtr = mImpl->pcmData.GetPtr();
    auto pcmBytes = mImpl->pcmData.GetSize();
    auto waveBufferCount = mImpl->waveHdr.size();

    // No more data to queue
    if( mImpl->nextPcm >= pcmPtr + pcmBytes )
    {
        // If all buffers are complete, wave is done playing
        uint32_t isWaveDonePlaying = WHDR_DONE;
        for( auto i = 0u; i < waveBufferCount; ++i )
            isWaveDonePlaying &= (mImpl->waveHdr[ i ].dwFlags & WHDR_DONE);
        if( isWaveDonePlaying )
            mImpl->hasEnded = true;
        return;
    }

    // Data remains to queue
    auto bytesLeft = pcmPtr - mImpl->nextPcm + pcmBytes;
    assert( bytesLeft );
    for( auto i = 0u; i < waveBufferCount; ++i )
    {
        auto& wh = mImpl->waveHdr[ i ];
        if( wh.dwFlags & WHDR_DONE ) // if this is the buffer that was signalled
        {
            // Refill it with new data
            auto bytesFilled = SetWaveHeader( wh, mImpl->nextPcm, bytesLeft );
            bytesLeft -= bytesFilled;
            assert( bytesLeft < pcmBytes );
            mImpl->nextPcm += bytesFilled;

            // waveOut.Prepare() is not necessary since we're reusing the buffers
            mImpl->waveOut.Write( wh );
            break;
        }
    }
}

bool WaveOut::IsPlaying() const
{
    return mImpl->isPlaying;
}

bool WaveOut::HasEnded() const
{
    return mImpl->hasEnded;
}

void WaveOut::Close()
{
    mImpl->waveOut.Reset();
    for( auto i = 0u; i < mImpl->waveHdr.size(); ++i )
    {
        auto& wh = mImpl->waveHdr[ i ];
        // waveOutReset() leaves buffers in unpredictable state; fix it here
        // before calling waveOutUnprepare()
        wh.dwFlags = WHDR_PREPARED; 
        mImpl->waveOut.Unprepare( wh );
    }
    mImpl->Clear();
}

WaveOut::Volume WaveOut::GetVolume() const // left, right
{
    return mImpl->waveOut.GetVolume();
}

void WaveOut::SetVolume( const WaveOut::Volume& volume ) // left, right
{
    mImpl->waveOut.SetVolume( volume );
}

uint32_t WaveOut::GetPositionMs() const
{
    auto bytePosition = mImpl->lastStartOffsetBytes;
    bytePosition += mImpl->waveOut.GetPositionBytes();
    return mImpl->pcmData.BytesToMilliseconds( bytePosition );
}

} // namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
