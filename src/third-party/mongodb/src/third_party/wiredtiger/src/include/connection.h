/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*******************************************
 * Global per-process structure.
 *******************************************/
/*
 * WT_PROCESS --
 *	Per-process information for the library.
 */
struct __wt_process {
	WT_SPINLOCK spinlock;		/* Per-process spinlock */

					/* Locked: connection list */
	TAILQ_HEAD(__wt_connection_impl_qh, __wt_connection_impl) connqh;
	WT_CACHE_POOL *cache_pool;
};
extern WT_PROCESS __wt_process;

/*
 * WT_NAMED_COLLATOR --
 *	A collator list entry
 */
struct __wt_named_collator {
	const char *name;		/* Name of collator */
	WT_COLLATOR *collator;		/* User supplied object */
	TAILQ_ENTRY(__wt_named_collator) q;	/* Linked list of collators */
};

/*
 * WT_NAMED_COMPRESSOR --
 *	A compressor list entry
 */
struct __wt_named_compressor {
	const char *name;		/* Name of compressor */
	WT_COMPRESSOR *compressor;	/* User supplied callbacks */
					/* Linked list of compressors */
	TAILQ_ENTRY(__wt_named_compressor) q;
};

/*
 * WT_NAMED_DATA_SOURCE --
 *	A data source list entry
 */
struct __wt_named_data_source {
	const char *prefix;		/* Name of data source */
	WT_DATA_SOURCE *dsrc;		/* User supplied callbacks */
					/* Linked list of data sources */
	TAILQ_ENTRY(__wt_named_data_source) q;
};

/*
 * WT_NAMED_EXTRACTOR --
 *	An extractor list entry
 */
struct __wt_named_extractor {
	const char *name;		/* Name of extractor */
	WT_EXTRACTOR *extractor;		/* User supplied object */
	TAILQ_ENTRY(__wt_named_extractor) q;	/* Linked list of extractors */
};

/*
 * Allocate some additional slots for internal sessions.  There is a default
 * session for each connection, plus a session for each server thread.
 */
#define	WT_NUM_INTERNAL_SESSIONS	10

/*
 * WT_CONN_CHECK_PANIC --
 *	Check if we've panicked and return the appropriate error.
 */
#define	WT_CONN_CHECK_PANIC(conn)					\
	(F_ISSET(conn, WT_CONN_PANIC) ? WT_PANIC : 0)
#define	WT_SESSION_CHECK_PANIC(session)					\
	WT_CONN_CHECK_PANIC(S2C(session))

/*
 * Macros to ensure the dhandle is inserted or removed from both the
 * main queue and the hashed queue.
 */
#define	WT_CONN_DHANDLE_INSERT(conn, dhandle, bucket) do {		\
	TAILQ_INSERT_HEAD(&(conn)->dhqh, dhandle, q);			\
	TAILQ_INSERT_HEAD(&(conn)->dhhash[bucket], dhandle, hashq);	\
	++conn->dhandle_count;						\
} while (0)

#define	WT_CONN_DHANDLE_REMOVE(conn, dhandle, bucket) do {		\
	TAILQ_REMOVE(&(conn)->dhqh, dhandle, q);			\
	TAILQ_REMOVE(&(conn)->dhhash[bucket], dhandle, hashq);		\
	--conn->dhandle_count;						\
} while (0)

/*
 * Macros to ensure the block is inserted or removed from both the
 * main queue and the hashed queue.
 */
#define	WT_CONN_BLOCK_INSERT(conn, block, bucket) do {			\
	TAILQ_INSERT_HEAD(&(conn)->blockqh, block, q);			\
	TAILQ_INSERT_HEAD(&(conn)->blockhash[bucket], block, hashq);	\
} while (0)

#define	WT_CONN_BLOCK_REMOVE(conn, block, bucket) do {			\
	TAILQ_REMOVE(&(conn)->blockqh, block, q);			\
	TAILQ_REMOVE(&(conn)->blockhash[bucket], block, hashq);		\
} while (0)

/*
 * Macros to ensure the file handle is inserted or removed from both the
 * main queue and the hashed queue.
 */
#define	WT_CONN_FILE_INSERT(conn, fh, bucket) do {			\
	TAILQ_INSERT_HEAD(&(conn)->fhqh, fh, q);			\
	TAILQ_INSERT_HEAD(&(conn)->fhhash[bucket], fh, hashq);		\
} while (0)

#define	WT_CONN_FILE_REMOVE(conn, fh, bucket) do {			\
	TAILQ_REMOVE(&(conn)->fhqh, fh, q);				\
	TAILQ_REMOVE(&(conn)->fhhash[bucket], fh, hashq);		\
} while (0)

/*
 * WT_CONNECTION_IMPL --
 *	Implementation of WT_CONNECTION
 */
struct __wt_connection_impl {
	WT_CONNECTION iface;

	/* For operations without an application-supplied session */
	WT_SESSION_IMPL *default_session;
	WT_SESSION_IMPL  dummy_session;

	const char *cfg;		/* Connection configuration */

	WT_SPINLOCK api_lock;		/* Connection API spinlock */
	WT_SPINLOCK checkpoint_lock;	/* Checkpoint spinlock */
	WT_SPINLOCK dhandle_lock;	/* Data handle list spinlock */
	WT_SPINLOCK fh_lock;		/* File handle queue spinlock */
	WT_SPINLOCK reconfig_lock;	/* Single thread reconfigure */
	WT_SPINLOCK schema_lock;	/* Schema operation spinlock */
	WT_SPINLOCK table_lock;		/* Table creation spinlock */

	/*
	 * We distribute the btree page locks across a set of spin locks; it
	 * can't be an array, we impose cache-line alignment and gcc doesn't
	 * support that for arrays.  Don't use too many: they are only held for
	 * very short operations, each one is 64 bytes, so 256 will fill the L1
	 * cache on most CPUs.
	 */
#define	WT_PAGE_LOCKS(conn)	16
	WT_SPINLOCK *page_lock;	        /* Btree page spinlocks */
	u_int	     page_lock_cnt;	/* Next spinlock to use */

					/* Connection queue */
	TAILQ_ENTRY(__wt_connection_impl) q;
					/* Cache pool queue */
	TAILQ_ENTRY(__wt_connection_impl) cpq;

	const char *home;		/* Database home */
	const char *error_prefix;	/* Database error prefix */
	int is_new;			/* Connection created database */

	WT_EXTENSION_API extension_api;	/* Extension API */

					/* Configuration */
	const WT_CONFIG_ENTRY **config_entries;

	void  **foc;			/* Free-on-close array */
	size_t  foc_cnt;		/* Array entries */
	size_t  foc_size;		/* Array size */

	WT_FH *lock_fh;			/* Lock file handle */

	volatile uint64_t  split_gen;	/* Generation number for splits */

	/*
	 * The connection keeps a cache of data handles. The set of handles
	 * can grow quite large so we maintain both a simple list and a hash
	 * table of lists. The hash table key is based on a hash of the table
	 * URI.
	 */
					/* Locked: data handle hash array */
#define	WT_HASH_ARRAY_SIZE	512
	TAILQ_HEAD(__wt_dhhash, __wt_data_handle) dhhash[WT_HASH_ARRAY_SIZE];
					/* Locked: data handle list */
	TAILQ_HEAD(__wt_dhandle_qh, __wt_data_handle) dhqh;
					/* Locked: LSM handle list. */
	TAILQ_HEAD(__wt_lsm_qh, __wt_lsm_tree) lsmqh;
					/* Locked: file list */
	TAILQ_HEAD(__wt_fhhash, __wt_fh) fhhash[WT_HASH_ARRAY_SIZE];
	TAILQ_HEAD(__wt_fh_qh, __wt_fh) fhqh;
					/* Locked: library list */
	TAILQ_HEAD(__wt_dlh_qh, __wt_dlh) dlhqh;

	WT_SPINLOCK block_lock;		/* Locked: block manager list */
	TAILQ_HEAD(__wt_blockhash, __wt_block) blockhash[WT_HASH_ARRAY_SIZE];
	TAILQ_HEAD(__wt_block_qh, __wt_block) blockqh;

	u_int dhandle_count;		/* Locked: handles in the queue */
	u_int open_btree_count;		/* Locked: open writable btree count */
	uint32_t next_file_id;		/* Locked: file ID counter */
	uint32_t open_file_count;	/* Atomic: open file handle count */

	/*
	 * WiredTiger allocates space for 50 simultaneous sessions (threads of
	 * control) by default.  Growing the number of threads dynamically is
	 * possible, but tricky since server threads are walking the array
	 * without locking it.
	 *
	 * There's an array of WT_SESSION_IMPL pointers that reference the
	 * allocated array; we do it that way because we want an easy way for
	 * the server thread code to avoid walking the entire array when only a
	 * few threads are running.
	 */
	WT_SESSION_IMPL	*sessions;	/* Session reference */
	uint32_t	 session_size;	/* Session array size */
	uint32_t	 session_cnt;	/* Session count */

	size_t     session_scratch_max;	/* Max scratch memory per session */

	/*
	 * WiredTiger allocates space for a fixed number of hazard pointers
	 * in each thread of control.
	 */
	uint32_t   hazard_max;		/* Hazard array size */

	WT_CACHE  *cache;		/* Page cache */
	uint64_t   cache_size;		/* Configured cache size */

	WT_TXN_GLOBAL txn_global;	/* Global transaction state */

	WT_SPINLOCK hot_backup_lock;	/* Hot backup serialization */
	int hot_backup;

	WT_SESSION_IMPL *ckpt_session;	/* Checkpoint thread session */
	wt_thread_t	 ckpt_tid;	/* Checkpoint thread */
	int		 ckpt_tid_set;	/* Checkpoint thread set */
	WT_CONDVAR	*ckpt_cond;	/* Checkpoint wait mutex */
	const char	*ckpt_config;	/* Checkpoint configuration */
#define	WT_CKPT_LOGSIZE(conn)	((conn)->ckpt_logsize != 0)
	wt_off_t	 ckpt_logsize;	/* Checkpoint log size period */
	uint32_t	 ckpt_signalled;/* Checkpoint signalled */
	uint64_t	 ckpt_usecs;	/* Checkpoint period */

	int compact_in_memory_pass;	/* Compaction serialization */

#define	WT_CONN_STAT_ALL	0x01	/* "all" statistics configured */
#define	WT_CONN_STAT_CLEAR	0x02	/* clear after gathering */
#define	WT_CONN_STAT_FAST	0x04	/* "fast" statistics configured */
#define	WT_CONN_STAT_NONE	0x08	/* don't gather statistics */
#define	WT_CONN_STAT_ON_CLOSE	0x10	/* output statistics on close */
#define	WT_CONN_STAT_SIZE	0x20	/* "size" statistics configured */
	uint32_t stat_flags;

	WT_CONNECTION_STATS stats;	/* Connection statistics */

#if SPINLOCK_TYPE == SPINLOCK_PTHREAD_MUTEX_LOGGING
	/*
	 * Spinlock registration, so we can track which spinlocks are heavily
	 * used, which are blocking and where.
	 *
	 * There's an array of spinlocks, and an array of blocking IDs.
	 */
#define	WT_SPINLOCK_MAX			1024
#define	WT_SPINLOCK_MAX_LOCATION_ID	60
	WT_SPINLOCK *spinlock_list[WT_SPINLOCK_MAX];

					/* Spinlock blocking matrix */
	struct __wt_connection_stats_spinlock {
		const char *name;	/* Mutex name */

		const char *file;	/* Caller's file/line, ID location */
		int line;

		u_int total;		/* Count of total, blocked calls */
		u_int blocked[WT_SPINLOCK_MAX_LOCATION_ID];
	} spinlock_block[WT_SPINLOCK_MAX_LOCATION_ID];
#endif

	WT_ASYNC	*async;		/* Async structure */
	int		 async_cfg;	/* Global async configuration */
	uint32_t	 async_size;	/* Async op array size */
	uint32_t	 async_workers;	/* Number of async workers */

	WT_LSM_MANAGER	lsm_manager;	/* LSM worker thread information */

	WT_SESSION_IMPL *evict_session; /* Eviction server sessions */
	wt_thread_t	 evict_tid;	/* Eviction server thread ID */
	int		 evict_tid_set;	/* Eviction server thread ID set */

	uint32_t	 evict_workers_alloc;/* Allocated eviction workers */
	uint32_t	 evict_workers_max;/* Max eviction workers */
	uint32_t	 evict_workers_min;/* Min eviction workers */
	uint32_t	 evict_workers;	/* Number of eviction workers */
	WT_EVICT_WORKER	*evict_workctx;	/* Eviction worker context */

	WT_SESSION_IMPL *stat_session;	/* Statistics log session */
	wt_thread_t	 stat_tid;	/* Statistics log thread */
	int		 stat_tid_set;	/* Statistics log thread set */
	WT_CONDVAR	*stat_cond;	/* Statistics log wait mutex */
	const char	*stat_format;	/* Statistics log timestamp format */
	FILE		*stat_fp;	/* Statistics log file handle */
	char		*stat_path;	/* Statistics log path format */
	char	       **stat_sources;	/* Statistics log list of objects */
	const char	*stat_stamp;	/* Statistics log entry timestamp */
	uint64_t	 stat_usecs;	/* Statistics log period */

#define	WT_CONN_LOG_ARCHIVE	0x01	/* Archive is enabled */
#define	WT_CONN_LOG_ENABLED	0x02	/* Logging is enabled */
#define	WT_CONN_LOG_EXISTED	0x04	/* Log files found */
#define	WT_CONN_LOG_PREALLOC	0x08	/* Pre-allocation is enabled */
#define	WT_CONN_LOG_RECOVER_ERR	0x10	/* Error if recovery required */
	uint32_t	 log_flags;	/* Global logging configuration */
	WT_CONDVAR	*log_cond;	/* Log server wait mutex */
	WT_SESSION_IMPL *log_session;	/* Log server session */
	wt_thread_t	 log_tid;	/* Log server thread */
	int		 log_tid_set;	/* Log server thread set */
	WT_CONDVAR	*log_file_cond; /* Log file thread wait mutex */
	WT_SESSION_IMPL *log_file_session;/* Log file thread session */
	wt_thread_t	 log_file_tid;	/* Log file thread thread */
	int		 log_file_tid_set;/* Log file thread set */
	WT_CONDVAR	*log_wrlsn_cond;/* Log write lsn thread wait mutex */
	WT_SESSION_IMPL *log_wrlsn_session;/* Log write lsn thread session */
	wt_thread_t	 log_wrlsn_tid;	/* Log write lsn thread thread */
	int		 log_wrlsn_tid_set;/* Log write lsn thread set */
	WT_LOG		*log;		/* Logging structure */
	WT_COMPRESSOR	*log_compressor;/* Logging compressor */
	wt_off_t	 log_file_max;	/* Log file max size */
	const char	*log_path;	/* Logging path format */
	uint32_t	 log_prealloc;	/* Log file pre-allocation */
	uint32_t	 txn_logsync;	/* Log sync configuration */

	WT_SESSION_IMPL *sweep_session;	/* Handle sweep session */
	wt_thread_t	 sweep_tid;	/* Handle sweep thread */
	int		 sweep_tid_set;	/* Handle sweep thread set */
	WT_CONDVAR	*sweep_cond;	/* Handle sweep wait mutex */
	time_t		 sweep_idle_time;/* Handle sweep idle time */
	time_t		 sweep_interval;/* Handle sweep interval */
	u_int		 sweep_handles_min;/* Handle sweep minimum open */

					/* Locked: collator list */
	TAILQ_HEAD(__wt_coll_qh, __wt_named_collator) collqh;

					/* Locked: compressor list */
	TAILQ_HEAD(__wt_comp_qh, __wt_named_compressor) compqh;

					/* Locked: data source list */
	TAILQ_HEAD(__wt_dsrc_qh, __wt_named_data_source) dsrcqh;

					/* Locked: extractor list */
	TAILQ_HEAD(__wt_extractor_qh, __wt_named_extractor) extractorqh;

	void	*lang_private;		/* Language specific private storage */

	/* If non-zero, all buffers used for I/O will be aligned to this. */
	size_t buffer_alignment;

	uint32_t schema_gen;		/* Schema generation number */

	wt_off_t data_extend_len;	/* file_extend data length */
	wt_off_t log_extend_len;	/* file_extend log length */

	uint32_t direct_io;		/* O_DIRECT file type flags */
	int	 mmap;			/* mmap configuration */
	uint32_t verbose;

	uint32_t flags;
};
