/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

static int __backup_all(WT_SESSION_IMPL *, WT_CURSOR_BACKUP *);
static int __backup_cleanup_handles(WT_SESSION_IMPL *, WT_CURSOR_BACKUP *);
static int __backup_file_create(WT_SESSION_IMPL *, WT_CURSOR_BACKUP *, int);
static int __backup_list_all_append(WT_SESSION_IMPL *, const char *[]);
static int __backup_list_append(
    WT_SESSION_IMPL *, WT_CURSOR_BACKUP *, const char *);
static int __backup_start(
    WT_SESSION_IMPL *, WT_CURSOR_BACKUP *, const char *[]);
static int __backup_stop(WT_SESSION_IMPL *);
static int __backup_uri(
    WT_SESSION_IMPL *, WT_CURSOR_BACKUP *, const char *[], int *, int *);

/*
 * __curbackup_next --
 *	WT_CURSOR->next method for the backup cursor type.
 */
static int
__curbackup_next(WT_CURSOR *cursor)
{
	WT_CURSOR_BACKUP *cb;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;

	cb = (WT_CURSOR_BACKUP *)cursor;
	CURSOR_API_CALL(cursor, session, next, NULL);

	if (cb->list == NULL || cb->list[cb->next].name == NULL) {
		F_CLR(cursor, WT_CURSTD_KEY_SET);
		WT_ERR(WT_NOTFOUND);
	}

	cb->iface.key.data = cb->list[cb->next].name;
	cb->iface.key.size = strlen(cb->list[cb->next].name) + 1;
	++cb->next;

	F_SET(cursor, WT_CURSTD_KEY_INT);

err:	API_END_RET(session, ret);
}

/*
 * __curbackup_reset --
 *	WT_CURSOR->reset method for the backup cursor type.
 */
static int
__curbackup_reset(WT_CURSOR *cursor)
{
	WT_CURSOR_BACKUP *cb;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;

	cb = (WT_CURSOR_BACKUP *)cursor;
	CURSOR_API_CALL(cursor, session, reset, NULL);

	cb->next = 0;
	F_CLR(cursor, WT_CURSTD_KEY_SET | WT_CURSTD_VALUE_SET);

err:	API_END_RET(session, ret);
}

/*
 * __curbackup_close --
 *	WT_CURSOR->close method for the backup cursor type.
 */
static int
__curbackup_close(WT_CURSOR *cursor)
{
	WT_CURSOR_BACKUP *cb;
	WT_DECL_RET;
	WT_SESSION_IMPL *session;
	int tret;

	cb = (WT_CURSOR_BACKUP *)cursor;
	CURSOR_API_CALL(cursor, session, close, NULL);

	WT_TRET(__backup_cleanup_handles(session, cb));
	WT_TRET(__wt_cursor_close(cursor));
	session->bkp_cursor = NULL;

	WT_WITH_SCHEMA_LOCK(session,
	    tret = __backup_stop(session));		/* Stop the backup. */
	WT_TRET(tret);

err:	API_END_RET(session, ret);
}

/*
 * __wt_curbackup_open --
 *	WT_SESSION->open_cursor method for the backup cursor type.
 */
int
__wt_curbackup_open(WT_SESSION_IMPL *session,
    const char *uri, const char *cfg[], WT_CURSOR **cursorp)
{
	WT_CURSOR_STATIC_INIT(iface,
	    __wt_cursor_get_key,	/* get-key */
	    __wt_cursor_notsup,		/* get-value */
	    __wt_cursor_notsup,		/* set-key */
	    __wt_cursor_notsup,		/* set-value */
	    __wt_cursor_notsup,		/* compare */
	    __wt_cursor_notsup,		/* equals */
	    __curbackup_next,		/* next */
	    __wt_cursor_notsup,		/* prev */
	    __curbackup_reset,		/* reset */
	    __wt_cursor_notsup,		/* search */
	    __wt_cursor_notsup,		/* search-near */
	    __wt_cursor_notsup,		/* insert */
	    __wt_cursor_notsup,		/* update */
	    __wt_cursor_notsup,		/* remove */
	    __wt_cursor_notsup,		/* reconfigure */
	    __curbackup_close);		/* close */
	WT_CURSOR *cursor;
	WT_CURSOR_BACKUP *cb;
	WT_DECL_RET;

	WT_STATIC_ASSERT(offsetof(WT_CURSOR_BACKUP, iface) == 0);

	cb = NULL;

	WT_RET(__wt_calloc_one(session, &cb));
	cursor = &cb->iface;
	*cursor = iface;
	cursor->session = &session->iface;
	session->bkp_cursor = cb;

	cursor->key_format = "S";	/* Return the file names as the key. */
	cursor->value_format = "";	/* No value. */

	/*
	 * Start the backup and fill in the cursor's list.  Acquire the schema
	 * lock, we need a consistent view when creating a copy.
	 */
	WT_WITH_SCHEMA_LOCK(session, ret = __backup_start(session, cb, cfg));
	WT_ERR(ret);

	/* __wt_cursor_init is last so we don't have to clean up on error. */
	WT_ERR(__wt_cursor_init(cursor, uri, NULL, cfg, cursorp));

	if (0) {
err:		__wt_free(session, cb);
	}

	return (ret);
}

/*
 * __backup_log_append --
 *	Append log files needed for backup.
 */
static int
__backup_log_append(WT_SESSION_IMPL *session, WT_CURSOR_BACKUP *cb, int active)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	u_int i, logcount;
	char **logfiles;

	conn = S2C(session);
	logfiles = NULL;
	logcount = 0;
	ret = 0;

	if (conn->log) {
		WT_ERR(__wt_log_get_all_files(
		    session, &logfiles, &logcount, &cb->maxid, active));
		for (i = 0; i < logcount; i++)
			WT_ERR(__backup_list_append(session, cb, logfiles[i]));
	}
err:	if (logfiles != NULL)
		__wt_log_files_free(session, logfiles, logcount);
	return (ret);
}

/*
 * __backup_start --
 *	Start a backup.
 */
static int
__backup_start(
    WT_SESSION_IMPL *session, WT_CURSOR_BACKUP *cb, const char *cfg[])
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;
	int exist, log_only, target_list;

	conn = S2C(session);

	cb->next = 0;
	cb->list = NULL;

	/*
	 * Single thread hot backups: we're holding the schema lock, so we
	 * know we'll serialize with other attempts to start a hot backup.
	 */
	if (conn->hot_backup)
		WT_RET_MSG(
		    session, EINVAL, "there is already a backup cursor open");

	/*
	 * The hot backup copy is done outside of WiredTiger, which means file
	 * blocks can't be freed and re-allocated until the backup completes.
	 * The checkpoint code checks the backup flag, and if a backup cursor
	 * is open checkpoints aren't discarded.   We release the lock as soon
	 * as we've set the flag, we don't want to block checkpoints, we just
	 * want to make sure no checkpoints are deleted.  The checkpoint code
	 * holds the lock until it's finished the checkpoint, otherwise we
	 * could start a hot backup that would race with an already-started
	 * checkpoint.
	 */
	__wt_spin_lock(session, &conn->hot_backup_lock);
	conn->hot_backup = 1;
	__wt_spin_unlock(session, &conn->hot_backup_lock);

	/* Create the hot backup file. */
	WT_ERR(__backup_file_create(session, cb, 0));

	/* Add log files if logging is enabled. */

	/*
	 * If a list of targets was specified, work our way through them.
	 * Else, generate a list of all database objects.
	 *
	 * Include log files if doing a full backup, and copy them before
	 * copying data files to avoid rolling the metadata forward across
	 * a checkpoint that completes during the backup.
	 */
	target_list = 0;
	WT_ERR(__backup_uri(session, cb, cfg, &target_list, &log_only));

	if (!target_list) {
		WT_ERR(__backup_log_append(session, cb, 1));
		WT_ERR(__backup_all(session, cb));
	}

	/* Add the hot backup and standard WiredTiger files to the list. */
	if (log_only) {
		/*
		 * Close any hot backup file.
		 * We're about to open the incremental backup file.
		 */
		WT_TRET(__wt_fclose(session, &cb->bfp, WT_FHANDLE_WRITE));
		WT_ERR(__backup_file_create(session, cb, log_only));
		WT_ERR(__backup_list_append(
		    session, cb, WT_INCREMENTAL_BACKUP));
	} else {
		WT_ERR(__backup_list_append(session, cb, WT_METADATA_BACKUP));
		WT_ERR(__wt_exist(session, WT_BASECONFIG, &exist));
		if (exist)
			WT_ERR(__backup_list_append(
			    session, cb, WT_BASECONFIG));
		WT_ERR(__wt_exist(session, WT_USERCONFIG, &exist));
		if (exist)
			WT_ERR(__backup_list_append(
			    session, cb, WT_USERCONFIG));
		WT_ERR(__backup_list_append(session, cb, WT_WIREDTIGER));
	}

err:	/* Close the hot backup file. */
	WT_TRET(__wt_fclose(session, &cb->bfp, WT_FHANDLE_WRITE));
	if (ret != 0) {
		WT_TRET(__backup_cleanup_handles(session, cb));
		WT_TRET(__backup_stop(session));
	}

	return (ret);
}

/*
 * __backup_cleanup_handles --
 *	Release and free all btree handles held by the backup. This is kept
 *	separate from __backup_stop because it can be called without the
 *	schema lock held.
 */
static int
__backup_cleanup_handles(WT_SESSION_IMPL *session, WT_CURSOR_BACKUP *cb)
{
	WT_CURSOR_BACKUP_ENTRY *p;
	WT_DECL_RET;

	if (cb->list == NULL)
		return (0);

	/* Release the handles, free the file names, free the list itself. */
	for (p = cb->list; p->name != NULL; ++p) {
		if (p->handle != NULL)
			WT_WITH_DHANDLE(session, p->handle,
			    WT_TRET(__wt_session_release_btree(session)));
		__wt_free(session, p->name);
	}

	__wt_free(session, cb->list);
	return (ret);
}

/*
 * __backup_stop --
 *	Stop a backup.
 */
static int
__backup_stop(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	WT_DECL_RET;

	conn = S2C(session);

	/* Remove any backup specific file. */
	ret = __wt_backup_file_remove(session);

	/* Checkpoint deletion can proceed, as can the next hot backup. */
	__wt_spin_lock(session, &conn->hot_backup_lock);
	conn->hot_backup = 0;
	__wt_spin_unlock(session, &conn->hot_backup_lock);

	return (ret);
}

/*
 * __backup_all --
 *	Backup all objects in the database.
 */
static int
__backup_all(WT_SESSION_IMPL *session, WT_CURSOR_BACKUP *cb)
{
	WT_CONFIG_ITEM cval;
	WT_CURSOR *cursor;
	WT_DECL_RET;
	const char *key, *value;

	cursor = NULL;

	/*
	 * Open a cursor on the metadata file and copy all of the entries to
	 * the hot backup file.
	 */
	WT_ERR(__wt_metadata_cursor(session, NULL, &cursor));
	while ((ret = cursor->next(cursor)) == 0) {
		WT_ERR(cursor->get_key(cursor, &key));
		WT_ERR(cursor->get_value(cursor, &value));
		WT_ERR(__wt_fprintf(session, cb->bfp, "%s\n%s\n", key, value));

		/*
		 * While reading the metadata file, check there are no "sources"
		 * or "types" which can't support hot backup.  This checks for
		 * a data source that's non-standard, which can't be backed up,
		 * but is also sanity checking: if there's an entry backed by
		 * anything other than a file or lsm entry, we're confused.
		 */
		if ((ret = __wt_config_getones(
		    session, value, "type", &cval)) == 0 &&
		    !WT_PREFIX_MATCH_LEN(cval.str, cval.len, "file") &&
		    !WT_PREFIX_MATCH_LEN(cval.str, cval.len, "lsm"))
			WT_ERR_MSG(session, ENOTSUP,
			    "hot backup is not supported for objects of "
			    "type %.*s", (int)cval.len, cval.str);
		WT_ERR_NOTFOUND_OK(ret);
		if ((ret =__wt_config_getones(
		    session, value, "source", &cval)) == 0 &&
		    !WT_PREFIX_MATCH_LEN(cval.str, cval.len, "file:") &&
		    !WT_PREFIX_MATCH_LEN(cval.str, cval.len, "lsm:"))
			WT_ERR_MSG(session, ENOTSUP,
			    "hot backup is not supported for objects of "
			    "source %.*s", (int)cval.len, cval.str);
		WT_ERR_NOTFOUND_OK(ret);
	}
	WT_ERR_NOTFOUND_OK(ret);

	/* Build a list of the file objects that need to be copied. */
	WT_WITH_DHANDLE_LOCK(session,
	    ret = __wt_meta_btree_apply(
	    session, __backup_list_all_append, NULL));

err:	if (cursor != NULL)
		WT_TRET(cursor->close(cursor));
	return (ret);
}

/*
 * __backup_uri --
 *	Backup a list of objects.
 */
static int
__backup_uri(WT_SESSION_IMPL *session,
    WT_CURSOR_BACKUP *cb, const char *cfg[], int *foundp, int *log_only)
{
	WT_CONFIG targetconf;
	WT_CONFIG_ITEM cval, k, v;
	WT_DECL_ITEM(tmp);
	WT_DECL_RET;
	int target_list;
	const char *uri;

	*foundp = 0;
	*log_only = 0;

	/*
	 * If we find a non-empty target configuration string, we have a job,
	 * otherwise it's not our problem.
	 */
	WT_RET(__wt_config_gets(session, cfg, "target", &cval));
	WT_RET(__wt_config_subinit(session, &targetconf, &cval));
	for (cb->list_next = 0, target_list = 0;
	    (ret = __wt_config_next(&targetconf, &k, &v)) == 0; ++target_list) {
		/* If it is our first time through, allocate. */
		if (target_list == 0) {
			*foundp = 1;
			WT_ERR(__wt_scr_alloc(session, 512, &tmp));
		}

		WT_ERR(__wt_buf_fmt(session, tmp, "%.*s", (int)k.len, k.str));
		uri = tmp->data;
		if (v.len != 0)
			WT_ERR_MSG(session, EINVAL,
			    "%s: invalid backup target: URIs may need quoting",
			    uri);

		/*
		 * Handle log targets.  We do not need to go through the
		 * schema worker, just call the function to append them.
		 * Set log_only only if it is our only URI target.
		 */
		if (WT_PREFIX_MATCH(uri, "log:")) {
			if (target_list == 0)
				*log_only = 1;
			else
				*log_only = 0;
			WT_ERR(__wt_backup_list_uri_append(
			    session, uri, NULL));
		} else
			WT_ERR(__wt_schema_worker(session,
			    uri, NULL, __wt_backup_list_uri_append, cfg, 0));
	}
	WT_ERR_NOTFOUND_OK(ret);

err:	__wt_scr_free(session, &tmp);
	return (ret);
}

/*
 * __backup_file_create --
 *	Create the meta-data backup file.
 */
static int
__backup_file_create(
    WT_SESSION_IMPL *session, WT_CURSOR_BACKUP *cb, int incremental)
{
	return (__wt_fopen(session,
	    incremental ? WT_INCREMENTAL_BACKUP : WT_METADATA_BACKUP,
	    WT_FHANDLE_WRITE, 0, &cb->bfp));
}

/*
 * __wt_backup_file_remove --
 *	Remove the incremental and meta-data backup files.
 */
int
__wt_backup_file_remove(WT_SESSION_IMPL *session)
{
	WT_DECL_RET;

	WT_TRET(__wt_remove_if_exists(session, WT_INCREMENTAL_BACKUP));
	WT_TRET(__wt_remove_if_exists(session, WT_METADATA_BACKUP));
	return (ret);
}

/*
 * __wt_backup_list_uri_append --
 *	Append a new file name to the list, allocate space as necessary.
 *	Called via the schema_worker function.
 */
int
__wt_backup_list_uri_append(
    WT_SESSION_IMPL *session, const char *name, int *skip)
{
	WT_CURSOR_BACKUP *cb;
	char *value;

	cb = session->bkp_cursor;
	WT_UNUSED(skip);

	if (WT_PREFIX_MATCH(name, "log:")) {
		WT_RET(__backup_log_append(session, cb, 0));
		return (0);
	}

	/* Add the metadata entry to the backup file. */
	WT_RET(__wt_metadata_search(session, name, &value));
	WT_RET(__wt_fprintf(session, cb->bfp, "%s\n%s\n", name, value));
	__wt_free(session, value);

	/* Add file type objects to the list of files to be copied. */
	if (WT_PREFIX_MATCH(name, "file:"))
		WT_RET(__backup_list_append(session, cb, name));

	return (0);
}

/*
 * __backup_list_all_append --
 *	Append a new file name to the list, allocate space as necessary.
 *	Called via the __wt_meta_btree_apply function.
 */
static int
__backup_list_all_append(WT_SESSION_IMPL *session, const char *cfg[])
{
	WT_CURSOR_BACKUP *cb;

	WT_UNUSED(cfg);

	cb = session->bkp_cursor;

	/* Ignore files in the process of being bulk-loaded. */
	if (F_ISSET(S2BT(session), WT_BTREE_BULK))
		return (0);

	/* Add the file to the list of files to be copied. */
	return (__backup_list_append(session, cb, session->dhandle->name));
}

/*
 * __backup_list_append --
 *	Append a new file name to the list, allocate space as necessary.
 */
static int
__backup_list_append(
    WT_SESSION_IMPL *session, WT_CURSOR_BACKUP *cb, const char *uri)
{
	WT_CURSOR_BACKUP_ENTRY *p;
	WT_DATA_HANDLE *old_dhandle;
	WT_DECL_RET;
	const char *name;
	int need_handle;

	/* Leave a NULL at the end to mark the end of the list. */
	WT_RET(__wt_realloc_def(session, &cb->list_allocated,
	    cb->list_next + 2, &cb->list));
	p = &cb->list[cb->list_next];
	p[0].name = p[1].name = NULL;
	p[0].handle = p[1].handle = NULL;

	need_handle = 0;
	name = uri;
	if (WT_PREFIX_MATCH(uri, "file:")) {
		need_handle = 1;
		name += strlen("file:");
	}

	/*
	 * !!!
	 * Assumes metadata file entries map one-to-one to physical files.
	 * To support a block manager where that's not the case, we'd need
	 * to call into the block manager and get a list of physical files
	 * that map to this logical "file".  I'm not going to worry about
	 * that for now, that block manager might not even support physical
	 * copying of files by applications.
	 */
	WT_RET(__wt_strdup(session, name, &p->name));

	/*
	 * If it's a file in the database, get a handle for the underlying
	 * object (this handle blocks schema level operations, for example
	 * WT_SESSION.drop or an LSM file discard after level merging).
	 */
	if (need_handle) {
		old_dhandle = session->dhandle;
		if ((ret =
		    __wt_session_get_btree(session, uri, NULL, NULL, 0)) == 0)
			p->handle = session->dhandle;
		session->dhandle = old_dhandle;
		WT_RET(ret);
	}

	++cb->list_next;
	return (0);
}
