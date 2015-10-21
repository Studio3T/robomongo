/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * __wt_thread_create --
 *	Create a new thread of control.
 */
int
__wt_thread_create(WT_SESSION_IMPL *session,
    wt_thread_t *tidret, WT_THREAD_CALLBACK(*func)(void *), void *arg)
{
	/* Spawn a new thread of control. */
	*tidret = (HANDLE)_beginthreadex(NULL, 0, func, arg, 0, NULL);
	if (*tidret != 0)
		return (0);

	WT_RET_MSG(session, errno, "_beginthreadex");
}

/*
 * __wt_thread_join --
 *	Wait for a thread of control to exit.
 */
int
__wt_thread_join(WT_SESSION_IMPL *session, wt_thread_t tid)
{
	WT_DECL_RET;

	if ((ret = WaitForSingleObject(tid, INFINITE)) != WAIT_OBJECT_0)
		/*
		 * If we fail to wait, we will leak handles so do not continue
		 */
		WT_PANIC_RET(session, ret == WAIT_FAILED ? __wt_errno() : ret,
		    "Wait for thread join failed");

	if (CloseHandle(tid) == 0) {
		WT_RET_MSG(session, __wt_errno(),
		    "CloseHandle: thread join");
	}

	return (0);
}

/*
 * __wt_thread_id --
 *	Fill in a printable version of the process and thread IDs.
 */
void
__wt_thread_id(char* buf, size_t buflen)
{
	(void)snprintf(buf, buflen,
	    "%" PRIu64 ":%" PRIu64,
	    (uint64_t)GetCurrentProcessId(), (uint64_t)GetCurrentThreadId);
}
