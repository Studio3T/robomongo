/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_atomic.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/capabilities.hpp>
#if defined(__clang__) && (defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B) || defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B))
#include <boost/atomic/detail/ops_gcc_x86_dcas.hpp>
#include <boost/atomic/detail/ops_cas_based.hpp>
#endif

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__INTEL_COMPILER)
// This is used to suppress warning #32013 described below for Intel Compiler.
// In debug builds the compiler does not inline any functions, so basically
// every atomic function call results in this warning. I don't know any other
// way to selectively disable just this one warning.
#pragma system_header
#endif

namespace boost {
namespace atomics {
namespace detail {

/*!
 * The function converts \c boost::memory_order values to the compiler-specific constants.
 *
 * NOTE: The intention is that the function is optimized away by the compiler, and the
 *       compiler-specific constants are passed to the intrinsics. I know constexpr doesn't
 *       work in this case because the standard atomics interface require memory ordering
 *       constants to be passed as function arguments, at which point they stop being constexpr.
 *       However it is crucial that the compiler sees constants and not runtime values,
 *       because otherwise it just ignores the ordering value and always uses seq_cst.
 *       This is the case with Intel C++ Compiler 14.0.3 (Composer XE 2013 SP1, update 3) and
 *       gcc 4.8.2. Intel Compiler issues a warning in this case:
 *
 *       warning #32013: Invalid memory order specified. Defaulting to seq_cst memory order.
 *
 *       while gcc acts silently.
 *
 *       To mitigate the problem ALL functions, including the atomic<> members must be
 *       declared with BOOST_FORCEINLINE. In this case the compilers are able to see that
 *       all functions are called with constant orderings and call intrinstcts properly.
 *
 *       Unfortunately, this still doesn't work in debug mode as the compiler doesn't
 *       inline functions even when marked with BOOST_FORCEINLINE. In this case all atomic
 *       operaions will be executed with seq_cst semantics.
 */
BOOST_FORCEINLINE BOOST_CONSTEXPR int convert_memory_order_to_gcc(memory_order order) BOOST_NOEXCEPT
{
    return (order == memory_order_relaxed ? __ATOMIC_RELAXED : (order == memory_order_consume ? __ATOMIC_CONSUME :
        (order == memory_order_acquire ? __ATOMIC_ACQUIRE : (order == memory_order_release ? __ATOMIC_RELEASE :
        (order == memory_order_acq_rel ? __ATOMIC_ACQ_REL : __ATOMIC_SEQ_CST)))));
}

template< typename T >
struct gcc_atomic_operations
{
    typedef T storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        __atomic_store_n(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_load_n(&storage, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_add(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_sub(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_exchange_n(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return __atomic_compare_exchange_n
        (
            &storage, &expected, desired, false,
            atomics::detail::convert_memory_order_to_gcc(success_order),
            atomics::detail::convert_memory_order_to_gcc(failure_order)
        );
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        return __atomic_compare_exchange_n
        (
            &storage, &expected, desired, true,
            atomics::detail::convert_memory_order_to_gcc(success_order),
            atomics::detail::convert_memory_order_to_gcc(failure_order)
        );
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_and(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_or(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_fetch_xor(&storage, v, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return __atomic_test_and_set(&storage, atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        __atomic_clear(const_cast< storage_type* >(&storage), atomics::detail::convert_memory_order_to_gcc(order));
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile& storage) BOOST_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(storage_type), &storage);
    }
};

#if BOOST_ATOMIC_INT8_LOCK_FREE > 0
template< bool Signed >
struct operations< 1u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 1u, Signed >::type >
{
};
#endif

#if BOOST_ATOMIC_INT16_LOCK_FREE > 0
template< bool Signed >
struct operations< 2u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 2u, Signed >::type >
{
};
#endif

#if BOOST_ATOMIC_INT32_LOCK_FREE > 0
template< bool Signed >
struct operations< 4u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 4u, Signed >::type >
{
};
#endif

#if BOOST_ATOMIC_INT64_LOCK_FREE > 0
#if defined(__clang__) && defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG8B)

// Workaround for clang bug http://llvm.org/bugs/show_bug.cgi?id=19355
template< bool Signed >
struct operations< 8u, Signed > :
    public cas_based_operations< gcc_dcas_x86< Signed > >
{
};

#else

template< bool Signed >
struct operations< 8u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 8u, Signed >::type >
{
};

#endif
#endif

#if BOOST_ATOMIC_INT128_LOCK_FREE > 0
#if defined(__clang__) && defined(BOOST_ATOMIC_DETAIL_X86_HAS_CMPXCHG16B)

// Workaround for clang bug: http://llvm.org/bugs/show_bug.cgi?id=19149
// Clang 3.4 does not implement 128-bit __atomic* intrinsics even though it defines __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
template< bool Signed >
struct operations< 16u, Signed > :
    public cas_based_operations< gcc_dcas_x86_64< Signed > >
{
};

#else

template< bool Signed >
struct operations< 16u, Signed > :
    public gcc_atomic_operations< typename make_storage_type< 16u, Signed >::type >
{
};

#endif
#endif

BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    __atomic_thread_fence(atomics::detail::convert_memory_order_to_gcc(order));
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    __atomic_signal_fence(atomics::detail::convert_memory_order_to_gcc(order));
}

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_ATOMIC_HPP_INCLUDED_
