/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

static void __bm_method_set(WT_BM *, int);

/*
 * __bm_readonly --
 *	General-purpose "writes not supported on this handle" function.
 */
static int
__bm_readonly(WT_BM *bm, WT_SESSION_IMPL *session)
{
	WT_RET_MSG(session, ENOTSUP,
	    "%s: write operation on read-only checkpoint handle",
	    bm->block->name);
}

/*
 * __bm_addr_string --
 *	Return a printable string representation of an address cookie.
 */
static int
__bm_addr_string(WT_BM *bm, WT_SESSION_IMPL *session,
    WT_ITEM *buf, const uint8_t *addr, size_t addr_size)
{
	return (
	    __wt_block_addr_string(session, bm->block, buf, addr, addr_size));
}

/*
 * __bm_addr_valid --
 *	Return if an address cookie is valid.
 */
static int
__bm_addr_valid(WT_BM *bm,
    WT_SESSION_IMPL *session, const uint8_t *addr, size_t addr_size)
{
	return (__wt_block_addr_valid(
	    session, bm->block, addr, addr_size, bm->is_live));
}

/*
 * __bm_block_header --
 *	Return the size of the block header.
 */
static u_int
__bm_block_header(WT_BM *bm)
{
	return (__wt_block_header(bm->block));
}

/*
 * __bm_checkpoint --
 *	Write a buffer into a block, creating a checkpoint.
 */
static int
__bm_checkpoint(WT_BM *bm,
    WT_SESSION_IMPL *session, WT_ITEM *buf, WT_CKPT *ckptbase, int data_cksum)
{
	return (__wt_block_checkpoint(
	    session, bm->block, buf, ckptbase, data_cksum));
}

/*
 * __bm_sync --
 *	Flush a file to disk.
 */
static int
__bm_sync(WT_BM *bm, WT_SESSION_IMPL *session, int async)
{
	return (async ?
	    __wt_fsync_async(session, bm->block->fh) :
	    __wt_fsync(session, bm->block->fh));
}

/*
 * __bm_checkpoint_load --
 *	Load a checkpoint.
 */
static int
__bm_checkpoint_load(WT_BM *bm, WT_SESSION_IMPL *session,
    const uint8_t *addr, size_t addr_size,
    uint8_t *root_addr, size_t *root_addr_sizep, int checkpoint)
{
	WT_CONNECTION_IMPL *conn;

	conn = S2C(session);

	/* If not opening a checkpoint, we're opening the live system. */
	bm->is_live = !checkpoint;
	WT_RET(__wt_block_checkpoint_load(session, bm->block,
	    addr, addr_size, root_addr, root_addr_sizep, checkpoint));

	if (checkpoint) {
		/*
		 * Read-only objects are optionally mapped into memory instead
		 * of being read into cache buffers.
		 */
		if (conn->mmap)
			WT_RET(__wt_block_map(session, bm->block,
			    &bm->map, &bm->maplen, &bm->mappingcookie));

		/*
		 * If this handle is for a checkpoint, that is, read-only, there
		 * isn't a lot you can do with it.  Although the btree layer
		 * prevents attempts to write a checkpoint reference, paranoia
		 * is healthy.
		 */
		__bm_method_set(bm, 1);
	}

	return (0);
}

/*
 * __bm_checkpoint_resolve --
 *	Resolve the checkpoint.
 */
static int
__bm_checkpoint_resolve(WT_BM *bm, WT_SESSION_IMPL *session)
{
	return (__wt_block_checkpoint_resolve(session, bm->block));
}

/*
 * __bm_checkpoint_unload --
 *	Unload a checkpoint point.
 */
static int
__bm_checkpoint_unload(WT_BM *bm, WT_SESSION_IMPL *session)
{
	WT_DECL_RET;

	/* Unmap any mapped segment. */
	if (bm->map != NULL)
		WT_TRET(__wt_block_unmap(session,
		    bm->block, bm->map, bm->maplen, &bm->mappingcookie));

	/* Unload the checkpoint. */
	WT_TRET(__wt_block_checkpoint_unload(session, bm->block, !bm->is_live));

	return (ret);
}

/*
 * __bm_close --
 *	Close a file.
 */
static int
__bm_close(WT_BM *bm, WT_SESSION_IMPL *session)
{
	WT_DECL_RET;

	if (bm == NULL)				/* Safety check */
		return (0);

	ret = __wt_block_close(session, bm->block);

	__wt_overwrite_and_free(session, bm);
	return (ret);
}

/*
 * __bm_compact_start --
 *	Start a block manager compaction.
 */
static int
__bm_compact_start(WT_BM *bm, WT_SESSION_IMPL *session)
{
	return (__wt_block_compact_start(session, bm->block));
}

/*
 * __bm_compact_page_skip --
 *	Return if a page is useful for compaction.
 */
static int
__bm_compact_page_skip(WT_BM *bm, WT_SESSION_IMPL *session,
    const uint8_t *addr, size_t addr_size, int *skipp)
{
	return (__wt_block_compact_page_skip(
	    session, bm->block, addr, addr_size, skipp));
}

/*
 * __bm_compact_skip --
 *	Return if a file can be compacted.
 */
static int
__bm_compact_skip(WT_BM *bm, WT_SESSION_IMPL *session, int *skipp)
{
	return (__wt_block_compact_skip(session, bm->block, skipp));
}

/*
 * __bm_compact_end --
 *	End a block manager compaction.
 */
static int
__bm_compact_end(WT_BM *bm, WT_SESSION_IMPL *session)
{
	return (__wt_block_compact_end(session, bm->block));
}

/*
 * __bm_free --
 *	Free a block of space to the underlying file.
 */
static int
__bm_free(WT_BM *bm,
    WT_SESSION_IMPL *session, const uint8_t *addr, size_t addr_size)
{
	return (__wt_block_free(session, bm->block, addr, addr_size));
}

/*
 * __bm_stat --
 *	Block-manager statistics.
 */
static int
__bm_stat(WT_BM *bm, WT_SESSION_IMPL *session, WT_DSRC_STATS *stats)
{
	__wt_block_stat(session, bm->block, stats);
	return (0);
}

/*
 * __bm_write --
 *	Write a buffer into a block, returning the block's address cookie.
 */
static int
__bm_write(WT_BM *bm, WT_SESSION_IMPL *session,
    WT_ITEM *buf, uint8_t *addr, size_t *addr_sizep, int data_cksum)
{
	return (__wt_block_write(
	    session, bm->block, buf, addr, addr_sizep, data_cksum));
}

/*
 * __bm_write_size --
 *	Return the buffer size required to write a block.
 */
static int
__bm_write_size(WT_BM *bm, WT_SESSION_IMPL *session, size_t *sizep)
{
	return (__wt_block_write_size(session, bm->block, sizep));
}

/*
 * __bm_salvage_start --
 *	Start a block manager salvage.
 */
static int
__bm_salvage_start(WT_BM *bm, WT_SESSION_IMPL *session)
{
	return (__wt_block_salvage_start(session, bm->block));
}

/*
 * __bm_salvage_valid --
 *	Inform salvage a block is valid.
 */
static int
__bm_salvage_valid(WT_BM *bm,
    WT_SESSION_IMPL *session, uint8_t *addr, size_t addr_size, int valid)
{
	return (__wt_block_salvage_valid(
	    session, bm->block, addr, addr_size, valid));
}

/*
 * __bm_salvage_next --
 *	Return the next block from the file.
 */
static int
__bm_salvage_next(WT_BM *bm,
    WT_SESSION_IMPL *session, uint8_t *addr, size_t *addr_sizep, int *eofp)
{
	return (__wt_block_salvage_next(
	    session, bm->block, addr, addr_sizep, eofp));
}

/*
 * __bm_salvage_end --
 *	End a block manager salvage.
 */
static int
__bm_salvage_end(WT_BM *bm, WT_SESSION_IMPL *session)
{
	return (__wt_block_salvage_end(session, bm->block));
}

/*
 * __bm_verify_start --
 *	Start a block manager verify.
 */
static int
__bm_verify_start(WT_BM *bm,
    WT_SESSION_IMPL *session, WT_CKPT *ckptbase, const char *cfg[])
{
	return (__wt_block_verify_start(session, bm->block, ckptbase, cfg));
}

/*
 * __bm_verify_addr --
 *	Verify an address.
 */
static int
__bm_verify_addr(WT_BM *bm,
    WT_SESSION_IMPL *session, const uint8_t *addr, size_t addr_size)
{
	return (__wt_block_verify_addr(session, bm->block, addr, addr_size));
}

/*
 * __bm_verify_end --
 *	End a block manager verify.
 */
static int
__bm_verify_end(WT_BM *bm, WT_SESSION_IMPL *session)
{
	return (__wt_block_verify_end(session, bm->block));
}

/*
 * __bm_method_set --
 *	Set up the legal methods.
 */
static void
__bm_method_set(WT_BM *bm, int readonly)
{
	if (readonly) {
		bm->addr_string = __bm_addr_string;
		bm->addr_valid = __bm_addr_valid;
		bm->block_header = __bm_block_header;
		bm->checkpoint = (int (*)(WT_BM *,
		    WT_SESSION_IMPL *, WT_ITEM *, WT_CKPT *, int))__bm_readonly;
		bm->checkpoint_load = __bm_checkpoint_load;
		bm->checkpoint_resolve =
		    (int (*)(WT_BM *, WT_SESSION_IMPL *))__bm_readonly;
		bm->checkpoint_unload = __bm_checkpoint_unload;
		bm->close = __bm_close;
		bm->compact_end =
		    (int (*)(WT_BM *, WT_SESSION_IMPL *))__bm_readonly;
		bm->compact_page_skip = (int (*)(WT_BM *, WT_SESSION_IMPL *,
		    const uint8_t *, size_t, int *))__bm_readonly;
		bm->compact_skip = (int (*)
		    (WT_BM *, WT_SESSION_IMPL *, int *))__bm_readonly;
		bm->compact_start =
		    (int (*)(WT_BM *, WT_SESSION_IMPL *))__bm_readonly;
		bm->free = (int (*)(WT_BM *,
		    WT_SESSION_IMPL *, const uint8_t *, size_t))__bm_readonly;
		bm->preload = __wt_bm_preload;
		bm->read = __wt_bm_read;
		bm->salvage_end = (int (*)
		    (WT_BM *, WT_SESSION_IMPL *))__bm_readonly;
		bm->salvage_next = (int (*)(WT_BM *, WT_SESSION_IMPL *,
		    uint8_t *, size_t *, int *))__bm_readonly;
		bm->salvage_start = (int (*)
		    (WT_BM *, WT_SESSION_IMPL *))__bm_readonly;
		bm->salvage_valid = (int (*)(WT_BM *,
		    WT_SESSION_IMPL *, uint8_t *, size_t, int))__bm_readonly;
		bm->stat = __bm_stat;
		bm->sync =
		    (int (*)(WT_BM *, WT_SESSION_IMPL *, int))__bm_readonly;
		bm->verify_addr = __bm_verify_addr;
		bm->verify_end = __bm_verify_end;
		bm->verify_start = __bm_verify_start;
		bm->write = (int (*)(WT_BM *, WT_SESSION_IMPL *,
		    WT_ITEM *, uint8_t *, size_t *, int))__bm_readonly;
		bm->write_size = (int (*)
		    (WT_BM *, WT_SESSION_IMPL *, size_t *))__bm_readonly;
	} else {
		bm->addr_string = __bm_addr_string;
		bm->addr_valid = __bm_addr_valid;
		bm->block_header = __bm_block_header;
		bm->checkpoint = __bm_checkpoint;
		bm->checkpoint_load = __bm_checkpoint_load;
		bm->checkpoint_resolve = __bm_checkpoint_resolve;
		bm->checkpoint_unload = __bm_checkpoint_unload;
		bm->close = __bm_close;
		bm->compact_end = __bm_compact_end;
		bm->compact_page_skip = __bm_compact_page_skip;
		bm->compact_skip = __bm_compact_skip;
		bm->compact_start = __bm_compact_start;
		bm->free = __bm_free;
		bm->preload = __wt_bm_preload;
		bm->read = __wt_bm_read;
		bm->salvage_end = __bm_salvage_end;
		bm->salvage_next = __bm_salvage_next;
		bm->salvage_start = __bm_salvage_start;
		bm->salvage_valid = __bm_salvage_valid;
		bm->stat = __bm_stat;
		bm->sync = __bm_sync;
		bm->verify_addr = __bm_verify_addr;
		bm->verify_end = __bm_verify_end;
		bm->verify_start = __bm_verify_start;
		bm->write = __bm_write;
		bm->write_size = __bm_write_size;
	}
}

/*
 * __wt_block_manager_open --
 *	Open a file.
 */
int
__wt_block_manager_open(WT_SESSION_IMPL *session,
    const char *filename, const char *cfg[],
    int forced_salvage, int readonly, uint32_t allocsize, WT_BM **bmp)
{
	WT_BM *bm;
	WT_DECL_RET;

	*bmp = NULL;

	WT_RET(__wt_calloc_one(session, &bm));
	__bm_method_set(bm, 0);

	WT_ERR(__wt_block_open(session, filename, cfg,
	    forced_salvage, readonly, allocsize, &bm->block));

	*bmp = bm;
	return (0);

err:	WT_TRET(bm->close(bm, session));
	return (ret);
}
