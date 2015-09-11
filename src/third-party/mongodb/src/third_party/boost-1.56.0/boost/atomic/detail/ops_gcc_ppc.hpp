/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2009 Helge Bahmann
 * Copyright (c) 2013 Tim Blechmann
 * Copyright (c) 2014 Andrey Semashev
 */
/*!
 * \file   atomic/detail/ops_gcc_ppc.hpp
 *
 * This header contains implementation of the \c operations template.
 */

#ifndef BOOST_ATOMIC_DETAIL_OPS_GCC_PPC_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_OPS_GCC_PPC_HPP_INCLUDED_

#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/storage_type.hpp>
#include <boost/atomic/detail/operations_fwd.hpp>
#include <boost/atomic/capabilities.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

/*
    Refer to: Motorola: "Programming Environments Manual for 32-Bit
    Implementations of the PowerPC Architecture", Appendix E:
    "Synchronization Programming Examples" for an explanation of what is
    going on here (can be found on the web at various places by the
    name "MPCFPE32B.pdf", Google is your friend...)

    Most of the atomic operations map to instructions in a relatively
    straight-forward fashion, but "load"s may at first glance appear
    a bit strange as they map to:

            lwz %rX, addr
            cmpw %rX, %rX
            bne- 1f
        1:

    That is, the CPU is forced to perform a branch that "formally" depends
    on the value retrieved from memory. This scheme has an overhead of
    about 1-2 clock cycles per load, but it allows to map "acquire" to
    the "isync" instruction instead of "sync" uniformly and for all type
    of atomic operations. Since "isync" has a cost of about 15 clock
    cycles, while "sync" hast a cost of about 50 clock cycles, the small
    penalty to atomic loads more than compensates for this.

    Byte- and halfword-sized atomic values are realized by encoding the
    value to be represented into a word, performing sign/zero extension
    as appropriate. This means that after add/sub operations the value
    needs fixing up to accurately preserve the wrap-around semantic of
    the smaller type. (Nothing special needs to be done for the bit-wise
    and the "exchange type" operators as the compiler already sees to
    it that values carried in registers are extended appropriately and
    everything falls into place naturally).

    The register constraint "b"  instructs gcc to use any register
    except r0; this is sometimes required because the encoding for
    r0 is used to signify "constant zero" in a number of instructions,
    making r0 unusable in this place. For simplicity this constraint
    is used everywhere since I am to lazy to look this up on a
    per-instruction basis, and ppc has enough registers for this not
    to pose a problem.
*/

// A note about memory_order_consume. Technically, this architecture allows to avoid
// unnecessary memory barrier after consume load since it supports data dependency ordering.
// However, some compiler optimizations may break a seemingly valid code relying on data
// dependency tracking by injecting bogus branches to aid out of order execution.
// This may happen not only in Boost.Atomic code but also in user's code, which we have no
// control of. See this thread: http://lists.boost.org/Archives/boost/2014/06/213890.php.
// For this reason we promote memory_order_consume to memory_order_acquire.

struct gcc_ppc_operations_base
{
    static BOOST_FORCEINLINE void fence_before(memory_order order) BOOST_NOEXCEPT
    {
#if defined(__powerpc64__)
        if (order == memory_order_seq_cst)
            __asm__ __volatile__ ("sync" ::: "memory");
        else if ((order & memory_order_release) != 0)
            __asm__ __volatile__ ("lwsync" ::: "memory");
#else
        if ((order & memory_order_release) != 0)
            __asm__ __volatile__ ("sync" ::: "memory");
#endif
    }

    static BOOST_FORCEINLINE void fence_after(memory_order order) BOOST_NOEXCEPT
    {
        if ((order & (memory_order_consume | memory_order_acquire)) != 0)
            __asm__ __volatile__ ("isync" ::: "memory");
    }

    static BOOST_FORCEINLINE void fence_after_store(memory_order order) BOOST_NOEXCEPT
    {
        if (order == memory_order_seq_cst)
            __asm__ __volatile__ ("sync" ::: "memory");
    }
};


template< bool Signed >
struct operations< 4u, Signed > :
    public gcc_ppc_operations_base
{
    typedef typename make_storage_type< 4u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        __asm__ __volatile__
        (
            "stw %1, %0\n"
            : "+m" (storage)
            : "r" (v)
        );
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v;
        __asm__ __volatile__
        (
            "lwz %0, %1\n"
            "cmpw %0, %0\n"
            "bne- 1f\n"
            "1:\n"
            : "=&r" (v)
            : "m" (storage)
            : "cr0"
        );
        fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y1\n"
            "stwcx. %2,%y1\n"
            "bne- 1b\n"
            : "=&b" (original), "+Z" (storage)
            : "b" (v)
            : "cr0"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        int success;
        fence_before(success_order);
        __asm__ __volatile__
        (
            "li %1, 0\n"
            "lwarx %0,%y2\n"
            "cmpw %0, %3\n"
            "bne- 1f\n"
            "stwcx. %4,%y2\n"
            "bne- 1f\n"
            "li %1, 1\n"
            "1:"
            : "=&b" (expected), "=&b" (success), "+Z" (storage)
            : "b" (expected), "b" (desired)
            : "cr0"
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        return !!success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        int success;
        fence_before(success_order);
        __asm__ __volatile__
        (
            "li %1, 0\n"
            "0: lwarx %0,%y2\n"
            "cmpw %0, %3\n"
            "bne- 1f\n"
            "stwcx. %4,%y2\n"
            "bne- 0b\n"
            "li %1, 1\n"
            "1:"
            : "=&b" (expected), "=&b" (success), "+Z" (storage)
            : "b" (expected), "b" (desired)
            : "cr0"
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        return !!success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "add %1,%0,%3\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "sub %1,%0,%3\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "and %1,%0,%3\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "or %1,%0,%3\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "xor %1,%0,%3\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, 0, order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};


template< >
struct operations< 1u, false > :
    public operations< 4u, false >
{
    typedef operations< 4u, false > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "add %1,%0,%3\n"
            "rlwinm %1, %1, 0, 0xff\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "sub %1,%0,%3\n"
            "rlwinm %1, %1, 0, 0xff\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }
};

template< >
struct operations< 1u, true > :
    public operations< 4u, true >
{
    typedef operations< 4u, true > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "add %1,%0,%3\n"
            "extsb %1, %1\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "sub %1,%0,%3\n"
            "extsb %1, %1\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }
};


template< >
struct operations< 2u, false > :
    public operations< 4u, false >
{
    typedef operations< 4u, false > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "add %1,%0,%3\n"
            "rlwinm %1, %1, 0, 0xffff\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "sub %1,%0,%3\n"
            "rlwinm %1, %1, 0, 0xffff\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }
};

template< >
struct operations< 2u, true > :
    public operations< 4u, true >
{
    typedef operations< 4u, true > base_type;
    typedef base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "add %1,%0,%3\n"
            "extsh %1, %1\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "lwarx %0,%y2\n"
            "sub %1,%0,%3\n"
            "extsh %1, %1\n"
            "stwcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }
};


#if defined(__powerpc64__)

template< bool Signed >
struct operations< 8u, Signed > :
    public gcc_ppc_operations_base
{
    typedef typename make_storage_type< 8u, Signed >::type storage_type;

    static BOOST_FORCEINLINE void store(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        fence_before(order);
        __asm__ __volatile__
        (
            "std %1, %0\n"
            : "+m" (storage)
            : "r" (v)
        );
        fence_after_store(order);
    }

    static BOOST_FORCEINLINE storage_type load(storage_type const volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        storage_type v;
        __asm__ __volatile__
        (
            "ld %0, %1\n"
            "cmpd %0, %0\n"
            "bne- 1f\n"
            "1:\n"
            : "=&b" (v)
            : "m" (storage)
            : "cr0"
        );
        fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type exchange(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "ldarx %0,%y1\n"
            "stdcx. %2,%y1\n"
            "bne- 1b\n"
            : "=&b" (original), "+Z" (storage)
            : "b" (v)
            : "cr0"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool compare_exchange_weak(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        int success;
        fence_before(success_order);
        __asm__ __volatile__
        (
            "li %1, 0\n"
            "ldarx %0,%y2\n"
            "cmpd %0, %3\n"
            "bne- 1f\n"
            "stdcx. %4,%y2\n"
            "bne- 1f\n"
            "li %1, 1\n"
            "1:"
            : "=&b" (expected), "=&b" (success), "+Z" (storage)
            : "b" (expected), "b" (desired)
            : "cr0"
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        return !!success;
    }

    static BOOST_FORCEINLINE bool compare_exchange_strong(
        storage_type volatile& storage, storage_type& expected, storage_type desired, memory_order success_order, memory_order failure_order) BOOST_NOEXCEPT
    {
        int success;
        fence_before(success_order);
        __asm__ __volatile__
        (
            "li %1, 0\n"
            "0: ldarx %0,%y2\n"
            "cmpd %0, %3\n"
            "bne- 1f\n"
            "stdcx. %4,%y2\n"
            "bne- 0b\n"
            "li %1, 1\n"
            "1:"
            : "=&b" (expected), "=&b" (success), "+Z" (storage)
            : "b" (expected), "b" (desired)
            : "cr0"
        );
        if (success)
            fence_after(success_order);
        else
            fence_after(failure_order);
        return !!success;
    }

    static BOOST_FORCEINLINE storage_type fetch_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "ldarx %0,%y2\n"
            "add %1,%0,%3\n"
            "stdcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "ldarx %0,%y2\n"
            "sub %1,%0,%3\n"
            "stdcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "ldarx %0,%y2\n"
            "and %1,%0,%3\n"
            "stdcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "ldarx %0,%y2\n"
            "or %1,%0,%3\n"
            "stdcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE storage_type fetch_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        storage_type original, tmp;
        fence_before(order);
        __asm__ __volatile__
        (
            "1:\n"
            "ldarx %0,%y2\n"
            "xor %1,%0,%3\n"
            "stdcx. %1,%y2\n"
            "bne- 1b\n"
            : "=&b" (original), "=&b" (tmp), "+Z" (storage)
            : "b" (v)
            : "cc"
        );
        fence_after(order);
        return original;
    }

    static BOOST_FORCEINLINE bool test_and_set(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        return !!exchange(storage, (storage_type)1, order);
    }

    static BOOST_FORCEINLINE void clear(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        store(storage, 0, order);
    }

    static BOOST_FORCEINLINE bool is_lock_free(storage_type const volatile&) BOOST_NOEXCEPT
    {
        return true;
    }
};

#endif // defined(__powerpc64__)


BOOST_FORCEINLINE void thread_fence(memory_order order) BOOST_NOEXCEPT
{
    switch (order)
    {
    case memory_order_consume:
    case memory_order_acquire:
        __asm__ __volatile__ ("isync" ::: "memory");
        break;
    case memory_order_release:
#if defined(__powerpc64__)
        __asm__ __volatile__ ("lwsync" ::: "memory");
        break;
#endif
    case memory_order_acq_rel:
    case memory_order_seq_cst:
        __asm__ __volatile__ ("sync" ::: "memory");
        break;
    default:;
    }
}

BOOST_FORCEINLINE void signal_fence(memory_order order) BOOST_NOEXCEPT
{
    if (order != memory_order_relaxed)
        __asm__ __volatile__ ("" ::: "memory");
}

} // namespace detail
} // namespace atomics
} // namespace boost

#endif // BOOST_ATOMIC_DETAIL_OPS_GCC_PPC_HPP_INCLUDED_
