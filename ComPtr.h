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
  ComPtr() : p_( nullptr )
  {
    static_assert( std::is_base_of<IUnknown, I>::value, "I needs to be IUnknown based" );
  }

  explicit ComPtr( I* p ) : p_( p )
  {
    static_assert( std::is_base_of<IUnknown, I>::value, "I needs to be IUnknown based" );
    if( p_ )
      p_->AddRef();
  }

  ComPtr( const ComPtr& cp) : p_( cp.p_ )
  {
    static_assert( std::is_base_of<IUnknown, I>::value, "I needs to be IUnknown based" );
    if( p_ )
      p_->AddRef();
  }

  ComPtr( ComPtr&& cp ) = delete; // not currently needed, but could be replaced with code below
  /*{
    p_ = cp.p_;
    cp.p_ = nullptr;
  }*/

  // not currently needed
  ComPtr& operator=( const ComPtr& ) = delete; 
  ComPtr& operator=( ComPtr&& ) = delete;

  I* operator=( I* p )
  {
    if( p_ )
      p_->Release();
    p_ = p;
    if( p_ )
      p_->AddRef();
    return p_;
  }

  ~ComPtr()
  {
    if( p_ )
      p_->Release();
  }

  operator I* () const
  {
    assert( p_ );
    return p_;
  }

  I* operator->() const
  {
    assert( p_ );
    return p_;
  }

  I** operator&()
  {
    assert( p_ == nullptr );
    return &p_;
  }

  I* Get()
  {
    return p_;
  }

  const I* Get() const
  {
    return p_;
  }

private:
  I* p_;

}; // class ComPtr

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////
