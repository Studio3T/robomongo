/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

#define	WT_DHANDLE_CAN_DISCARD(dhandle)					\
	(!F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE | WT_DHANDLE_OPEN) &&	\
	dhandle->session_inuse == 0 && dhandle->session_ref == 0)

/*
 * __sweep_mark --
 *	Mark idle handles with a time of death, and note if we see dead
 *	handles.
 */
static int
__sweep_mark(WT_SESSION_IMPL *session, time_t now)
{
	WT_CONNECTION_IMPL *conn;
	WT_DATA_HANDLE *dhandle;

	conn = S2C(session);

	TAILQ_FOREACH(dhandle, &conn->dhqh, q) {
		if (WT_IS_METADATA(dhandle))
			continue;

		/*
		 * There are some internal increments of the in-use count such
		 * as eviction.  Don't keep handles alive because of those
		 * cases, but if we see multiple cursors open, clear the time
		 * of death.
		 */
		if (dhandle->session_inuse > 1)
			dhandle->timeofdeath = 0;

		/*
		 * If the handle is open exclusive or currently in use, or the
		 * time of death is already set, move on.
		 */
		if (F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE) ||
		    dhandle->session_inuse > 0 ||
		    dhandle->timeofdeath != 0)
			continue;

		dhandle->timeofdeath = now;
		WT_STAT_FAST_CONN_INCR(session, dh_sweep_tod);
	}

	return (0);
}

/*
 * __sweep_expire_one --
 *	Mark a single handle dead.
 */
static int
__sweep_expire_one(WT_SESSION_IMPL *session)
{
	WT_BTREE *btree;
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	int evict_reset;

	btree = S2BT(session);
	dhandle = session->dhandle;
	evict_reset = 0;

	/*
	 * Acquire an exclusive lock on the handle and mark it dead.
	 *
	 * The close would require I/O if an update cannot be written
	 * (updates in a no-longer-referenced file might not yet be
	 * globally visible if sessions have disjoint sets of files
	 * open).  In that case, skip it: we'll retry the close the
	 * next time, after the transaction state has progressed.
	 *
	 * We don't set WT_DHANDLE_EXCLUSIVE deliberately, we want
	 * opens to block on us and then retry rather than returning an
	 * EBUSY error to the application.  This is done holding the
	 * handle list lock so that connection-level handle searches
	 * never need to retry.
	 */
	WT_RET(__wt_try_writelock(session, dhandle->rwlock));

	/* Only sweep clean trees where all updates are visible. */
	if (btree->modified ||
	    !__wt_txn_visible_all(session, btree->rec_max_txn))
		goto err;

	/* Ensure that we aren't racing with the eviction server */
	WT_ERR(__wt_evict_file_exclusive_on(session, &evict_reset));

	/*
	 * Mark the handle as dead and close the underlying file
	 * handle. Closing the handle decrements the open file count,
	 * meaning the close loop won't overrun the configured minimum.
	 */
	ret = __wt_conn_btree_sync_and_close(session, 0, 1);

	if (evict_reset)
		__wt_evict_file_exclusive_off(session);

err:	WT_TRET(__wt_writeunlock(session, dhandle->rwlock));

	return (ret);
}

/*
 * __sweep_expire --
 *	Mark trees dead if they are clean and haven't been accessed recently,
 *	until we have reached the configured minimum number of handles.
 */
static int
__sweep_expire(WT_SESSION_IMPL *session, time_t now)
{
	WT_CONNECTION_IMPL *conn;
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;

	conn = S2C(session);

	TAILQ_FOREACH(dhandle, &conn->dhqh, q) {
		/*
		 * Ignore open files once the btree file count is below the
		 * minimum number of handles.
		 */
		if (conn->open_btree_count < conn->sweep_handles_min)
			break;

		if (WT_IS_METADATA(dhandle) ||
		    !F_ISSET(dhandle, WT_DHANDLE_OPEN) ||
		    dhandle->session_inuse != 0 ||
		    dhandle->timeofdeath == 0 ||
		    now <= dhandle->timeofdeath + conn->sweep_idle_time)
			continue;

		WT_WITH_DHANDLE_LOCK(session,
		    WT_WITH_DHANDLE(session, dhandle,
			ret = __sweep_expire_one(session)));
		WT_RET_BUSY_OK(ret);
	}

	return (0);
}

/*
 * __sweep_discard_trees --
 *	Discard pages from dead trees.
 */
static int
__sweep_discard_trees(WT_SESSION_IMPL *session, u_int *dead_handlesp)
{
	WT_CONNECTION_IMPL *conn;
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;

	conn = S2C(session);

	*dead_handlesp = 0;

	TAILQ_FOREACH(dhandle, &conn->dhqh, q) {
		if (WT_DHANDLE_CAN_DISCARD(dhandle))
			++*dead_handlesp;

		if (!F_ISSET(dhandle, WT_DHANDLE_OPEN) ||
		    !F_ISSET(dhandle, WT_DHANDLE_DEAD))
			continue;

		/* If the handle is marked "dead", flush it from cache. */
		WT_WITH_DHANDLE(session, dhandle, ret =
		    __wt_conn_btree_sync_and_close(session, 0, 0));

		/* We closed the btree handle. */
		if (ret == 0) {
			WT_STAT_FAST_CONN_INCR(session, dh_sweep_close);
			++*dead_handlesp;
		} else
			WT_STAT_FAST_CONN_INCR(session, dh_sweep_ref);

		WT_RET_BUSY_OK(ret);
	}

	return (0);
}

/*
 * __sweep_remove_one --
 *	Remove a closed handle from the connection list.
 */
static int
__sweep_remove_one(WT_SESSION_IMPL *session, WT_DATA_HANDLE *dhandle)
{
	WT_DECL_RET;

	/* Try to get exclusive access. */
	WT_RET(__wt_try_writelock(session, dhandle->rwlock));

	/*
	 * If there are no longer any references to the handle in any
	 * sessions, attempt to discard it.
	 */
	if (!WT_DHANDLE_CAN_DISCARD(dhandle))
		WT_ERR(EBUSY);

	WT_WITH_DHANDLE(session, dhandle,
	    ret = __wt_conn_dhandle_discard_single(session, 0, 1));

	/*
	 * If the handle was not successfully discarded, unlock it and
	 * don't retry the discard until it times out again.
	 */
	if (ret != 0) {
err:		WT_TRET(__wt_writeunlock(session, dhandle->rwlock));
	}

	return (ret);
}

/*
 * __sweep_remove_handles --
 *	Remove closed handles from the connection list.
 */
static int
__sweep_remove_handles(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	WT_DATA_HANDLE *dhandle, *dhandle_next;
	WT_DECL_RET;

	conn = S2C(session);

	for (dhandle = TAILQ_FIRST(&conn->dhqh);
	    dhandle != NULL;
	    dhandle = dhandle_next) {
		dhandle_next = TAILQ_NEXT(dhandle, q);
		if (WT_IS_METADATA(dhandle))
			continue;
		if (!WT_DHANDLE_CAN_DISCARD(dhandle))
			continue;

		WT_WITH_DHANDLE_LOCK(session,
		    ret = __sweep_remove_one(session, dhandle));
		if (ret == 0)
			WT_STAT_FAST_CONN_INCR(session, dh_sweep_remove);
		else
			WT_STAT_FAST_CONN_INCR(session, dh_sweep_ref);
		WT_RET_BUSY_OK(ret);
	}

	return (ret == EBUSY ? 0 : ret);
}

/*
 * __sweep_server --
 *	The handle sweep server thread.
 */
static WT_THREAD_RET
__sweep_server(void *arg)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;
	time_t now;
	u_int dead_handles;

	session = arg;
	conn = S2C(session);

	/*
	 * Sweep for dead and excess handles.
	 */
	while (F_ISSET(conn, WT_CONN_SERVER_RUN) &&
	    F_ISSET(conn, WT_CONN_SERVER_SWEEP)) {
		/* Wait until the next event. */
		WT_ERR(__wt_cond_wait(session, conn->sweep_cond,
		    (uint64_t)conn->sweep_interval * WT_MILLION));
		WT_ERR(__wt_seconds(session, &now));

		WT_STAT_FAST_CONN_INCR(session, dh_sweeps);

		/*
		 * Mark handles with a time of death, and report whether any
		 * handles are marked dead.  If sweep_idle_time is 0, handles
		 * never become idle.
		 */
		if (conn->sweep_idle_time != 0)
			WT_ERR(__sweep_mark(session, now));

		/*
		 * Close handles if we have reached the configured limit.
		 * If sweep_idle_time is 0, handles never become idle.
		 */
		if (conn->sweep_idle_time != 0 &&
		    conn->open_btree_count >= conn->sweep_handles_min)
			WT_ERR(__sweep_expire(session, now));

		WT_ERR(__sweep_discard_trees(session, &dead_handles));

		if (dead_handles > 0)
			WT_ERR(__sweep_remove_handles(session));
	}

	if (0) {
err:		WT_PANIC_MSG(session, ret, "handle sweep server error");
	}
	return (WT_THREAD_RET_VALUE);
}

/*
 * __wt_sweep_config --
 *	Pull out sweep configuration settings
 */
int
__wt_sweep_config(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_CONFIG_ITEM cval;
	WT_CONNECTION_IMPL *conn;

	conn = S2C(session);

	/* Pull out the sweep configurations. */
	WT_RET(__wt_config_gets(session,
	    cfg, "file_manager.close_idle_time", &cval));
	conn->sweep_idle_time = (time_t)cval.val;

	WT_RET(__wt_config_gets(session,
	    cfg, "file_manager.close_scan_interval", &cval));
	conn->sweep_interval = (time_t)cval.val;

	WT_RET(__wt_config_gets(session,
	    cfg, "file_manager.close_handle_minimum", &cval));
	conn->sweep_handles_min = (u_int)cval.val;

	return (0);
}

/*
 * __wt_sweep_create --
 *	Start the handle sweep thread.
 */
int
__wt_sweep_create(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;

	conn = S2C(session);

	/* Set first, the thread might run before we finish up. */
	F_SET(conn, WT_CONN_SERVER_SWEEP);

	WT_RET(__wt_open_internal_session(
	    conn, "sweep-server", 1, 1, &conn->sweep_session));
	session = conn->sweep_session;

	/*
	 * Handle sweep does enough I/O it may be called upon to perform slow
	 * operations for the block manager.
	 */
	F_SET(session, WT_SESSION_CAN_WAIT);

	WT_RET(__wt_cond_alloc(
	    session, "handle sweep server", 0, &conn->sweep_cond));

	WT_RET(__wt_thread_create(
	    session, &conn->sweep_tid, __sweep_server, session));
	conn->sweep_tid_set = 1;

	return (0);
}

/*
 * __wt_sweep_destroy --
 *	Destroy the handle-sweep thread.
 */
int
__wt_sweep_destroy(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	WT_SESSION *wt_session;

	conn = S2C(session);

	F_CLR(conn, WT_CONN_SERVER_SWEEP);
	if (conn->sweep_tid_set) {
		WT_TRET(__wt_cond_signal(session, conn->sweep_cond));
		WT_TRET(__wt_thread_join(session, conn->sweep_tid));
		conn->sweep_tid_set = 0;
	}
	WT_TRET(__wt_cond_destroy(session, &conn->sweep_cond));

	if (conn->sweep_session != NULL) {
		wt_session = &conn->sweep_session->iface;
		WT_TRET(wt_session->close(wt_session, NULL));

		conn->sweep_session = NULL;
	}
	return (ret);
}
