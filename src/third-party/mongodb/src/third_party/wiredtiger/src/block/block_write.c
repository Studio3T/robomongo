/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * __wt_block_header --
 *	Return the size of the block-specific header.
 */
u_int
__wt_block_header(WT_BLOCK *block)
{
	WT_UNUSED(block);

	return ((u_int)WT_BLOCK_HEADER_SIZE);
}

/*
 * __wt_block_truncate --
 *	Truncate the file.
 */
int
__wt_block_truncate(WT_SESSION_IMPL *session, WT_FH *fh, wt_off_t len)
{
	WT_RET(__wt_ftruncate(session, fh, len));

	fh->size = fh->extend_size = len;

	return (0);
}

/*
 * __wt_block_extend --
 *	Extend the file.
 */
static inline int
__wt_block_extend(WT_SESSION_IMPL *session, WT_BLOCK *block,
    WT_FH *fh, wt_off_t offset, size_t align_size, int *release_lockp)
{
	WT_DECL_RET;
	int locked;

	/*
	 * The locking in this function is messy: the live system is locked when
	 * we're called, by definition, but that lock may have been acquired by
	 * our our caller or our caller's caller. If it's our caller's lock and
	 * we can unlock it before returning (either before extending the file
	 * or afterward, depending on the call used), then release_lock is set.
	 *
	 * If we unlock, but then find out we need a lock after all, re-acquire
	 * the lock (and set release_lock so our caller knows to release it).
	 */
	locked = 1;

	/* If not configured to extend the file, we're done. */
	if (fh->extend_len == 0)
		return (0);

	/*
	 * Extend the file in chunks.  We want to limit the number of threads
	 * extending the file at the same time, so choose the one thread that's
	 * crossing the extended boundary.  We don't extend newly created files,
	 * and it's theoretically possible we might wait so long our extension
	 * of the file is passed by another thread writing single blocks, that's
	 * why there's a check in case the extended file size becomes too small:
	 * if the file size catches up, every thread tries to extend it.
	 */
	if (fh->extend_size > fh->size &&
	    (offset > fh->extend_size ||
	    offset + fh->extend_len + (wt_off_t)align_size < fh->extend_size))
		return (0);

	/*
	 * File extension may require locking: some variants of the system call
	 * used to extend the file initialize the extended space. If a writing
	 * thread races with the extending thread, the extending thread might
	 * overwrite already written data, and that would be very, very bad.
	 *
	 * Some variants of the system call to extend the file fail at run-time
	 * based on the filesystem type, fall back to ftruncate in that case,
	 * and remember that ftruncate requires locking.
	 */
	if (fh->fallocate_available != WT_FALLOCATE_NOT_AVAILABLE) {
		/*
		 * Release any locally acquired lock if not needed to extend the
		 * file, extending the file may require updating on-disk file's
		 * metadata, which can be slow. (It may be a bad idea to
		 * configure for file extension on systems that require locking
		 * over the extend call.)
		 */
		if (!fh->fallocate_requires_locking && *release_lockp) {
			*release_lockp = locked = 0;
			__wt_spin_unlock(session, &block->live_lock);
		}

		/*
		 * Extend the file: there's a race between setting the value of
		 * extend_size and doing the extension, but it should err on the
		 * side of extend_size being smaller than the actual file size,
		 * and that's OK, we simply may do another extension sooner than
		 * otherwise.
		 */
		fh->extend_size = fh->size + fh->extend_len * 2;
		if ((ret = __wt_fallocate(
		    session, fh, fh->size, fh->extend_len * 2)) == 0)
			return (0);
		if (ret != ENOTSUP)
			return (ret);
	}

	/*
	 * We may have a caller lock or a locally acquired lock, but we need a
	 * lock to call ftruncate.
	 */
	if (!locked) {
		__wt_spin_lock(session, &block->live_lock);
		*release_lockp = 1;
	}

	/*
	 * The underlying truncate call initializes allocated space, reset the
	 * extend length after locking so we don't overwrite already-written
	 * blocks.
	 */
	fh->extend_size = fh->size + fh->extend_len * 2;

	/*
	 * The truncate might fail if there's a mapped file (in other words, if
	 * there's an open checkpoint on the file), that's OK.
	 */
	if ((ret = __wt_ftruncate(session, fh, fh->extend_size)) == EBUSY)
		ret = 0;
	return (ret);
}

/*
 * __wt_block_write_size --
 *	Return the buffer size required to write a block.
 */
int
__wt_block_write_size(WT_SESSION_IMPL *session, WT_BLOCK *block, size_t *sizep)
{
	WT_UNUSED(session);

	/*
	 * We write the page size, in bytes, into the block's header as a 4B
	 * unsigned value, and it's possible for the engine to accept an item
	 * we can't write.  For example, a huge key/value where the allocation
	 * size has been set to something large will overflow 4B when it tries
	 * to align the write.  We could make this work (for example, writing
	 * the page size in units of allocation size or something else), but
	 * it's not worth the effort, writing 4GB objects into a btree makes
	 * no sense.  Limit the writes to (4GB - 1KB), it gives us potential
	 * mode bits, and I'm not interested in debugging corner cases anyway.
	 */
	*sizep = (size_t)
	    WT_ALIGN(*sizep + WT_BLOCK_HEADER_BYTE_SIZE, block->allocsize);
	return (*sizep > UINT32_MAX - 1024 ? EINVAL : 0);
}

/*
 * __wt_block_write --
 *	Write a buffer into a block, returning the block's address cookie.
 */
int
__wt_block_write(WT_SESSION_IMPL *session, WT_BLOCK *block,
    WT_ITEM *buf, uint8_t *addr, size_t *addr_sizep, int data_cksum)
{
	wt_off_t offset;
	uint32_t size, cksum;
	uint8_t *endp;

	WT_RET(__wt_block_write_off(
	    session, block, buf, &offset, &size, &cksum, data_cksum, 0));

	endp = addr;
	WT_RET(__wt_block_addr_to_buffer(block, &endp, offset, size, cksum));
	*addr_sizep = WT_PTRDIFF(endp, addr);

	return (0);
}

/*
 * __wt_block_write_off --
 *	Write a buffer into a block, returning the block's offset, size and
 * checksum.
 */
int
__wt_block_write_off(WT_SESSION_IMPL *session, WT_BLOCK *block,
    WT_ITEM *buf, wt_off_t *offsetp, uint32_t *sizep, uint32_t *cksump,
    int data_cksum, int caller_locked)
{
	WT_BLOCK_HEADER *blk;
	WT_DECL_RET;
	WT_FH *fh;
	size_t align_size;
	wt_off_t offset;
	int local_locked;

	blk = WT_BLOCK_HEADER_REF(buf->mem);
	fh = block->fh;

	/* Buffers should be aligned for writing. */
	if (!F_ISSET(buf, WT_ITEM_ALIGNED)) {
		WT_ASSERT(session, F_ISSET(buf, WT_ITEM_ALIGNED));
		WT_RET_MSG(session, EINVAL,
		    "direct I/O check: write buffer incorrectly allocated");
	}

	/*
	 * Align the size to an allocation unit.
	 *
	 * The buffer must be big enough for us to zero to the next allocsize
	 * boundary, this is one of the reasons the btree layer must find out
	 * from the block-manager layer the maximum size of the eventual write.
	 */
	align_size = WT_ALIGN(buf->size, block->allocsize);
	if (align_size > buf->memsize) {
		WT_ASSERT(session, align_size <= buf->memsize);
		WT_RET_MSG(session, EINVAL,
		    "buffer size check: write buffer incorrectly allocated");
	}
	if (align_size > UINT32_MAX) {
		WT_ASSERT(session, align_size <= UINT32_MAX);
		WT_RET_MSG(session, EINVAL,
		    "buffer size check: write buffer too large to write");
	}

	/* Zero out any unused bytes at the end of the buffer. */
	memset((uint8_t *)buf->mem + buf->size, 0, align_size - buf->size);

	/*
	 * Set the disk size so we don't have to incrementally read blocks
	 * during salvage.
	 */
	blk->disk_size = WT_STORE_SIZE(align_size);

	/*
	 * Update the block's checksum: if our caller specifies, checksum the
	 * complete data, otherwise checksum the leading WT_BLOCK_COMPRESS_SKIP
	 * bytes.  The assumption is applications with good compression support
	 * turn off checksums and assume corrupted blocks won't decompress
	 * correctly.  However, if compression failed to shrink the block, the
	 * block wasn't compressed, in which case our caller will tell us to
	 * checksum the data to detect corruption.   If compression succeeded,
	 * we still need to checksum the first WT_BLOCK_COMPRESS_SKIP bytes
	 * because they're not compressed, both to give salvage a quick test
	 * of whether a block is useful and to give us a test so we don't lose
	 * the first WT_BLOCK_COMPRESS_SKIP bytes without noticing.
	 */
	blk->flags = 0;
	if (data_cksum)
		F_SET(blk, WT_BLOCK_DATA_CKSUM);
	blk->cksum = 0;
	blk->cksum = __wt_cksum(
	    buf->mem, data_cksum ? align_size : WT_BLOCK_COMPRESS_SKIP);

	/* Pre-allocate some number of extension structures. */
	WT_RET(__wt_block_ext_prealloc(session, 5));

	/*
	 * Acquire a lock, if we don't already hold one.
	 * Allocate space for the write, and optionally extend the file (note
	 * the block-extend function may release the lock).
	 * Release any locally acquired lock.
	 */
	local_locked = 0;
	if (!caller_locked) {
		__wt_spin_lock(session, &block->live_lock);
		local_locked = 1;
	}
	ret = __wt_block_alloc(session, block, &offset, (wt_off_t)align_size);
	if (ret == 0)
		ret = __wt_block_extend(
		    session, block, fh, offset, align_size, &local_locked);
	if (local_locked)
		__wt_spin_unlock(session, &block->live_lock);
	WT_RET(ret);

	/* Write the block. */
	if ((ret =
	    __wt_write(session, fh, offset, align_size, buf->mem)) != 0) {
		if (!caller_locked)
			__wt_spin_lock(session, &block->live_lock);
		WT_TRET(__wt_block_off_free(
		    session, block, offset, (wt_off_t)align_size));
		if (!caller_locked)
			__wt_spin_unlock(session, &block->live_lock);
		WT_RET(ret);
	}

#ifdef HAVE_SYNC_FILE_RANGE
	/*
	 * Optionally schedule writes for dirty pages in the system buffer
	 * cache, but only if the current session can wait.
	 */
	if (block->os_cache_dirty_max != 0 &&
	    (block->os_cache_dirty += align_size) > block->os_cache_dirty_max &&
	    __wt_session_can_wait(session)) {
		block->os_cache_dirty = 0;
		WT_RET(__wt_fsync_async(session, fh));
	}
#endif
#ifdef HAVE_POSIX_FADVISE
	/* Optionally discard blocks from the system buffer cache. */
	if (block->os_cache_max != 0 &&
	    (block->os_cache += align_size) > block->os_cache_max) {
		block->os_cache = 0;
		if ((ret = posix_fadvise(fh->fd,
		    (wt_off_t)0, (wt_off_t)0, POSIX_FADV_DONTNEED)) != 0)
			WT_RET_MSG(
			    session, ret, "%s: posix_fadvise", block->name);
	}
#endif
	WT_STAT_FAST_CONN_INCR(session, block_write);
	WT_STAT_FAST_CONN_INCRV(session, block_byte_write, align_size);

	WT_RET(__wt_verbose(session, WT_VERB_WRITE,
	    "off %" PRIuMAX ", size %" PRIuMAX ", cksum %" PRIu32,
	    (uintmax_t)offset, (uintmax_t)align_size, blk->cksum));

	*offsetp = offset;
	*sizep = WT_STORE_SIZE(align_size);
	*cksump = blk->cksum;

	return (ret);
}
