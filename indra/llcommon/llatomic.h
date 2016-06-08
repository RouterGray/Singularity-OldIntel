/** 
 * @file llatmomic.h
 * @brief Base classes for atomics.
 * 
 * $LicenseInfo:firstyear=2014&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 * Copyright (C) 2014, Alchemy Development Group
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#pragma once

#define LL_BOOST_ATOMICS 1

//Internal definitions
#define NEEDS_APR_ATOMICS do_not_define_manually_thanks
#undef NEEDS_APR_ATOMICS

//Prefer boost over stl over apr.

#if LL_BOOST_ATOMICS
#include <boost/version.hpp>
#include <boost/atomic.hpp>
template<typename Type>
struct impl_atomic_type { typedef boost::atomic<Type> type; };
#elif LL_STD_ATOMICS
#include <atomic>
template<typename Type>
struct impl_atomic_type { typedef std::atomic<Type> type; };
#else
#include "apr_atomic.h"
#define NEEDS_APR_ATOMICS
template <typename Type> class LLAtomic32
{
public:
	LLAtomic32(void) { }
	LLAtomic32(LLAtomic32 const& atom) { apr_uint32_t data = apr_atomic_read32(const_cast<apr_uint32_t*>(&atom.mData)); apr_atomic_set32(&mData, data); }
	LLAtomic32(Type x) { apr_atomic_set32(&mData, static_cast<apr_uint32_t>(x)); }
	LLAtomic32& operator=(LLAtomic32 const& atom) { apr_uint32_t data = apr_atomic_read32(const_cast<apr_uint32_t*>(&atom.mData)); apr_atomic_set32(&mData, data); return *this; }

	operator Type() const { apr_uint32_t data = apr_atomic_read32(const_cast<apr_uint32_t*>(&mData)); return static_cast<Type>(data); }
	void operator=(Type x) { apr_atomic_set32(&mData, static_cast<apr_uint32_t>(x)); }
	void operator-=(Type x) { apr_atomic_sub32(&mData, static_cast<apr_uint32_t>(x)); }
	void operator+=(Type x) { apr_atomic_add32(&mData, static_cast<apr_uint32_t>(x)); }
	Type operator++(int) { return apr_atomic_inc32(&mData); } // Type++
	Type operator++() { return apr_atomic_inc32(&mData); } // ++Type
	Type operator--(int) { return apr_atomic_dec32(&mData); } // Type--
	Type operator--() { return apr_atomic_dec32(&mData); } // --Type
	
private:
	apr_uint32_t mData;
};
#endif
#if !defined(NEEDS_APR_ATOMICS)
template <typename Type> class LLAtomic32
{
public:
	LLAtomic32(void) { }
	LLAtomic32(LLAtomic32 const& atom) { mData = Type(atom.mData); }
	LLAtomic32(Type x) { mData = x; }
	LLAtomic32& operator=(LLAtomic32 const& atom) { mData = Type(atom.mData); return *this; }

	operator Type() const { return mData; }
	void operator=(Type x) { mData = x; }
	void operator-=(Type x) { mData -= x; }
	void operator+=(Type x) { mData += x; }
	Type operator++(int) { return mData++; } // Type++
	Type operator++() { return ++mData; } // ++Type
	Type operator--(int) { return mData--; } // Type--
	Type operator--() { return --mData; } // --Type

private:
	typename impl_atomic_type<Type>::type mData;
};
#endif

typedef LLAtomic32<U32> LLAtomicU32;
typedef LLAtomic32<S32> LLAtomicS32;
