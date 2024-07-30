///////////////////////////////////////////////////////////////////////////////
//
//  ComPtr.h
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
#include "unknwn.h"

namespace PKIsensee
{

template<typename I>
class ComPtr
{
public:
    ComPtr() : mp( nullptr )
    {
        static_assert( std::is_base_of<IUnknown, I>::value, "I needs to be IUnknown based" );
    }

    explicit ComPtr( I* p ) : mp( p )
    {
        static_assert( std::is_base_of<IUnknown, I>::value, "I needs to be IUnknown based" );
        if( mp )
            mp->AddRef();
    }

    ComPtr( const ComPtr& cp) : mp( cp.mp )
    {
        static_assert( std::is_base_of<IUnknown, I>::value, "I needs to be IUnknown based" );
        if( mp )
            mp->AddRef();
    }

    ComPtr( ComPtr&& cp ) = delete; // not currently needed, but could be replaced with code below
    /*{
        mp = cp.mp;
        cp.mp = nullptr;
    }*/

    // not currently needed
    ComPtr& operator=( const ComPtr& ) = delete; 
    ComPtr& operator=( ComPtr&& ) = delete;

    I* operator=( I* p )
    {
        if( mp )
            mp->Release();
        mp = p;
        if( mp )
            mp->AddRef();
        return mp;
    }

    ~ComPtr()
    {
        if( mp )
            mp->Release();
    }

    operator I* () const
    {
        assert( mp );
        return mp;
    }

    I* operator->() const
    {
        assert( mp );
        return mp;
    }

    I** operator&()
    {
        assert( mp == nullptr );
        return &mp;
    }

    I* Get()
    {
        return mp;
    }

    const I* Get() const
    {
        return mp;
    }

private:
    I* mp;

}; // class ComPtr

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
