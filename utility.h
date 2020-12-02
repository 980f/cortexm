#pragma once


#if __has_include("utility")
#include <utility>
#else

/** This file was extracted from files marked: */
// Move, forward and identity for C++0x + swap -*- C++ -*-

// Copyright (C) 2007-2016 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file bits/move.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{utility}
 */

/**
 * but contains very little of it, just enough to forward and move.
 *
 * The Linux build already includes this somehow, but we need this file for the firmware build.
 * */
#ifndef _MOVE_H
#define _MOVE_H 1

namespace std  {
  template<class T> struct remove_reference {
    typedef T type;
  };
  template<class T> struct remove_reference<T &> {
    typedef T type;
  };
  template< class T > struct remove_reference<T&&> {typedef T type;};

  /**
   *  @brief  Forward an lvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template<typename _Tp>  constexpr _Tp &&  forward(typename std::remove_reference<_Tp>::type &__t) noexcept { return static_cast<_Tp &&>(__t); }

  /**
   *  @brief  Forward an rvalue.
   *  @return The parameter cast to the specified type.
   *
   *  This function is used to implement "perfect forwarding".
   */
  template<typename _Tp>  constexpr _Tp &&  forward(typename std::remove_reference<_Tp>::type &&__t) noexcept {
    return static_cast<_Tp &&>(__t);
  }

  /**
   *  @brief  Convert a value to an rvalue.
   *  @param  __t  A thing of arbitrary type.
   *  @return The parameter cast to an rvalue-reference to allow moving it.
  */
  template<typename _Tp>  constexpr typename std::remove_reference<_Tp>::type &&  move(_Tp &&__t) noexcept { return static_cast<typename std::remove_reference<_Tp>::type &&>(__t); }

} // namespace
#endif /* _MOVE_H */
#endif
