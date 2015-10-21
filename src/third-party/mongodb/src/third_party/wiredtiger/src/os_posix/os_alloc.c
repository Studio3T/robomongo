/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * On systems with poor default allocators for allocations greater than 16 KB,
 * we provide an option to use TCMalloc explicitly.
 * This is important on Windows which does not have a builtin mechanism
 * to replace C run-time memory management functions with alternatives.
 */
#ifdef HAVE_LIBTCMALLOC
#include <gperftools/tcmalloc.h>

/*
 * Define HAVE_POSIX_MEMALIGN explicitly since TCMalloc supports it
 */
#define	HAVE_POSIX_MEMALIGN 1

#define	calloc			tc_calloc
#define	realloc 		tc_realloc
#define	posix_memalign 		tc_posix_memalign
#define	free 			tc_free

#endif

/*
 * There's no malloc interface, WiredTiger never calls malloc.
 *
 * The problem is an application might allocate memory, write secret stuff in
 * it, free the memory, then WiredTiger allocates the memory and uses it for a
 * file page or log record, then writes it to disk, without having overwritten
 * it fully.  That results in the secret stuff being protected by WiredTiger's
 * permission mechanisms, potentially inappropriate for the secret stuff.
 */

/*
 * __wt_calloc --
 *	ANSI calloc function.
 */
int
__wt_calloc(WT_SESSION_IMPL *session, size_t number, size_t size, void *retp)
{
	void *p;

	/*
	 * !!!
	 * This function MUST handle a NULL WT_SESSION_IMPL handle.
	 */
	WT_ASSERT(session, number != 0 && size != 0);

	if (session != NULL)
		WT_STAT_FAST_CONN_INCR(session, memory_allocation);

	if ((p = calloc(number, size)) == NULL)
		WT_RET_MSG(session, __wt_errno(), "memory allocation");

	*(void **)retp = p;
	return (0);
}

/*
 * __wt_realloc --
 *	ANSI realloc function.
 */
int
__wt_realloc(WT_SESSION_IMPL *session,
    size_t *bytes_allocated_ret, size_t bytes_to_allocate, void *retp)
{
	void *p;
	size_t bytes_allocated;

	/*
	 * !!!
	 * This function MUST handle a NULL WT_SESSION_IMPL handle.
	 *
	 * Sometimes we're allocating memory and we don't care about the
	 * final length -- bytes_allocated_ret may be NULL.
	 */
	p = *(void **)retp;
	bytes_allocated =
	    (bytes_allocated_ret == NULL) ? 0 : *bytes_allocated_ret;
	WT_ASSERT(session,
	    (p == NULL && bytes_allocated == 0) ||
	    (p != NULL &&
	    (bytes_allocated_ret == NULL || bytes_allocated != 0)));
	WT_ASSERT(session, bytes_to_allocate != 0);
	WT_ASSERT(session, bytes_allocated < bytes_to_allocate);

	if (session != NULL) {
		if (p == NULL)
			WT_STAT_FAST_CONN_INCR(session, memory_allocation);
		else
			WT_STAT_FAST_CONN_INCR(session, memory_grow);
	}

	if ((p = realloc(p, bytes_to_allocate)) == NULL)
		WT_RET_MSG(session, __wt_errno(), "memory allocation");

	/*
	 * Clear the allocated memory -- an application might: allocate memory,
	 * write secret stuff into it, free the memory, then we re-allocate the
	 * memory and use it for a file page or log record, and then write it to
	 * disk.  That would result in the secret stuff being protected by the
	 * WiredTiger permission mechanisms, potentially inappropriate for the
	 * secret stuff.
	 */
	memset((uint8_t *)
	    p + bytes_allocated, 0, bytes_to_allocate - bytes_allocated);

	/* Update caller's bytes allocated value. */
	if (bytes_allocated_ret != NULL)
		*bytes_allocated_ret = bytes_to_allocate;

	*(void **)retp = p;
	return (0);
}

/*
 * __wt_realloc_aligned --
 *	ANSI realloc function that aligns to buffer boundaries, configured with
 *	the "buffer_alignment" key to wiredtiger_open.
 */
int
__wt_realloc_aligned(WT_SESSION_IMPL *session,
    size_t *bytes_allocated_ret, size_t bytes_to_allocate, void *retp)
{
#if defined(HAVE_POSIX_MEMALIGN)
	WT_DECL_RET;

	/*
	 * !!!
	 * This function MUST handle a NULL WT_SESSION_IMPL handle.
	 */
	if (session != NULL && S2C(session)->buffer_alignment > 0) {
		void *p, *newp;
		size_t bytes_allocated;

		/*
		 * Sometimes we're allocating memory and we don't care about the
		 * final length -- bytes_allocated_ret may be NULL.
		 */
		p = *(void **)retp;
		bytes_allocated =
		    (bytes_allocated_ret == NULL) ? 0 : *bytes_allocated_ret;
		WT_ASSERT(session,
		    (p == NULL && bytes_allocated == 0) ||
		    (p != NULL &&
		    (bytes_allocated_ret == NULL || bytes_allocated != 0)));
		WT_ASSERT(session, bytes_to_allocate != 0);
		WT_ASSERT(session, bytes_allocated < bytes_to_allocate);

		/*
		 * We are going to allocate an aligned buffer.  When we do this
		 * repeatedly, the allocator is expected to start on a boundary
		 * each time, account for that additional space by never asking
		 * for less than a full alignment size.  The primary use case
		 * for aligned buffers is Linux direct I/O, which requires that
		 * the size be a multiple of the alignment anyway.
		 */
		bytes_to_allocate =
		    WT_ALIGN(bytes_to_allocate, S2C(session)->buffer_alignment);

		WT_STAT_FAST_CONN_INCR(session, memory_allocation);

		if ((ret = posix_memalign(&newp,
		    S2C(session)->buffer_alignment,
		    bytes_to_allocate)) != 0)
			WT_RET_MSG(session, ret, "memory allocation");

		if (p != NULL)
			memcpy(newp, p, bytes_allocated);
		__wt_free(session, p);
		p = newp;

		/* Clear the allocated memory (see above). */
		memset((uint8_t *)p + bytes_allocated, 0,
		    bytes_to_allocate - bytes_allocated);

		/* Update caller's bytes allocated value. */
		if (bytes_allocated_ret != NULL)
			*bytes_allocated_ret = bytes_to_allocate;

		*(void **)retp = p;
		return (0);
	}
#endif
	/*
	 * If there is no posix_memalign function, or no alignment configured,
	 * fall back to realloc.
	 *
	 * Windows note: Visual C CRT memalign does not match Posix behavior
	 * and would also double each allocation so it is bad for memory use
	 */
	return (__wt_realloc(
	    session, bytes_allocated_ret, bytes_to_allocate, retp));
}

/*
 * __wt_strndup --
 *	Duplicate a byte string of a given length (and NUL-terminate).
 */
int
__wt_strndup(WT_SESSION_IMPL *session, const void *str, size_t len, void *retp)
{
	void *p;

	if (str == NULL) {
		*(void **)retp = NULL;
		return (0);
	}

	WT_RET(__wt_calloc(session, len + 1, 1, &p));

	/*
	 * Don't change this to strncpy, we rely on this function to duplicate
	 * "strings" that contain nul bytes.
	 */
	memcpy(p, str, len);

	*(void **)retp = p;
	return (0);
}

/*
 * __wt_strdup --
 *	ANSI strdup function.
 */
int
__wt_strdup(WT_SESSION_IMPL *session, const char *str, void *retp)
{
	return (__wt_strndup(
	    session, str, (str == NULL) ? 0 : strlen(str), retp));
}

/*
 * __wt_free_int --
 *	ANSI free function.
 */
void
__wt_free_int(WT_SESSION_IMPL *session, const void *p_arg)
{
	void *p;

	p = *(void **)p_arg;
	if (p == NULL)				/* ANSI C free semantics */
		return;

	/*
	 * If there's a serialization bug we might race with another thread.
	 * We can't avoid the race (and we aren't willing to flush memory),
	 * but we minimize the window by clearing the free address, hoping a
	 * racing thread will see, and won't free, a NULL pointer.
	 */
	*(void **)p_arg = NULL;

	/*
	 * !!!
	 * This function MUST handle a NULL WT_SESSION_IMPL handle.
	 */
	if (session != NULL)
		WT_STAT_FAST_CONN_INCR(session, memory_free);

	free(p);
}
