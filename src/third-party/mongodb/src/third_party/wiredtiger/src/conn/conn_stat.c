/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

#ifdef __GNUC__
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 1)
/*
 * !!!
 * GCC with -Wformat-nonliteral complains about calls to strftime in this file.
 * There's nothing wrong, this makes the warning go away.
 */
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
#endif

/*
 * __stat_sources_free --
 *	Free the array of statistics sources.
 */
static void
__stat_sources_free(WT_SESSION_IMPL *session, char ***sources)
{
	char **p;

	if ((p = (*sources)) != NULL) {
		for (; *p != NULL; ++p)
			__wt_free(session, *p);
		__wt_free(session, *sources);
	}
}

/*
 * __wt_conn_stat_init --
 *	Initialize the per-connection statistics.
 */
void
__wt_conn_stat_init(WT_SESSION_IMPL *session)
{
	__wt_async_stats_update(session);
	__wt_cache_stats_update(session);
	__wt_txn_stats_update(session);

	WT_CONN_STAT(session, dh_conn_handle_count) =
	    S2C(session)->dhandle_count;
	WT_CONN_STAT(session, file_open) = S2C(session)->open_file_count;
}

/*
 * __statlog_config --
 *	Parse and setup the statistics server options.
 */
static int
__statlog_config(WT_SESSION_IMPL *session, const char **cfg, int *runp)
{
	WT_CONFIG objectconf;
	WT_CONFIG_ITEM cval, k, v;
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	int cnt;
	char **sources;

	conn = S2C(session);
	sources = NULL;

	WT_RET(__wt_config_gets(session, cfg, "statistics_log.wait", &cval));
	/* Only start the server if wait time is non-zero */
	*runp = (cval.val == 0) ? 0 : 1;
	conn->stat_usecs = (uint64_t)cval.val * 1000000;

	WT_RET(__wt_config_gets(
	    session, cfg, "statistics_log.on_close", &cval));
	if (cval.val != 0)
		FLD_SET(conn->stat_flags, WT_CONN_STAT_ON_CLOSE);

	/*
	 * Statistics logging configuration requires either a wait time or an
	 * on-close setting.
	 */
	if (*runp == 0 && !FLD_ISSET(conn->stat_flags, WT_CONN_STAT_ON_CLOSE))
		return (0);

	WT_RET(__wt_config_gets(session, cfg, "statistics_log.sources", &cval));
	WT_RET(__wt_config_subinit(session, &objectconf, &cval));
	for (cnt = 0; (ret = __wt_config_next(&objectconf, &k, &v)) == 0; ++cnt)
		;
	WT_RET_NOTFOUND_OK(ret);
	if (cnt != 0) {
		WT_RET(__wt_calloc_def(session, cnt + 1, &sources));
		WT_RET(__wt_config_subinit(session, &objectconf, &cval));
		for (cnt = 0;
		    (ret = __wt_config_next(&objectconf, &k, &v)) == 0; ++cnt) {
			/*
			 * XXX
			 * Only allow "file:" and "lsm:" for now: "file:" works
			 * because it's been converted to data handles, "lsm:"
			 * works because we can easily walk the list of open LSM
			 * objects, even though it hasn't been converted.
			 */
			if (!WT_PREFIX_MATCH(k.str, "file:") &&
			    !WT_PREFIX_MATCH(k.str, "lsm:"))
				WT_ERR_MSG(session, EINVAL,
				    "statistics_log sources configuration only "
				    "supports objects of type \"file\" or "
				    "\"lsm\"");
			WT_ERR(
			    __wt_strndup(session, k.str, k.len, &sources[cnt]));
		}
		WT_ERR_NOTFOUND_OK(ret);

		conn->stat_sources = sources;
		sources = NULL;
	}

	WT_ERR(__wt_config_gets(session, cfg, "statistics_log.path", &cval));
	WT_ERR(__wt_nfilename(session, cval.str, cval.len, &conn->stat_path));

	WT_ERR(__wt_config_gets(
	    session, cfg, "statistics_log.timestamp", &cval));
	WT_ERR(__wt_strndup(session, cval.str, cval.len, &conn->stat_format));

err:	__stat_sources_free(session, &sources);
	return (ret);
}

/*
 * __statlog_dump --
 *	Dump out handle/connection statistics.
 */
static int
__statlog_dump(WT_SESSION_IMPL *session, const char *name, int conn_stats)
{
	WT_CONNECTION_IMPL *conn;
	WT_CURSOR *cursor;
	WT_DECL_ITEM(tmp);
	WT_DECL_RET;
	WT_STATS *stats;
	u_int i;
	uint64_t max;
	const char *uri;
	const char *cfg[] = {
	    WT_CONFIG_BASE(session, session_open_cursor), NULL };

	conn = S2C(session);

	/* Build URI and configuration string. */
	if (conn_stats)
		uri = "statistics:";
	else {
		WT_RET(__wt_scr_alloc(session, 0, &tmp));
		WT_ERR(__wt_buf_fmt(session, tmp, "statistics:%s", name));
		uri = tmp->data;
	}

	/*
	 * Open the statistics cursor and dump the statistics.
	 *
	 * If we don't find an underlying object, silently ignore it, the object
	 * may exist only intermittently.
	 */
	switch (ret = __wt_curstat_open(session, uri, cfg, &cursor)) {
	case 0:
		max = conn_stats ?
		    sizeof(WT_CONNECTION_STATS) / sizeof(WT_STATS) :
		    sizeof(WT_DSRC_STATS) / sizeof(WT_STATS);
		for (i = 0,
		    stats = WT_CURSOR_STATS(cursor); i <  max; ++i, ++stats)
			WT_ERR(__wt_fprintf(session, conn->stat_fp,
			    "%s %" PRIu64 " %s %s\n",
			    conn->stat_stamp,
			    stats->v, name, stats->desc));
		WT_ERR(cursor->close(cursor));
		break;
	case EBUSY:
	case ENOENT:
	case WT_NOTFOUND:
		ret = 0;
		break;
	default:
		break;
	}

err:	__wt_scr_free(session, &tmp);
	return (ret);
}

/*
 * __statlog_apply --
 *	Review a single open handle and dump statistics on demand.
 */
static int
__statlog_apply(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_DATA_HANDLE *dhandle;
	WT_DECL_RET;
	char **p;

	WT_UNUSED(cfg);

	dhandle = session->dhandle;

	/* Check for a match on the set of sources. */
	for (p = S2C(session)->stat_sources; *p != NULL; ++p)
		if (WT_PREFIX_MATCH(dhandle->name, *p)) {
			WT_WITHOUT_DHANDLE(session,
			    ret = __statlog_dump(session, dhandle->name, 0));
			return (ret);
		}
	return (0);
}

/*
 * __statlog_lsm_apply --
 *	Review the list open LSM trees, and dump statistics on demand.
 *
 * XXX
 * This code should be removed when LSM objects are converted to data handles.
 */
static int
__statlog_lsm_apply(WT_SESSION_IMPL *session)
{
#define	WT_LSM_TREE_LIST_SLOTS	100
	WT_LSM_TREE *lsm_tree, *list[WT_LSM_TREE_LIST_SLOTS];
	WT_DECL_RET;
	int cnt, locked;
	char **p;

	cnt = locked = 0;

	/*
	 * Walk the list of LSM trees, checking for a match on the set of
	 * sources.
	 *
	 * XXX
	 * We can't hold the schema lock for the traversal because the LSM
	 * statistics code acquires the tree lock, and the LSM cursor code
	 * acquires the tree lock and then acquires the schema lock, it's a
	 * classic deadlock.  This is temporary code so I'm not going to do
	 * anything fancy.
	 * It is OK to not keep holding the schema lock after populating
	 * the list of matching LSM trees, since the __wt_lsm_tree_get call
	 * will bump a reference count, so the tree won't go away.
	 */
	__wt_spin_lock(session, &S2C(session)->schema_lock);
	locked = 1;
	TAILQ_FOREACH(lsm_tree, &S2C(session)->lsmqh, q) {
		if (cnt == WT_LSM_TREE_LIST_SLOTS)
			break;
		for (p = S2C(session)->stat_sources; *p != NULL; ++p)
			if (WT_PREFIX_MATCH(lsm_tree->name, *p)) {
				WT_ERR(__wt_lsm_tree_get(
				    session, lsm_tree->name, 0, &list[cnt++]));
				break;
			}
	}
	__wt_spin_unlock(session, &S2C(session)->schema_lock);
	locked = 0;

	while (cnt > 0) {
		--cnt;
		WT_TRET(__statlog_dump(session, list[cnt]->name, 0));
		__wt_lsm_tree_release(session, list[cnt]);
	}

err:	if (locked)
		__wt_spin_unlock(session, &S2C(session)->schema_lock);
	/* Release any LSM trees on error. */
	while (cnt > 0) {
		--cnt;
		__wt_lsm_tree_release(session, list[cnt]);
	}
	return (ret);
}

/*
 * __statlog_log_one --
 *	Output a set of statistics into the current log file.
 */
static int
__statlog_log_one(WT_SESSION_IMPL *session, WT_ITEM *path, WT_ITEM *tmp)
{
	FILE *log_file;
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	struct timespec ts;
	struct tm *tm, _tm;

	conn = S2C(session);

	/* Get the current local time of day. */
	WT_RET(__wt_epoch(session, &ts));
	tm = localtime_r(&ts.tv_sec, &_tm);

	/* Create the logging path name for this time of day. */
	if (strftime(tmp->mem, tmp->memsize, conn->stat_path, tm) == 0)
		WT_RET_MSG(session, ENOMEM, "strftime path conversion");

	/* If the path has changed, cycle the log file. */
	if ((log_file = conn->stat_fp) == NULL ||
	    path == NULL || strcmp(tmp->mem, path->mem) != 0) {
		conn->stat_fp = NULL;
		WT_RET(__wt_fclose(session, &log_file, WT_FHANDLE_APPEND));
		if (path != NULL)
			(void)strcpy(path->mem, tmp->mem);
		WT_RET(__wt_fopen(session,
		    tmp->mem, WT_FHANDLE_APPEND, WT_FOPEN_FIXED, &log_file));
	}
	conn->stat_fp = log_file;

	/* Create the entry prefix for this time of day. */
	if (strftime(tmp->mem, tmp->memsize, conn->stat_format, tm) == 0)
		WT_RET_MSG(session, ENOMEM, "strftime timestamp conversion");
	conn->stat_stamp = tmp->mem;

	/* Dump the connection statistics. */
	WT_RET(__statlog_dump(session, conn->home, 1));

#if SPINLOCK_TYPE == SPINLOCK_PTHREAD_MUTEX_LOGGING
	/* Dump the spinlock statistics. */
	WT_RET(__wt_statlog_dump_spinlock(conn, conn->home));
#endif

	/*
	 * Lock the schema and walk the list of open handles, dumping
	 * any that match the list of object sources.
	 */
	if (conn->stat_sources != NULL) {
		WT_WITH_DHANDLE_LOCK(session, ret =
		    __wt_conn_btree_apply(
		    session, 0, NULL, __statlog_apply, NULL));
		WT_RET(ret);
	}

	/*
	 * Walk the list of open LSM trees, dumping any that match the
	 * the list of object sources.
	 *
	 * XXX
	 * This code should be removed when LSM objects are converted to
	 * data handles.
	 */
	if (conn->stat_sources != NULL)
		WT_RET(__statlog_lsm_apply(session));

	/* Flush. */
	return (__wt_fflush(session, conn->stat_fp));
}

/*
 * __wt_statlog_log_one --
 *	Log a set of statistics into the configured statistics log. Requires
 *	that the server is not currently running.
 */
int
__wt_statlog_log_one(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	WT_DECL_ITEM(tmp);

	conn = S2C(session);

	if (!FLD_ISSET(conn->stat_flags, WT_CONN_STAT_ON_CLOSE))
		return (0);

	if (F_ISSET(conn, WT_CONN_SERVER_RUN) &&
	    F_ISSET(conn, WT_CONN_SERVER_STATISTICS))
		WT_RET_MSG(session, EINVAL,
		    "Attempt to log statistics while a server is running");

	WT_RET(__wt_scr_alloc(session, strlen(conn->stat_path) + 128, &tmp));
	WT_ERR(__statlog_log_one(session, NULL, tmp));

err:	__wt_scr_free(session, &tmp);
	return (ret);
}

/*
 * __statlog_server --
 *	The statistics server thread.
 */
static WT_THREAD_RET
__statlog_server(void *arg)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	WT_ITEM path, tmp;
	WT_SESSION_IMPL *session;

	session = arg;
	conn = S2C(session);

	WT_CLEAR(path);
	WT_CLEAR(tmp);

	/*
	 * We need a temporary place to build a path and an entry prefix.
	 * The length of the path plus 128 should be more than enough.
	 *
	 * We also need a place to store the current path, because that's
	 * how we know when to close/re-open the file.
	 */
	WT_ERR(__wt_buf_init(session, &path, strlen(conn->stat_path) + 128));
	WT_ERR(__wt_buf_init(session, &tmp, strlen(conn->stat_path) + 128));

	while (F_ISSET(conn, WT_CONN_SERVER_RUN) &&
	    F_ISSET(conn, WT_CONN_SERVER_STATISTICS)) {
		/* Wait until the next event. */
		WT_ERR(
		    __wt_cond_wait(session, conn->stat_cond, conn->stat_usecs));

		if (!FLD_ISSET(conn->stat_flags, WT_CONN_STAT_NONE))
			WT_ERR(__statlog_log_one(session, &path, &tmp));
	}

	if (0) {
err:		WT_PANIC_MSG(session, ret, "statistics log server error");
	}
	__wt_buf_free(session, &path);
	__wt_buf_free(session, &tmp);
	return (WT_THREAD_RET_VALUE);
}

/*
 * __statlog_start --
 *	Start the statistics server thread.
 */
static int
__statlog_start(WT_CONNECTION_IMPL *conn)
{
	WT_SESSION_IMPL *session;

	/* Nothing to do if the server is already running. */
	if (conn->stat_session != NULL)
		return (0);

	F_SET(conn, WT_CONN_SERVER_STATISTICS);
	/* The statistics log server gets its own session. */
	WT_RET(__wt_open_internal_session(
	    conn, "statlog-server", 1, 1, &conn->stat_session));
	session = conn->stat_session;

	WT_RET(__wt_cond_alloc(
	    session, "statistics log server", 0, &conn->stat_cond));

	/*
	 * Start the thread.
	 *
	 * Statistics logging creates a thread per database, rather than using
	 * a single thread to do logging for all of the databases.   If we ever
	 * see lots of databases at a time, doing statistics logging, and we
	 * want to reduce the number of threads, there's no reason we have to
	 * have more than one thread, I just didn't feel like writing the code
	 * to figure out the scheduling.
	 */
	WT_RET(__wt_thread_create(
	    session, &conn->stat_tid, __statlog_server, session));
	conn->stat_tid_set = 1;

	return (0);
}

/*
 * __wt_statlog_create --
 *	Start the statistics server thread.
 */
int
__wt_statlog_create(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_CONNECTION_IMPL *conn;
	int start;

	conn = S2C(session);
	start = 0;

	/*
	 * Stop any server that is already running. This means that each time
	 * reconfigure is called we'll bounce the server even if there are no
	 * configuration changes - but that makes our lives easier.
	 */
	if (conn->stat_session != NULL)
		WT_RET(__wt_statlog_destroy(session, 0));

	WT_RET(__statlog_config(session, cfg, &start));
	if (start)
		WT_RET(__statlog_start(conn));

	return (0);
}

/*
 * __wt_statlog_destroy --
 *	Destroy the statistics server thread.
 */
int
__wt_statlog_destroy(WT_SESSION_IMPL *session, int is_close)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	WT_SESSION *wt_session;

	conn = S2C(session);

	F_CLR(conn, WT_CONN_SERVER_STATISTICS);
	if (conn->stat_tid_set) {
		WT_TRET(__wt_cond_signal(session, conn->stat_cond));
		WT_TRET(__wt_thread_join(session, conn->stat_tid));
		conn->stat_tid_set = 0;
	}

	/* Log a set of statistics on shutdown if configured. */
	if (is_close)
		WT_TRET(__wt_statlog_log_one(session));

	WT_TRET(__wt_cond_destroy(session, &conn->stat_cond));

	__stat_sources_free(session, &conn->stat_sources);
	__wt_free(session, conn->stat_path);
	__wt_free(session, conn->stat_format);

	/* Close the server thread's session. */
	if (conn->stat_session != NULL) {
		wt_session = &conn->stat_session->iface;
		WT_TRET(wt_session->close(wt_session, NULL));
	}

	/* Clear connection settings so reconfigure is reliable. */
	conn->stat_session = NULL;
	conn->stat_tid_set = 0;
	conn->stat_format = NULL;
	WT_TRET(__wt_fclose(session, &conn->stat_fp, WT_FHANDLE_APPEND));
	conn->stat_path = NULL;
	conn->stat_sources = NULL;
	conn->stat_stamp = NULL;
	conn->stat_usecs = 0;

	return (ret);
}
