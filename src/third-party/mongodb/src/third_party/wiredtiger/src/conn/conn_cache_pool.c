/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * Tuning constants.
 */
/* Threshold when a connection is allocated more cache */
#define	WT_CACHE_POOL_BUMP_THRESHOLD	6
/* Threshold when a connection is allocated less cache */
#define	WT_CACHE_POOL_REDUCE_THRESHOLD	2
/* Balancing passes after a bump before a connection is a candidate. */
#define	WT_CACHE_POOL_BUMP_SKIPS	10
/* Balancing passes after a reduction before a connection is a candidate. */
#define	WT_CACHE_POOL_REDUCE_SKIPS	5

static int __cache_pool_adjust(WT_SESSION_IMPL *, uint64_t, uint64_t, int *);
static int __cache_pool_assess(WT_SESSION_IMPL *, uint64_t *);
static int __cache_pool_balance(WT_SESSION_IMPL *);

/*
 * __wt_cache_pool_config --
 *	Parse and setup the cache pool options.
 */
int
__wt_cache_pool_config(WT_SESSION_IMPL *session, const char **cfg)
{
	WT_CACHE_POOL *cp;
	WT_CONFIG_ITEM cval;
	WT_CONNECTION_IMPL *conn, *entry;
	WT_DECL_RET;
	char *pool_name;
	int created, updating;
	uint64_t chunk, reserve, size, used_cache;

	conn = S2C(session);
	created = updating = 0;
	pool_name = NULL;
	cp = NULL;
	size = 0;

	if (F_ISSET(conn, WT_CONN_CACHE_POOL))
		updating = 1;
	else {
		WT_RET(__wt_config_gets_none(
		    session, cfg, "shared_cache.name", &cval));
		if (cval.len == 0) {
			/*
			 * Tell the user if they configured a cache pool
			 * size but didn't enable it by naming the pool.
			 */
			if (__wt_config_gets(session, &cfg[1],
			    "shared_cache.size", &cval) != WT_NOTFOUND)
				WT_RET_MSG(session, EINVAL,
				    "Shared cache configuration requires a "
				    "pool name");
			return (0);
		}

		if (__wt_config_gets(session,
		    &cfg[1], "cache_size", &cval) != WT_NOTFOUND)
			WT_RET_MSG(session, EINVAL,
			    "Only one of cache_size and shared_cache can be "
			    "in the configuration");

		/*
		 * NOTE: The allocations made when configuring and opening a
		 * cache pool don't really belong to the connection that
		 * allocates them. If a memory allocator becomes connection
		 * specific in the future we will need a way to allocate memory
		 * outside of the connection here.
		 */
		WT_RET(__wt_strndup(session, cval.str, cval.len, &pool_name));
	}

	__wt_spin_lock(session, &__wt_process.spinlock);
	if (__wt_process.cache_pool == NULL) {
		WT_ASSERT(session, !updating);
		/* Create a cache pool. */
		WT_ERR(__wt_calloc_one(session, &cp));
		created = 1;
		cp->name = pool_name;
		pool_name = NULL; /* Belongs to the cache pool now. */
		TAILQ_INIT(&cp->cache_pool_qh);
		WT_ERR(__wt_spin_init(
		    session, &cp->cache_pool_lock, "cache shared pool"));
		WT_ERR(__wt_cond_alloc(session,
		    "cache pool server", 0, &cp->cache_pool_cond));

		__wt_process.cache_pool = cp;
		WT_ERR(__wt_verbose(session,
		    WT_VERB_SHARED_CACHE, "Created cache pool %s", cp->name));
	} else if (!updating && !WT_STRING_MATCH(
	    __wt_process.cache_pool->name, pool_name, strlen(pool_name)))
		/* Only a single cache pool is supported. */
		WT_ERR_MSG(session, WT_ERROR,
		    "Attempting to join a cache pool that does not exist: %s",
		    pool_name);

	cp = __wt_process.cache_pool;

	/*
	 * The cache pool requires a reference count to avoid a race between
	 * configuration/open and destroy.
	 */
	if (!updating)
		++cp->refs;

	/*
	 * Cache pool configurations are optional when not creating. If
	 * values aren't being changed, retrieve the current value so that
	 * validation of settings works.
	 */
	if (!created) {
		if (__wt_config_gets(session, &cfg[1],
		    "shared_cache.size", &cval) == 0 && cval.val != 0)
			size = (uint64_t)cval.val;
		 else
			size = cp->size;
		if (__wt_config_gets(session, &cfg[1],
		    "shared_cache.chunk", &cval) == 0 && cval.val != 0)
			chunk = (uint64_t)cval.val;
		else
			chunk = cp->chunk;
	} else {
		/*
		 * The only time shared cache configuration uses default
		 * values is when we are creating the pool.
		 */
		WT_ERR(__wt_config_gets(
		    session, cfg, "shared_cache.size", &cval));
		WT_ASSERT(session, cval.val != 0);
		size = (uint64_t)cval.val;
		WT_ERR(__wt_config_gets(
		    session, cfg, "shared_cache.chunk", &cval));
		WT_ASSERT(session, cval.val != 0);
		chunk = (uint64_t)cval.val;
	}

	/*
	 * Retrieve the reserve size here for validation of configuration.
	 * Don't save it yet since the connections cache is not created if
	 * we are opening. Cache configuration is responsible for saving the
	 * setting.
	 * The different conditions when reserved size are set are:
	 *  - It's part of the users configuration - use that value.
	 *  - We are reconfiguring - keep the previous value.
	 *  - We are joining a cache pool for the first time (including
	 *  creating the pool) - use the chunk size; that's the default.
	 */
	if (__wt_config_gets(session, &cfg[1],
	    "shared_cache.reserve", &cval) == 0 && cval.val != 0)
		reserve = (uint64_t)cval.val;
	else if (updating)
		reserve = conn->cache->cp_reserved;
	else
		reserve = chunk;

	/*
	 * Validate that size and reserve values don't cause the cache
	 * pool to be over subscribed.
	 */
	used_cache = 0;
	if (!created) {
		TAILQ_FOREACH(entry, &cp->cache_pool_qh, cpq)
			used_cache += entry->cache->cp_reserved;
	}
	/* Ignore our old allocation if reconfiguring */
	if (updating)
		used_cache -= conn->cache->cp_reserved;
	if (used_cache + reserve > size)
		WT_ERR_MSG(session, EINVAL,
		    "Shared cache unable to accommodate this configuration. "
		    "Shared cache size: %" PRIu64 ", requested min: %" PRIu64,
		    size, used_cache + reserve);

	/* The configuration is verified - it's safe to update the pool. */
	cp->size = size;
	cp->chunk = chunk;

	conn->cache->cp_reserved = reserve;

	/* Wake up the cache pool server so any changes are noticed. */
	if (updating)
		WT_ERR(__wt_cond_signal(
		    session, __wt_process.cache_pool->cache_pool_cond));

	WT_ERR(__wt_verbose(session, WT_VERB_SHARED_CACHE,
	    "Configured cache pool %s. Size: %" PRIu64
	    ", chunk size: %" PRIu64, cp->name, cp->size, cp->chunk));

	F_SET(conn, WT_CONN_CACHE_POOL);
err:	__wt_spin_unlock(session, &__wt_process.spinlock);
	if (!updating)
		__wt_free(session, pool_name);
	if (ret != 0 && created) {
		__wt_free(session, cp->name);
		WT_TRET(__wt_cond_destroy(session, &cp->cache_pool_cond));
		__wt_free(session, cp);
	}
	return (ret);
}

/*
 * __wt_conn_cache_pool_open --
 *	Add a connection to the cache pool.
 */
int
__wt_conn_cache_pool_open(WT_SESSION_IMPL *session)
{
	WT_CACHE *cache;
	WT_CACHE_POOL *cp;
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;

	conn = S2C(session);
	cache = conn->cache;
	cp = __wt_process.cache_pool;

	/*
	 * Create a session that can be used by the cache pool thread, do
	 * it in the main thread to avoid shutdown races
	 */
	if ((ret = __wt_open_internal_session(
		conn, "cache-pool", 0, 0, &cache->cp_session)) != 0)
		WT_RET_MSG(NULL, ret,
		    "Failed to create session for cache pool");

	/*
	 * Add this connection into the cache pool connection queue. Figure
	 * out if a manager thread is needed while holding the lock. Don't
	 * start the thread until we have released the lock.
	 */
	__wt_spin_lock(session, &cp->cache_pool_lock);
	TAILQ_INSERT_TAIL(&cp->cache_pool_qh, conn, cpq);
	__wt_spin_unlock(session, &cp->cache_pool_lock);

	WT_RET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
	    "Added %s to cache pool %s", conn->home, cp->name));

	/*
	 * Each connection participating in the cache pool starts a manager
	 * thread. Only one manager is active at a time, but having a thread
	 * in each connection saves having a complex election process when
	 * the active connection shuts down.
	 */
	F_SET_ATOMIC(cp, WT_CACHE_POOL_ACTIVE);
	F_SET(cache, WT_CACHE_POOL_RUN);
	WT_RET(__wt_thread_create(session, &cache->cp_tid,
	    __wt_cache_pool_server, cache->cp_session));

	/* Wake up the cache pool server to get our initial chunk. */
	WT_RET(__wt_cond_signal(session, cp->cache_pool_cond));

	return (0);
}

/*
 * __wt_conn_cache_pool_destroy --
 *	Remove our resources from the shared cache pool. Remove the cache pool
 *	if we were the last connection.
 */
int
__wt_conn_cache_pool_destroy(WT_SESSION_IMPL *session)
{
	WT_CACHE *cache;
	WT_CACHE_POOL *cp;
	WT_CONNECTION_IMPL *conn, *entry;
	WT_DECL_RET;
	WT_SESSION *wt_session;
	int cp_locked, found;

	conn = S2C(session);
	cache = conn->cache;
	cp_locked = found = 0;
	cp = __wt_process.cache_pool;

	if (!F_ISSET(conn, WT_CONN_CACHE_POOL))
		return (0);

	__wt_spin_lock(session, &cp->cache_pool_lock);
	cp_locked = 1;
	TAILQ_FOREACH(entry, &cp->cache_pool_qh, cpq)
		if (entry == conn) {
			found = 1;
			break;
		}

	/*
	 * If there was an error during open, we may not have made it onto the
	 * queue.  We did increment the reference count, so proceed regardless.
	 */
	if (found) {
		WT_TRET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
		    "Removing %s from cache pool", entry->home));
		TAILQ_REMOVE(&cp->cache_pool_qh, entry, cpq);

		/* Give the connection's resources back to the pool. */
		WT_ASSERT(session, cp->currently_used >= conn->cache_size);
		cp->currently_used -= conn->cache_size;

		/*
		 * Stop our manager thread - release the cache pool lock while
		 * joining the thread to allow it to complete any balance
		 * operation.
		 */
		__wt_spin_unlock(session, &cp->cache_pool_lock);
		cp_locked = 0;

		F_CLR(cache, WT_CACHE_POOL_RUN);
		WT_TRET(__wt_cond_signal(session, cp->cache_pool_cond));
		WT_TRET(__wt_thread_join(session, cache->cp_tid));

		wt_session = &cache->cp_session->iface;
		WT_TRET(wt_session->close(wt_session, NULL));

		/*
		 * Grab the lock again now to stop other threads joining the
		 * pool while we are figuring out whether we were the last
		 * participant.
		 */
		__wt_spin_lock(session, &cp->cache_pool_lock);
		cp_locked = 1;
	}

	/*
	 * If there are no references, we are cleaning up after a failed
	 * wiredtiger_open, there is nothing further to do.
	 */
	if (cp->refs < 1) {
		if (cp_locked)
			__wt_spin_unlock(session, &cp->cache_pool_lock);
		return (0);
	}

	if (--cp->refs == 0) {
		WT_ASSERT(session, TAILQ_EMPTY(&cp->cache_pool_qh));
		F_CLR_ATOMIC(cp, WT_CACHE_POOL_ACTIVE);
	}

	if (!F_ISSET_ATOMIC(cp, WT_CACHE_POOL_ACTIVE)) {
		WT_TRET(__wt_verbose(
		    session, WT_VERB_SHARED_CACHE, "Destroying cache pool"));
		__wt_spin_lock(session, &__wt_process.spinlock);
		/*
		 * We have been holding the pool lock - no connections could
		 * have been added.
		 */
		WT_ASSERT(session,
		    cp == __wt_process.cache_pool &&
		    TAILQ_EMPTY(&cp->cache_pool_qh));
		__wt_process.cache_pool = NULL;
		__wt_spin_unlock(session, &__wt_process.spinlock);
		__wt_spin_unlock(session, &cp->cache_pool_lock);
		cp_locked = 0;

		/* Now free the pool. */
		__wt_free(session, cp->name);

		__wt_spin_destroy(session, &cp->cache_pool_lock);
		WT_TRET(__wt_cond_destroy(session, &cp->cache_pool_cond));
		__wt_free(session, cp);
	}

	if (cp_locked) {
		__wt_spin_unlock(session, &cp->cache_pool_lock);

		/* Notify other participants if we were managing */
		if (F_ISSET(cache, WT_CACHE_POOL_MANAGER)) {
			F_CLR_ATOMIC(cp, WT_CACHE_POOL_MANAGED);
			WT_TRET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
			    "Shutting down shared cache manager connection"));
		}
	}

	return (ret);
}

/*
 * __cache_pool_balance --
 *	Do a pass over the cache pool members and ensure the pool is being
 *	effectively used.
 */
static int
__cache_pool_balance(WT_SESSION_IMPL *session)
{
	WT_CACHE_POOL *cp;
	WT_DECL_RET;
	int adjusted;
	uint64_t bump_threshold, highest;

	cp = __wt_process.cache_pool;
	adjusted = 0;
	highest = 0;

	__wt_spin_lock(NULL, &cp->cache_pool_lock);

	/* If the queue is empty there is nothing to do. */
	if (TAILQ_FIRST(&cp->cache_pool_qh) == NULL)
		goto err;

	WT_ERR(__cache_pool_assess(session, &highest));
	bump_threshold = WT_CACHE_POOL_BUMP_THRESHOLD;
	/*
	 * Actively attempt to:
	 * - Reduce the amount allocated, if we are over the budget
	 * - Increase the amount used if there is capacity and any pressure.
	 */
	for (bump_threshold = WT_CACHE_POOL_BUMP_THRESHOLD;
	    F_ISSET_ATOMIC(cp, WT_CACHE_POOL_ACTIVE) &&
	    F_ISSET(S2C(session)->cache, WT_CACHE_POOL_RUN);) {
		WT_ERR(__cache_pool_adjust(
		    session, highest, bump_threshold, &adjusted));
		/*
		 * Stop if the amount of cache being used is stable, and we
		 * aren't over capacity.
		 */
		if (cp->currently_used <= cp->size && !adjusted)
			break;
		if (bump_threshold > 0)
			--bump_threshold;
	}

err:	__wt_spin_unlock(NULL, &cp->cache_pool_lock);
	return (ret);
}

/*
 * __cache_pool_assess --
 *	Assess the usage of the cache pool.
 */
static int
__cache_pool_assess(WT_SESSION_IMPL *session, uint64_t *phighest)
{
	WT_CACHE_POOL *cp;
	WT_CACHE *cache;
	WT_CONNECTION_IMPL *entry;
	uint64_t entries, highest, new;

	cp = __wt_process.cache_pool;
	entries = highest = 0;

	/* Generate read pressure information. */
	TAILQ_FOREACH(entry, &cp->cache_pool_qh, cpq) {
		if (entry->cache_size == 0 ||
		    entry->cache == NULL)
			continue;
		cache = entry->cache;
		++entries;
		new = cache->bytes_read;
		/* Handle wrapping of eviction requests. */
		if (new >= cache->cp_saved_read)
			cache->cp_current_read = new - cache->cp_saved_read;
		else
			cache->cp_current_read = new;
		cache->cp_saved_read = new;
		if (cache->cp_current_read > highest)
			highest = cache->cp_current_read;
	}
	WT_RET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
	    "Highest eviction count: %" PRIu64 ", entries: %" PRIu64,
	    highest, entries));
	/* Normalize eviction information across connections. */
	highest = highest / (entries + 1);
	++highest; /* Avoid divide by zero. */

	*phighest = highest;
	return (0);
}

/*
 * __cache_pool_adjust --
 *	Adjust the allocation of cache to each connection. If force is set
 *	ignore cache load information, and reduce the allocation for every
 *	connection allocated more than their reserved size.
 */
static int
__cache_pool_adjust(WT_SESSION_IMPL *session,
    uint64_t highest, uint64_t bump_threshold, int *adjustedp)
{
	WT_CACHE_POOL *cp;
	WT_CACHE *cache;
	WT_CONNECTION_IMPL *entry;
	uint64_t adjusted, reserved, read_pressure;
	int force, grew;

	*adjustedp = 0;
	cp = __wt_process.cache_pool;
	force = (cp->currently_used > cp->size);
	grew = 0;
	if (WT_VERBOSE_ISSET(session, WT_VERB_SHARED_CACHE)) {
		WT_RET(__wt_verbose(session,
		    WT_VERB_SHARED_CACHE, "Cache pool distribution: "));
		WT_RET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
		    "\t" "cache_size, read_pressure, skips: "));
	}

	TAILQ_FOREACH(entry, &cp->cache_pool_qh, cpq) {
		cache = entry->cache;
		reserved = cache->cp_reserved;
		adjusted = 0;

		read_pressure = cache->cp_current_read / highest;
		WT_RET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
		    "\t%" PRIu64 ", %" PRIu64 ", %" PRIu32,
		    entry->cache_size, read_pressure, cache->cp_skip_count));

		/* Allow to stabilize after changes. */
		if (cache->cp_skip_count > 0 && --cache->cp_skip_count > 0)
			continue;
		/*
		 * If the entry is currently allocated less than the reserved
		 * size, increase it's allocation. This should only happen if:
		 *  - It's the first time we've seen this member
		 *  - The reserved size has been adjusted
		 */
		if (entry->cache_size < reserved) {
			grew = 1;
			adjusted = reserved - entry->cache_size;
		/*
		 * Conditions for reducing the amount of resources for an
		 * entry:
		 *  - If we are forcing and this entry has more than the
		 *    minimum amount of space in use.
		 *  - If the read pressure in this entry is below the
		 *    threshold, other entries need more cache, the entry has
		 *    more than the minimum space and there is no available
		 *    space in the pool.
		 */
		} else if ((force && entry->cache_size > reserved) ||
		    (read_pressure < WT_CACHE_POOL_REDUCE_THRESHOLD &&
		     highest > 1 && entry->cache_size > reserved &&
		     cp->currently_used >= cp->size)) {
			grew = 0;
			/*
			 * Shrink by a chunk size if that doesn't drop us
			 * below the reserved size.
			 */
			if (entry->cache_size > cp->chunk + reserved)
				adjusted = cp->chunk;
			else
				adjusted = entry->cache_size - reserved;
		/*
		 * Conditions for increasing the amount of resources for an
		 * entry:
		 *  - There was some activity across the pool
		 *  - This entry is using less than the entire cache pool
		 *  - The connection is using enough cache to require eviction
		 *  - There is space available in the pool
		 *  - Additional cache would benefit the connection
		 */
		} else if (highest > 1 &&
		    entry->cache_size < cp->size &&
		     cache->bytes_inmem >=
		     (entry->cache_size * cache->eviction_target) / 100 &&
		     cp->currently_used < cp->size &&
		     read_pressure > bump_threshold) {
			grew = 1;
			adjusted = WT_MIN(cp->chunk,
			    cp->size - cp->currently_used);
		}
		if (adjusted > 0) {
			*adjustedp = 1;
			if (grew > 0) {
				cache->cp_skip_count = WT_CACHE_POOL_BUMP_SKIPS;
				entry->cache_size += adjusted;
				cp->currently_used += adjusted;
			} else {
				cache->cp_skip_count =
				    WT_CACHE_POOL_REDUCE_SKIPS;
				WT_ASSERT(session,
				    entry->cache_size >= adjusted &&
				    cp->currently_used >= adjusted);
				entry->cache_size -= adjusted;
				cp->currently_used -= adjusted;
			}
			WT_RET(__wt_verbose(session, WT_VERB_SHARED_CACHE,
			    "Allocated %s%" PRId64 " to %s",
			    grew ? "" : "-", adjusted, entry->home));
			/*
			 * TODO: Add a loop waiting for connection to give up
			 * cache.
			 */
		}
	}
	return (0);
}

/*
 * __wt_cache_pool_server --
 *	Thread to manage cache pool among connections.
 */
WT_THREAD_RET
__wt_cache_pool_server(void *arg)
{
	WT_CACHE *cache;
	WT_CACHE_POOL *cp;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;

	session = (WT_SESSION_IMPL *)arg;

	cp = __wt_process.cache_pool;
	cache = S2C(session)->cache;

	while (F_ISSET_ATOMIC(cp, WT_CACHE_POOL_ACTIVE) &&
	    F_ISSET(cache, WT_CACHE_POOL_RUN)) {
		if (cp->currently_used <= cp->size)
			WT_ERR(__wt_cond_wait(session,
			    cp->cache_pool_cond, 1000000));

		/*
		 * Re-check pool run flag - since we want to avoid getting the
		 * lock on shutdown.
		 */
		if (!F_ISSET_ATOMIC(cp, WT_CACHE_POOL_ACTIVE) &&
		    F_ISSET(cache, WT_CACHE_POOL_RUN))
			break;

		/* Try to become the managing thread */
		F_CAS_ATOMIC(cp, WT_CACHE_POOL_MANAGED, ret);
		if (ret == 0) {
			F_SET(cache, WT_CACHE_POOL_MANAGER);
			WT_ERR(__wt_verbose(session, WT_VERB_SHARED_CACHE,
			    "Cache pool switched manager thread"));
		}

		/*
		 * Continue even if there was an error. Details of errors are
		 * reported in the balance function.
		 */
		if (F_ISSET(cache, WT_CACHE_POOL_MANAGER))
			(void)__cache_pool_balance(session);
	}

	if (0) {
err:		WT_PANIC_MSG(session, ret, "cache pool manager server error");
	}
	return (WT_THREAD_RET_VALUE);
}
