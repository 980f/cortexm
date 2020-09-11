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

#ifndef _MOVE_H
#define _MOVE_H 1

//#include <bits/c++config.h>
//#include <bits/concept_check.h>

//namespace std {
//  /**
//   *  @brief Same as C++11 std::addressof
//   *  @ingroup utilities
//   */
//  template<typename _Tp> inline _Tp * __addressof(_Tp &__r) noexcept {
//    return reinterpret_cast<_Tp *> (&const_cast<char &>(reinterpret_cast<const volatile char &>(__r)));
//  }
//} // namespace
//
//#include <type_traits> // Brings in std::declval too.

namespace std  {
  template< class T > struct remove_reference      {typedef T type;};
  template< class T > struct remove_reference<T&>  {typedef T type;};
  template< class T > struct remove_reference<T&&> {typedef T type;};
  /**
   *  @addtogroup utilities
   *  @{
   */

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
//    static_assert(!std::is_lvalue_reference<_Tp>::value, "template argument"  " substituting _Tp is an lvalue reference type");
    return static_cast<_Tp &&>(__t);
  }

  /**
   *  @brief  Convert a value to an rvalue.
   *  @param  __t  A thing of arbitrary type.
   *  @return The parameter cast to an rvalue-reference to allow moving it.
  */
  template<typename _Tp>  constexpr typename std::remove_reference<_Tp>::type &&  move(_Tp &&__t) noexcept { return static_cast<typename std::remove_reference<_Tp>::type &&>(__t); }

//  template<typename _Tp>  struct __move_if_noexcept_cond    : public __and_<__not_<is_nothrow_move_constructible<_Tp>>,      is_copy_constructible<_Tp>>::type {
//  };

//  /**
//   *  @brief  Conditionally convert a value to an rvalue.
//   *  @param  __x  A thing of arbitrary type.
//   *  @return The parameter, possibly cast to an rvalue-reference.
//   *
//   *  Same as std::move unless the type's move constructor could throw and the
//   *  type is copyable, in which case an lvalue-reference is returned instead.
//   */
//  template<typename _Tp>  constexpr typename   conditional<__move_if_noexcept_cond<_Tp>::value, const _Tp &, _Tp &&>::type  move_if_noexcept(_Tp &__x) noexcept { return std::move(__x); }
//
//  // declval, from type_traits.

//  /**
//   *  @brief Returns the actual address of the object or function
//   *         referenced by r, even in the presence of an overloaded
//   *         operator&.
//   *  @param  __r  Reference to an object or function.
//   *  @return   The actual address.
//  */
//  template<typename _Tp>  inline _Tp *  addressof(_Tp &__r) noexcept { return std::__addressof(__r); }
//
//  // C++11 version of std::exchange for internal use.
//  template<typename _Tp, typename _Up = _Tp>  inline _Tp  __exchange(_Tp &__obj, _Up &&__new_val) {
//    _Tp __old_val = std::move(__obj);
//    __obj = std::forward<_Up>(__new_val);
//    return __old_val;
//  }

} // namespace

#endif /* _MOVE_H */

