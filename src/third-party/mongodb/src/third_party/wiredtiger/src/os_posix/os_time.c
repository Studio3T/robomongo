/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * __wt_seconds --
 *	Return the seconds since the Epoch.
 */
int
__wt_seconds(WT_SESSION_IMPL *session, time_t *timep)
{
	struct timespec t;

	WT_RET(__wt_epoch(session, &t));

	*timep = t.tv_sec;

	return (0);
}

/*
 * __wt_epoch --
 *	Return the time since the Epoch.
 */
int
__wt_epoch(WT_SESSION_IMPL *session, struct timespec *tsp)
{
	WT_DECL_RET;

#if defined(HAVE_CLOCK_GETTIME)
	WT_SYSCALL_RETRY(clock_gettime(CLOCK_REALTIME, tsp), ret);
	if (ret == 0)
		return (0);
	WT_RET_MSG(session, ret, "clock_gettime");
#elif defined(HAVE_GETTIMEOFDAY)
	struct timeval v;

	WT_SYSCALL_RETRY(gettimeofday(&v, NULL), ret);
	if (ret == 0) {
		tsp->tv_sec = v.tv_sec;
		tsp->tv_nsec = v.tv_usec * 1000;
		return (0);
	}
	WT_RET_MSG(session, ret, "gettimeofday");
#else
	NO TIME-OF-DAY IMPLEMENTATION: see src/os_posix/os_time.c
#endif
}
