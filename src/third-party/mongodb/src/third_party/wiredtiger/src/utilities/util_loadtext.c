/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "util.h"

static int insert(WT_CURSOR *, const char *, int);
static int text(WT_SESSION *, const char *);
static int usage(void);

int
util_loadtext(WT_SESSION *session, int argc, char *argv[])
{
	int ch;
	const char *uri;

	while ((ch = __wt_getopt(progname, argc, argv, "f:")) != EOF)
		switch (ch) {
		case 'f':	/* input file */
			if (freopen(__wt_optarg, "r", stdin) == NULL)
				return (util_err(
				    session, errno, "%s: reopen", __wt_optarg));
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= __wt_optind;
	argv += __wt_optind;

	/* The remaining argument is the uri. */
	if (argc != 1)
		return (usage());
	if ((uri = util_name(session, *argv, "table")) == NULL)
		return (1);

	return (text(session, uri));
}

/*
 * text --
 *	Load flat-text into a file/table.
 */
static int
text(WT_SESSION *session, const char *uri)
{
	WT_CURSOR *cursor;
	WT_DECL_RET;
	int readkey, tret;

	/*
	 * Open the cursor, configured to append new records (in the case of
	 * column-store objects), or to overwrite existing strings (in the
	 * case of row-store objects).  The two flags are mutually exclusive,
	 * but the library doesn't currently care that we set both of them.
	 */
	if ((ret = session->open_cursor(
	    session, uri, NULL, "append,overwrite", &cursor)) != 0)
		return (util_err(session, ret, "%s: session.open", uri));

	/*
	 * We're about to load strings, make sure the formats match.
	 *
	 * Row-store tables have key/value pairs, column-store tables only have
	 * values.
	 */
	if (strcmp(cursor->value_format, "S") != 0 ||
	    (strcmp(cursor->key_format, "S") != 0 &&
	    strcmp(cursor->key_format, "r") != 0))
		return (util_err(session, EINVAL,
		    "the loadtext command can only load objects configured "
		    "for record number or string keys, and string values"));
	readkey = strcmp(cursor->key_format, "r") == 0 ? 0 : 1;

	/* Insert the records */
	ret = insert(cursor, uri, readkey);

	/*
	 * Technically, we don't have to close the cursor because the session
	 * handle will do it for us, but I'd like to see the flush to disk and
	 * the close succeed, it's better to fail early when loading files.
	 */
	if ((tret = cursor->close(cursor)) != 0) {
		tret = util_err(session, tret, "%s: cursor.close", uri);
		if (ret == 0)
			ret = tret;
	}
	if (ret == 0)
		ret = util_flush(session, uri);

	return (ret == 0 ? 0 : 1);
}

/*
 * insert --
 *	Read and insert data.
 */
static int
insert(WT_CURSOR *cursor, const char *name, int readkey)
{
	ULINE key, value;
	WT_DECL_RET;
	WT_SESSION *session;
	uint64_t insert_count;
	int eof;

	session = cursor->session;

	memset(&key, 0, sizeof(key));
	memset(&value, 0, sizeof(value));

	/* Read key/value pairs and insert them into the file. */
	for (insert_count = 0;;) {
		/*
		 * Three modes: in row-store, we always read a key and use it,
		 * in column-store, we might read it (a dump), we might read
		 * and ignore it (a dump with "append" set), or not read it at
		 * all (flat-text load).
		 */
		if (readkey) {
			if (util_read_line(session, &key, 1, &eof))
				return (1);
			if (eof == 1)
				break;
			cursor->set_key(cursor, key.mem);
		}
		if (util_read_line(session, &value, readkey ? 0 : 1, &eof))
			return (1);
		if (eof == 1)
			break;
		cursor->set_value(cursor, value.mem);

		if ((ret = cursor->insert(cursor)) != 0)
			return (
			    util_err(session, ret, "%s: cursor.insert", name));

		/* Report on progress every 100 inserts. */
		if (verbose && ++insert_count % 100 == 0) {
			printf("\r\t%s: %" PRIu64, name, insert_count);
			fflush(stdout);
		}
	}

	if (verbose)
		printf("\r\t%s: %" PRIu64 "\n", name, insert_count);

	return (0);
}

static int
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s %s "
	    "loadtext [-f input-file] uri\n",
	    progname, usage_prefix);
	return (1);
}
