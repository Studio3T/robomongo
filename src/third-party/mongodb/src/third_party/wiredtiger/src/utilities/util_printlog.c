/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "util.h"

static int usage(void);

int
util_printlog(WT_SESSION *session, int argc, char *argv[])
{
	WT_DECL_RET;
	int ch, printable;

	printable = 0;
	while ((ch = __wt_getopt(progname, argc, argv, "f:p")) != EOF)
		switch (ch) {
		case 'f':			/* output file */
			if (freopen(__wt_optarg, "w", stdout) == NULL) {
				fprintf(stderr, "%s: %s: reopen: %s\n",
				    progname, __wt_optarg, strerror(errno));
				return (1);
			}
			break;
		case 'p':
			printable = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= __wt_optind;
	argv += __wt_optind;

	/* There should not be any more arguments. */
	if (argc != 0)
		return (usage());

	WT_UNUSED(printable);
	ret = __wt_txn_printlog(session, stdout);

	if (ret != 0) {
		fprintf(stderr, "%s: printlog failed: %s\n",
		    progname, session->strerror(session, ret));
		goto err;
	}

	if (0) {
err:		ret = 1;
	}
	return (ret);
}

static int
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s %s "
	    "printlog [-p] [-f output-file]\n",
	    progname, usage_prefix);
	return (1);
}
