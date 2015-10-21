/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * __wt_errno --
 *	Return errno, or WT_ERROR if errno not set.
 */
int
__wt_errno(void)
{
	/*
	 * Called when we know an error occurred, and we want the system
	 * error code, but there's some chance it's not set.
	 */
	return (errno == 0 ? WT_ERROR : errno);
}

/*
 * __wt_strerror --
 *	POSIX implementation of WT_SESSION.strerror and wiredtiger_strerror.
 */
const char *
__wt_strerror(WT_SESSION_IMPL *session, int error, char *errbuf, size_t errlen)
{
	const char *p;

	/*
	 * Check for a WiredTiger or POSIX constant string, no buffer needed.
	 */
	if ((p = __wt_wiredtiger_error(error)) != NULL)
		return (p);

	/*
	 * When called from wiredtiger_strerror, write a passed-in buffer.
	 * When called from WT_SESSION.strerror, write the session's buffer.
	 *
	 * Fallback to a generic message.
	 */
	if (session == NULL &&
	    snprintf(errbuf, errlen, "error return: %d", error) > 0)
		return (errbuf);
	if (session != NULL && __wt_buf_fmt(
	    session, &session->err, "error return: %d", error) == 0)
		return (session->err.data);

	/* Defeated. */
	return ("Unable to return error string");
}
