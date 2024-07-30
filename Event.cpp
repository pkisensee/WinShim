///////////////////////////////////////////////////////////////////////////////
//
//  Event.cpp
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
#include "Util.h"
#include <cassert>

// Windows-specific
#define NOMINMAX 1
#include "Windows.h"

namespace PKIsensee
{

namespace Util
{

class Event::Impl
{
public:
    HANDLE event = NULL;
};

///////////////////////////////////////////////////////////////////////////////
//
// The deleter must handle both closing the handle (ha) and freeing the pimpl

Event::Event()
    : mImpl( new Impl, []( Impl* e ) 
        { 
            if( e->event != NULL )
                ::CloseHandle( e->event ); 
            delete e; 
        } )
{
    mImpl->event = ::CreateEvent( NULL, FALSE, FALSE, NULL ); // auto reset event
    assert( mImpl->event != NULL );
}

void* Event::GetHandle()
{
    return mImpl->event;
}

void Event::Reset()
{
    ::ResetEvent( mImpl->event );
}

bool Event::IsSignalled( uint32_t timeoutMs ) const // true if signalled, false if timeout
{
    DWORD result = ::WaitForSingleObject( mImpl->event, timeoutMs );
    assert( result != WAIT_ABANDONED );
    assert( result != WAIT_FAILED );
    return ( result == WAIT_OBJECT_0 );
}

}; // end namespace Util

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////

