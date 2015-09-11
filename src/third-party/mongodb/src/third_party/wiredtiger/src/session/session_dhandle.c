/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

static int __session_dhandle_sweep(WT_SESSION_IMPL *);

/*
 * __session_add_dhandle --
 *	Add a handle to the session's cache.
 */
static int
__session_add_dhandle(
    WT_SESSION_IMPL *session, WT_DATA_HANDLE_CACHE **dhandle_cachep)
{
	WT_DATA_HANDLE_CACHE *dhandle_cache;
	uint64_t bucket;

	WT_RET(__wt_calloc_one(session, &dhandle_cache));
	dhandle_cache->dhandle = session->dhandle;

	bucket = dhandle_cache->dhandle->name_hash % WT_HASH_ARRAY_SIZE;
	TAILQ_INSERT_HEAD(&session->dhandles, dhandle_cache, q);
	TAILQ_INSERT_HEAD(&session->dhhash[bucket], dhandle_cache, hashq);

	if (dhandle_cachep != NULL)
		*dhandle_cachep = dhandle_cache;

	(void)WT_ATOMIC_ADD4(session->dhandle->session_ref, 1);

	/* Sweep the handle list to remove any dead handles. */
	return (__session_dhandle_sweep(session));
}

/*
 * __session_discard_dhandle --
 *	Remove a data handle from the session cache.
 */
static void
__session_discard_dhandle(
    WT_SESSION_IMPL *session, WT_DATA_HANDLE_CACHE *dhandle_cache)
{
	uint64_t bucket;

	bucket = dhandle_cache->dhandle->name_hash % WT_HASH_ARRAY_SIZE;
	TAILQ_REMOVE(&session->dhandles, dhandle_cache, q);
	TAILQ_REMOVE(&session->dhhash[bucket], dhandle_cache, hashq);

	(void)WT_ATOMIC_SUB4(dhandle_cache->dhandle->session_ref, 1);

	__wt_overwrite_and_free(session, dhandle_cache);
}

/*
 * __session_find_dhandle --
 *	Search for a data handle in the session cache.
 */
static void
__session_find_dhandle(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint,
    WT_DATA_HANDLE_CACHE **dhandle_cachep)
{
	WT_DATA_HANDLE *dhandle;
	WT_DATA_HANDLE_CACHE *dhandle_cache;
	uint64_t bucket;

	dhandle = NULL;

	bucket = __wt_hash_city64(uri, strlen(uri)) % WT_HASH_ARRAY_SIZE;
retry:	TAILQ_FOREACH(dhandle_cache, &session->dhhash[bucket], hashq) {
		dhandle = dhandle_cache->dhandle;
		if (WT_DHANDLE_INACTIVE(dhandle) && !WT_IS_METADATA(dhandle)) {
			__session_discard_dhandle(session, dhandle_cache);
			/* We deleted our entry, retry from the start. */
			goto retry;
		}

		if (strcmp(uri, dhandle->name) != 0)
			continue;
		if (checkpoint == NULL && dhandle->checkpoint == NULL)
			break;
		if (checkpoint != NULL && dhandle->checkpoint != NULL &&
		    strcmp(checkpoint, dhandle->checkpoint) == 0)
			break;
	}

	*dhandle_cachep = dhandle_cache;
}

/*
 * __wt_session_lock_dhandle --
 *	Try to lock a handle that is cached in this session.  This is the fast
 *	path that tries to lock a handle without the need for the schema lock.
 *
 *	If the handle can't be locked in the required state, release it and
 *	fail with WT_NOTFOUND: we have to take the slow path after acquiring
 *	the schema lock.
 */
int
__wt_session_lock_dhandle(WT_SESSION_IMPL *session, uint32_t flags, int *deadp)
{
	enum { NOLOCK, READLOCK, WRITELOCK } locked;
	WT_BTREE *btree;
	WT_DATA_HANDLE *dhandle;
	uint32_t special_flags;

	btree = S2BT(session);
	dhandle = session->dhandle;
	locked = NOLOCK;
	if (deadp != NULL)
		*deadp = 0;

	/*
	 * Special operation flags will cause the handle to be reopened.
	 * For example, a handle opened with WT_BTREE_BULK cannot use the same
	 * internal data structures as a handle opened for ordinary access.
	 */
	special_flags = LF_ISSET(WT_BTREE_SPECIAL_FLAGS);
	WT_ASSERT(session,
	    special_flags == 0 || LF_ISSET(WT_DHANDLE_EXCLUSIVE));

	if (LF_ISSET(WT_DHANDLE_EXCLUSIVE)) {
		/*
		 * Try to get an exclusive handle lock and fail immediately if
		 * it's unavailable.  We don't expect exclusive operations on
		 * trees to be mixed with ordinary cursor access, but if there
		 * is a use case in the future, we could make blocking here
		 * configurable.
		 *
		 * Special flags will cause the handle to be reopened, which
		 * will get the necessary lock, so don't bother here.
		 */
		if (LF_ISSET(WT_DHANDLE_LOCK_ONLY) || special_flags == 0) {
			WT_RET(__wt_try_writelock(session, dhandle->rwlock));
			F_SET(dhandle, WT_DHANDLE_EXCLUSIVE);
			locked = WRITELOCK;
		}
	} else if (F_ISSET(btree, WT_BTREE_SPECIAL_FLAGS))
		return (EBUSY);
	else {
		WT_RET(__wt_readlock(session, dhandle->rwlock));
		locked = READLOCK;
	}

	/*
	 * At this point, we have the requested lock -- if that is all that was
	 * required, we're done.  Otherwise, check that the handle is open and
	 * that no special flags are required.
	 */
	if (F_ISSET(dhandle, WT_DHANDLE_DEAD)) {
		WT_ASSERT(session, deadp != NULL);
		*deadp = 1;
	} else if (LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
	    (F_ISSET(dhandle, WT_DHANDLE_OPEN) && special_flags == 0))
		return (0);

	/*
	 * The handle needs to be opened.  If we locked the handle above,
	 * unlock it before returning.
	 */
	switch (locked) {
	case NOLOCK:
		break;
	case READLOCK:
		WT_RET(__wt_readunlock(session, dhandle->rwlock));
		break;
	case WRITELOCK:
		F_CLR(dhandle, WT_DHANDLE_EXCLUSIVE);
		WT_RET(__wt_writeunlock(session, dhandle->rwlock));
		break;
	}

	/* Treat an unopened handle just like a non-existent handle. */
	return (WT_NOTFOUND);
}

/*
 * __wt_session_release_btree --
 *	Unlock a btree handle.
 */
int
__wt_session_release_btree(WT_SESSION_IMPL *session)
{
	enum { NOLOCK, READLOCK, WRITELOCK } locked;
	WT_BTREE *btree;
	WT_DATA_HANDLE *dhandle;
	WT_DATA_HANDLE_CACHE *dhandle_cache;
	WT_DECL_RET;

	btree = S2BT(session);
	dhandle = session->dhandle;

	locked = F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE) ? WRITELOCK : READLOCK;
	/*
	 * If we had special flags set, close the handle so that future access
	 * can get a handle without special flags.
	 */
	if (F_ISSET(dhandle, WT_DHANDLE_DISCARD | WT_DHANDLE_DISCARD_FORCE)) {
		__session_find_dhandle(session,
		    dhandle->name, dhandle->checkpoint, &dhandle_cache);
		if (dhandle_cache != NULL)
			__session_discard_dhandle(session, dhandle_cache);
	}

	if (F_ISSET(dhandle, WT_DHANDLE_DISCARD_FORCE)) {
		WT_WITH_DHANDLE_LOCK(session,
		    ret = __wt_conn_btree_sync_and_close(session, 0, 1));
		F_CLR(dhandle, WT_DHANDLE_DISCARD_FORCE);
	} else if (F_ISSET(dhandle, WT_DHANDLE_DISCARD) ||
	    F_ISSET(btree, WT_BTREE_SPECIAL_FLAGS)) {
		WT_ASSERT(session, F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE));
		ret = __wt_conn_btree_sync_and_close(session, 0, 0);
		F_CLR(dhandle, WT_DHANDLE_DISCARD);
	}

	if (F_ISSET(dhandle, WT_DHANDLE_EXCLUSIVE))
		F_CLR(dhandle, WT_DHANDLE_EXCLUSIVE);

	switch (locked) {
	case NOLOCK:
		break;
	case READLOCK:
		WT_TRET(__wt_readunlock(session, dhandle->rwlock));
		break;
	case WRITELOCK:
		WT_TRET(__wt_writeunlock(session, dhandle->rwlock));
		break;
	}

	session->dhandle = NULL;
	return (ret);
}

/*
 * __wt_session_get_btree_ckpt --
 *	Check the configuration strings for a checkpoint name, get a btree
 * handle for the given name, set session->dhandle.
 */
int
__wt_session_get_btree_ckpt(WT_SESSION_IMPL *session,
    const char *uri, const char *cfg[], uint32_t flags)
{
	WT_CONFIG_ITEM cval;
	WT_DECL_RET;
	int last_ckpt;
	const char *checkpoint;

	last_ckpt = 0;
	checkpoint = NULL;

	/*
	 * This function exists to handle checkpoint configuration.  Callers
	 * that never open a checkpoint call the underlying function directly.
	 */
	WT_RET_NOTFOUND_OK(
	    __wt_config_gets_def(session, cfg, "checkpoint", 0, &cval));
	if (cval.len != 0) {
		/*
		 * The internal checkpoint name is special, find the last
		 * unnamed checkpoint of the object.
		 */
		if (WT_STRING_MATCH(WT_CHECKPOINT, cval.str, cval.len)) {
			last_ckpt = 1;
retry:			WT_RET(__wt_meta_checkpoint_last_name(
			    session, uri, &checkpoint));
		} else
			WT_RET(__wt_strndup(
			    session, cval.str, cval.len, &checkpoint));
	}

	ret = __wt_session_get_btree(session, uri, checkpoint, cfg, flags);

	__wt_free(session, checkpoint);

	/*
	 * There's a potential race: we get the name of the most recent unnamed
	 * checkpoint, but if it's discarded (or locked so it can be discarded)
	 * by the time we try to open it, we'll fail the open.  Retry in those
	 * cases, a new "last" checkpoint should surface, and we can't return an
	 * error, the application will be justifiably upset if we can't open the
	 * last checkpoint instance of an object.
	 *
	 * The check against WT_NOTFOUND is correct: if there was no checkpoint
	 * for the object (that is, the object has never been in a checkpoint),
	 * we returned immediately after the call to search for that name.
	 */
	if (last_ckpt && (ret == WT_NOTFOUND || ret == EBUSY))
		goto retry;
	return (ret);
}

/*
 * __wt_session_close_cache --
 *	Close any cached handles in a session.
 */
void
__wt_session_close_cache(WT_SESSION_IMPL *session)
{
	WT_DATA_HANDLE_CACHE *dhandle_cache;

	while ((dhandle_cache = TAILQ_FIRST(&session->dhandles)) != NULL)
		__session_discard_dhandle(session, dhandle_cache);
}

/*
 * __session_dhandle_sweep --
 *	Discard any session dhandles that are not open.
 */
static int
__session_dhandle_sweep(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	WT_DATA_HANDLE *dhandle;
	WT_DATA_HANDLE_CACHE *dhandle_cache, *dhandle_cache_next;
	time_t now;

	conn = S2C(session);

	/*
	 * Periodically sweep for dead handles; if we've swept recently, don't
	 * do it again.
	 */
	WT_RET(__wt_seconds(session, &now));
	if (now - session->last_sweep < conn->sweep_interval)
		return (0);
	session->last_sweep = now;

	WT_STAT_FAST_CONN_INCR(session, dh_session_sweeps);

	dhandle_cache = TAILQ_FIRST(&session->dhandles);
	while (dhandle_cache != NULL) {
		dhandle_cache_next = TAILQ_NEXT(dhandle_cache, q);
		dhandle = dhandle_cache->dhandle;
		if (dhandle != session->dhandle &&
		    dhandle->session_inuse == 0 &&
		    (WT_DHANDLE_INACTIVE(dhandle) ||
		    (dhandle->timeofdeath != 0 &&
		    now - dhandle->timeofdeath > conn->sweep_idle_time))) {
			WT_STAT_FAST_CONN_INCR(session, dh_session_handles);
			WT_ASSERT(session, !WT_IS_METADATA(dhandle));
			__session_discard_dhandle(session, dhandle_cache);
		}
		dhandle_cache = dhandle_cache_next;
	}
	return (0);
}

/*
 * __session_find_shared_dhandle --
 *	Search for a data handle in the connection and add it to a session's
 *	cache.  Since the data handle isn't locked, this must be called holding
 *	the handle list lock, and we must increment the handle's reference
 *	count before releasing it.
 */
static int
__session_find_shared_dhandle(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint, uint32_t flags)
{
	WT_RET(__wt_conn_dhandle_find(session, uri, checkpoint, flags));
	(void)WT_ATOMIC_ADD4(session->dhandle->session_ref, 1);
	return (0);
}

/*
 * __wt_session_get_btree --
 *	Get a btree handle for the given name, set session->dhandle.
 */
int
__wt_session_get_btree(WT_SESSION_IMPL *session,
    const char *uri, const char *checkpoint, const char *cfg[], uint32_t flags)
{
	WT_DATA_HANDLE *dhandle;
	WT_DATA_HANDLE_CACHE *dhandle_cache;
	WT_DECL_RET;
	int is_dead;

	WT_ASSERT(session, !F_ISSET(session, WT_SESSION_NO_DATA_HANDLES));
	WT_ASSERT(session, !LF_ISSET(WT_DHANDLE_HAVE_REF));

	__session_find_dhandle(session, uri, checkpoint, &dhandle_cache);

	if (dhandle_cache != NULL)
		session->dhandle = dhandle = dhandle_cache->dhandle;
	else {
		/*
		 * We didn't find a match in the session cache, now search the
		 * shared handle list and cache any handle we find.
		 */
		WT_WITH_DHANDLE_LOCK(session, ret =
		    __session_find_shared_dhandle(session, uri, checkpoint, flags));
		dhandle = (ret == 0) ? session->dhandle : NULL;
		WT_RET_NOTFOUND_OK(ret);
	}

	if (dhandle != NULL) {
		/* Try to lock the handle; if this succeeds, we're done. */
		if ((ret =
		    __wt_session_lock_dhandle(session, flags, &is_dead)) == 0)
			goto done;

		/* Propagate errors we don't expect. */
		if (ret != WT_NOTFOUND && ret != EBUSY)
			return (ret);

		/*
		 * Don't try harder to get the handle if we're only checking
		 * for locks or our caller hasn't allowed us to take the schema
		 * lock - they do so on purpose and will handle error returns.
		 */
		if ((LF_ISSET(WT_DHANDLE_LOCK_ONLY) && ret == EBUSY) ||
		    (!F_ISSET(session, WT_SESSION_SCHEMA_LOCKED) &&
		    F_ISSET(session,
		    WT_SESSION_HANDLE_LIST_LOCKED | WT_SESSION_TABLE_LOCKED)))
			return (ret);

		/* If we found the handle and it isn't dead, reopen it. */
		if (is_dead) {
			__session_discard_dhandle(session, dhandle_cache);
			dhandle_cache = NULL;
			session->dhandle = dhandle = NULL;
		} else
			LF_SET(WT_DHANDLE_HAVE_REF);
	}

	/*
	 * Acquire the schema lock and the data handle lock, find and/or
	 * open the handle.
	 *
	 * We need the schema lock for this call so that if we lock a handle in
	 * order to open it, that doesn't race with a schema-changing operation
	 * such as drop.
	 *
	 * Recheck whether the handle is dead after grabbing the locks.  If so,
	 * discard the handle and retry.
	 */
retry:	is_dead = 0;
	WT_WITH_SCHEMA_LOCK(session,
	    WT_WITH_DHANDLE_LOCK(session, ret =
		(is_dead = (dhandle != NULL &&
		    F_ISSET(dhandle, WT_DHANDLE_DEAD))) ?
		0 : __wt_conn_btree_get(session, uri, checkpoint, cfg, flags)));

	if (is_dead) {
		if (dhandle_cache != NULL)
			__session_discard_dhandle(session, dhandle_cache);
		dhandle_cache = NULL;
		session->dhandle = dhandle = NULL;
		LF_CLR(WT_DHANDLE_HAVE_REF);
		goto retry;
	}
	WT_RET(ret);

	if (!LF_ISSET(WT_DHANDLE_HAVE_REF))
		WT_RET(__session_add_dhandle(session, NULL));

	WT_ASSERT(session, LF_ISSET(WT_DHANDLE_LOCK_ONLY) ||
	    (F_ISSET(session->dhandle, WT_DHANDLE_OPEN) &&
	    !F_ISSET(session->dhandle, WT_DHANDLE_DEAD)));

done:	WT_ASSERT(session, LF_ISSET(WT_DHANDLE_EXCLUSIVE) ==
	    F_ISSET(session->dhandle, WT_DHANDLE_EXCLUSIVE));

	return (0);
}

/*
 * __wt_session_lock_checkpoint --
 *	Lock the btree handle for the given checkpoint name.
 */
int
__wt_session_lock_checkpoint(WT_SESSION_IMPL *session, const char *checkpoint)
{
	WT_DATA_HANDLE *dhandle, *saved_dhandle;
	WT_DECL_RET;

	WT_ASSERT(session, WT_META_TRACKING(session));
	saved_dhandle = session->dhandle;

	/*
	 * If we already have the checkpoint locked, don't attempt to lock
	 * it again.
	 */
	if ((ret = __wt_meta_track_find_handle(
	    session, saved_dhandle->name, checkpoint)) != WT_NOTFOUND)
		return (ret);

	/*
	 * Get the checkpoint handle exclusive, so no one else can access it
	 * while we are creating the new checkpoint.
	 */
	WT_ERR(__wt_session_get_btree(session, saved_dhandle->name,
	    checkpoint, NULL, WT_DHANDLE_EXCLUSIVE | WT_DHANDLE_LOCK_ONLY));

	/*
	 * Flush any pages in this checkpoint from the cache (we are about to
	 * re-write the checkpoint which will mean cached pages no longer have
	 * valid contents).  This is especially noticeable with memory mapped
	 * files, since changes to the underlying file are visible to the in
	 * memory pages.
	 */
	WT_ERR(__wt_cache_op(session, NULL, WT_SYNC_DISCARD));

	/*
	 * We lock checkpoint handles that we are overwriting, so the handle
	 * must be closed when we release it.
	 */
	dhandle = session->dhandle;
	F_SET(dhandle, WT_DHANDLE_DISCARD);

	WT_ERR(__wt_meta_track_handle_lock(session, 0));

	/* Restore the original btree in the session. */
err:	session->dhandle = saved_dhandle;

	return (ret);
}
