///////////////////////////////////////////////////////////////////////////////
//
//  WinWindow.cpp
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

class Window::Impl
{
public:
  HWND window = NULL;
};

///////////////////////////////////////////////////////////////////////////////
//
// The deleter must handle both closing the handle (ha) and freeing the pimpl

Window::Window( void* windowHandle )
  : impl_( new Impl, []( Impl* w ) { delete w; } )
{
  impl_->window = reinterpret_cast<HWND>( windowHandle );
}

template<>
HWND Window::GetHandle<HWND>()
{
  return impl_->window;
}

#pragma warning(push)
#pragma warning(disable : 5266) // const qualifier on return type has no effect
template<>
const HWND Window::GetHandle<HWND>() const
{
  return impl_->window;
}
#pragma warning(pop)

}; // end namespace Util

}; // end namespace PKIsensee

///////////////////////////////////////////////////////////////////////////////


