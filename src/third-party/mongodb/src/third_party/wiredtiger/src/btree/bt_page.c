/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

static void __inmem_col_fix(WT_SESSION_IMPL *, WT_PAGE *);
static void __inmem_col_int(WT_SESSION_IMPL *, WT_PAGE *);
static int  __inmem_col_var(WT_SESSION_IMPL *, WT_PAGE *, size_t *);
static int  __inmem_row_int(WT_SESSION_IMPL *, WT_PAGE *, size_t *);
static int  __inmem_row_leaf(WT_SESSION_IMPL *, WT_PAGE *);
static int  __inmem_row_leaf_entries(
	WT_SESSION_IMPL *, const WT_PAGE_HEADER *, uint32_t *);

/*
 * __evict_force_check --
 *	Check if a page matches the criteria for forced eviction.
 */
static int
__evict_force_check(WT_SESSION_IMPL *session, WT_PAGE *page, uint32_t flags)
{
	WT_BTREE *btree;

	btree = S2BT(session);

	/* Pages are usually small enough, check that first. */
	if (page->memory_footprint < btree->maxmempage)
		return (0);

	/* Leaf pages only. */
	if (WT_PAGE_IS_INTERNAL(page))
		return (0);

	/* Eviction may be turned off. */
	if (LF_ISSET(WT_READ_NO_EVICT) || F_ISSET(btree, WT_BTREE_NO_EVICTION))
		return (0);

	/*
	 * It's hard to imagine a page with a huge memory footprint that has
	 * never been modified, but check to be sure.
	 */
	if (page->modify == NULL)
		return (0);

	/* Trigger eviction on the next page release. */
	__wt_page_evict_soon(page);

	/* Bump the oldest ID, we're about to do some visibility checks. */
	__wt_txn_update_oldest(session, 0);

	/* If eviction cannot succeed, don't try. */
	return (__wt_page_can_evict(session, page, 1));
}

/*
 * __wt_page_in_func --
 *	Acquire a hazard pointer to a page; if the page is not in-memory,
 *	read it from the disk and build an in-memory version.
 */
int
__wt_page_in_func(WT_SESSION_IMPL *session, WT_REF *ref, uint32_t flags
#ifdef HAVE_DIAGNOSTIC
    , const char *file, int line
#endif
    )
{
	WT_DECL_RET;
	WT_PAGE *page;
	u_int sleep_cnt, wait_cnt;
	int busy, force_attempts, oldgen;

	for (force_attempts = oldgen = 0, wait_cnt = 0;;) {
		switch (ref->state) {
		case WT_REF_DISK:
		case WT_REF_DELETED:
			if (LF_ISSET(WT_READ_CACHE))
				return (WT_NOTFOUND);

			/*
			 * The page isn't in memory, attempt to read it.
			 * Make sure there is space in the cache.
			 */
			WT_RET(__wt_cache_full_check(session));
			WT_RET(__wt_cache_read(session, ref));
			oldgen = LF_ISSET(WT_READ_WONT_NEED) ||
			    F_ISSET(session, WT_SESSION_NO_CACHE);
			continue;
		case WT_REF_READING:
			if (LF_ISSET(WT_READ_CACHE))
				return (WT_NOTFOUND);
			if (LF_ISSET(WT_READ_NO_WAIT))
				return (WT_NOTFOUND);
			WT_STAT_FAST_CONN_INCR(session, page_read_blocked);
			break;
		case WT_REF_LOCKED:
			if (LF_ISSET(WT_READ_NO_WAIT))
				return (WT_NOTFOUND);
			WT_STAT_FAST_CONN_INCR(session, page_locked_blocked);
			break;
		case WT_REF_SPLIT:
			return (WT_RESTART);
		case WT_REF_MEM:
			/*
			 * The page is in memory: get a hazard pointer, update
			 * the page's LRU and return.  The expected reason we
			 * can't get a hazard pointer is because the page is
			 * being evicted; yield and try again.
			 */
#ifdef HAVE_DIAGNOSTIC
			WT_RET(
			    __wt_hazard_set(session, ref, &busy, file, line));
#else
			WT_RET(__wt_hazard_set(session, ref, &busy));
#endif
			if (busy) {
				WT_STAT_FAST_CONN_INCR(
				    session, page_busy_blocked);
				break;
			}

			page = ref->page;
			WT_ASSERT(session, page != NULL);

			/*
			 * Forcibly evict pages that are too big.
			 */
			if (force_attempts < 10 &&
			    __evict_force_check(session, page, flags)) {
				++force_attempts;
				ret = __wt_page_release_evict(session, ref);
				/* If forced eviction fails, stall. */
				if (ret == EBUSY) {
					ret = 0;
					wait_cnt += 1000;
					WT_STAT_FAST_CONN_INCR(session,
					    page_forcible_evict_blocked);
					break;
				} else
					WT_RET(ret);

				/*
				 * The result of a successful forced eviction
				 * is a page-state transition (potentially to
				 * an in-memory page we can use, or a restart
				 * return for our caller), continue the outer
				 * page-acquisition loop.
				 */
				continue;
			}

			/* Check if we need an autocommit transaction. */
			if ((ret = __wt_txn_autocommit_check(session)) != 0) {
				WT_TRET(__wt_hazard_clear(session, page));
				return (ret);
			}

			/*
			 * If we read the page and we are configured to not
			 * trash the cache, set the oldest read generation so
			 * the page is forcibly evicted as soon as possible.
			 *
			 * Otherwise, update the page's read generation.
			 */
			if (oldgen && page->read_gen == WT_READGEN_NOTSET)
				__wt_page_evict_soon(page);
			else if (!LF_ISSET(WT_READ_NO_GEN) &&
			    page->read_gen != WT_READGEN_OLDEST &&
			    page->read_gen < __wt_cache_read_gen(session))
				page->read_gen =
				    __wt_cache_read_gen_bump(session);

			return (0);
		WT_ILLEGAL_VALUE(session);
		}

		/*
		 * We failed to get the page -- yield before retrying, and if
		 * we've yielded enough times, start sleeping so we don't burn
		 * CPU to no purpose.
		 */
		if (++wait_cnt < 1000)
			__wt_yield();
		else {
			sleep_cnt = WT_MIN(wait_cnt, 10000);
			wait_cnt *= 2;
			WT_STAT_FAST_CONN_INCRV(session, page_sleep, sleep_cnt);
			__wt_sleep(0, sleep_cnt);
		}
	}
}

/*
 * __wt_page_alloc --
 *	Create or read a page into the cache.
 */
int
__wt_page_alloc(WT_SESSION_IMPL *session, uint8_t type,
    uint64_t recno, uint32_t alloc_entries, int alloc_refs, WT_PAGE **pagep)
{
	WT_CACHE *cache;
	WT_DECL_RET;
	WT_PAGE *page;
	WT_PAGE_INDEX *pindex;
	size_t size;
	uint32_t i;
	void *p;

	*pagep = NULL;

	cache = S2C(session)->cache;
	page = NULL;

	size = sizeof(WT_PAGE);
	switch (type) {
	case WT_PAGE_COL_FIX:
	case WT_PAGE_COL_INT:
	case WT_PAGE_ROW_INT:
		break;
	case WT_PAGE_COL_VAR:
		/*
		 * Variable-length column-store leaf page: allocate memory to
		 * describe the page's contents with the initial allocation.
		 */
		size += alloc_entries * sizeof(WT_COL);
		break;
	case WT_PAGE_ROW_LEAF:
		/*
		 * Row-store leaf page: allocate memory to describe the page's
		 * contents with the initial allocation.
		 */
		size += alloc_entries * sizeof(WT_ROW);
		break;
	WT_ILLEGAL_VALUE(session);
	}

	WT_RET(__wt_calloc(session, 1, size, &page));

	page->type = type;
	page->read_gen = WT_READGEN_NOTSET;

	switch (type) {
	case WT_PAGE_COL_FIX:
		page->pg_fix_recno = recno;
		page->pg_fix_entries = alloc_entries;
		break;
	case WT_PAGE_COL_INT:
	case WT_PAGE_ROW_INT:
		page->pg_intl_recno = recno;

		/*
		 * Internal pages have an array of references to objects so they
		 * can split.  Allocate the array of references and optionally,
		 * the objects to which they point.
		 */
		WT_ERR(__wt_calloc(session, 1,
		    sizeof(WT_PAGE_INDEX) + alloc_entries * sizeof(WT_REF *),
		    &p));
		size +=
		    sizeof(WT_PAGE_INDEX) + alloc_entries * sizeof(WT_REF *);
		pindex = p;
		pindex->index = (WT_REF **)((WT_PAGE_INDEX *)p + 1);
		pindex->entries = alloc_entries;
		WT_INTL_INDEX_SET(page, pindex);
		if (alloc_refs)
			for (i = 0; i < pindex->entries; ++i) {
				WT_ERR(__wt_calloc_one(
				    session, &pindex->index[i]));
				size += sizeof(WT_REF);
			}
		if (0) {
err:			if ((pindex = WT_INTL_INDEX_GET_SAFE(page)) != NULL) {
				for (i = 0; i < pindex->entries; ++i)
					__wt_free(session, pindex->index[i]);
				__wt_free(session, pindex);
			}
			__wt_free(session, page);
			return (ret);
		}
		break;
	case WT_PAGE_COL_VAR:
		page->pg_var_recno = recno;
		page->pg_var_d = (WT_COL *)((uint8_t *)page + sizeof(WT_PAGE));
		page->pg_var_entries = alloc_entries;
		break;
	case WT_PAGE_ROW_LEAF:
		page->pg_row_d = (WT_ROW *)((uint8_t *)page + sizeof(WT_PAGE));
		page->pg_row_entries = alloc_entries;
		break;
	WT_ILLEGAL_VALUE(session);
	}

	/* Increment the cache statistics. */
	__wt_cache_page_inmem_incr(session, page, size);
	(void)WT_ATOMIC_ADD8(cache->bytes_read, size);
	(void)WT_ATOMIC_ADD8(cache->pages_inmem, 1);

	*pagep = page;
	return (0);
}

/*
 * __wt_page_inmem --
 *	Build in-memory page information.
 */
int
__wt_page_inmem(WT_SESSION_IMPL *session, WT_REF *ref,
    const void *image, size_t memsize, uint32_t flags, WT_PAGE **pagep)
{
	WT_DECL_RET;
	WT_PAGE *page;
	const WT_PAGE_HEADER *dsk;
	uint32_t alloc_entries;
	size_t size;

	*pagep = NULL;

	dsk = image;
	alloc_entries = 0;

	/*
	 * Figure out how many underlying objects the page references so we can
	 * allocate them along with the page.
	 */
	switch (dsk->type) {
	case WT_PAGE_COL_FIX:
	case WT_PAGE_COL_INT:
	case WT_PAGE_COL_VAR:
		/*
		 * Column-store leaf page entries map one-to-one to the number
		 * of physical entries on the page (each physical entry is a
		 * value item).
		 *
		 * Column-store internal page entries map one-to-one to the
		 * number of physical entries on the page (each entry is a
		 * location cookie).
		 */
		alloc_entries = dsk->u.entries;
		break;
	case WT_PAGE_ROW_INT:
		/*
		 * Row-store internal page entries map one-to-two to the number
		 * of physical entries on the page (each entry is a key and
		 * location cookie pair).
		 */
		alloc_entries = dsk->u.entries / 2;
		break;
	case WT_PAGE_ROW_LEAF:
		/*
		 * If the "no empty values" flag is set, row-store leaf page
		 * entries map one-to-one to the number of physical entries
		 * on the page (each physical entry is a key or value item).
		 * If that flag is not set, there are more keys than values,
		 * we have to walk the page to figure it out.
		 */
		if (F_ISSET(dsk, WT_PAGE_EMPTY_V_ALL))
			alloc_entries = dsk->u.entries;
		else if (F_ISSET(dsk, WT_PAGE_EMPTY_V_NONE))
			alloc_entries = dsk->u.entries / 2;
		else
			WT_RET(__inmem_row_leaf_entries(
			    session, dsk, &alloc_entries));
		break;
	WT_ILLEGAL_VALUE(session);
	}

	/* Allocate and initialize a new WT_PAGE. */
	WT_RET(__wt_page_alloc(
	    session, dsk->type, dsk->recno, alloc_entries, 1, &page));
	page->dsk = dsk;
	F_SET_ATOMIC(page, flags);

	/*
	 * Track the memory allocated to build this page so we can update the
	 * cache statistics in a single call. If the disk image is in allocated
	 * memory, start with that.
	 */
	size = LF_ISSET(WT_PAGE_DISK_ALLOC) ? memsize : 0;

	switch (page->type) {
	case WT_PAGE_COL_FIX:
		__inmem_col_fix(session, page);
		break;
	case WT_PAGE_COL_INT:
		__inmem_col_int(session, page);
		break;
	case WT_PAGE_COL_VAR:
		WT_ERR(__inmem_col_var(session, page, &size));
		break;
	case WT_PAGE_ROW_INT:
		WT_ERR(__inmem_row_int(session, page, &size));
		break;
	case WT_PAGE_ROW_LEAF:
		WT_ERR(__inmem_row_leaf(session, page));
		break;
	WT_ILLEGAL_VALUE_ERR(session);
	}

	/* Update the page's in-memory size and the cache statistics. */
	__wt_cache_page_inmem_incr(session, page, size);

	/* Link the new internal page to the parent. */
	if (ref != NULL) {
		switch (page->type) {
		case WT_PAGE_COL_INT:
		case WT_PAGE_ROW_INT:
			page->pg_intl_parent_ref = ref;
			break;
		}
		ref->page = page;
	}

	*pagep = page;
	return (0);

err:	__wt_page_out(session, &page);
	return (ret);
}

/*
 * __inmem_col_fix --
 *	Build in-memory index for fixed-length column-store leaf pages.
 */
static void
__inmem_col_fix(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	const WT_PAGE_HEADER *dsk;

	btree = S2BT(session);
	dsk = page->dsk;

	page->pg_fix_bitf = WT_PAGE_HEADER_BYTE(btree, dsk);
}

/*
 * __inmem_col_int --
 *	Build in-memory index for column-store internal pages.
 */
static void
__inmem_col_int(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	const WT_PAGE_HEADER *dsk;
	WT_PAGE_INDEX *pindex;
	WT_REF **refp, *ref;
	uint32_t i;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;

	/*
	 * Walk the page, building references: the page contains value items.
	 * The value items are on-page items (WT_CELL_VALUE).
	 */
	pindex = WT_INTL_INDEX_GET_SAFE(page);
	refp = pindex->index;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		ref = *refp++;
		ref->home = page;

		__wt_cell_unpack(cell, unpack);
		ref->addr = cell;
		ref->key.recno = unpack->v;
	}
}

/*
 * __inmem_col_var_repeats --
 *	Count the number of repeat entries on the page.
 */
static int
__inmem_col_var_repeats(WT_SESSION_IMPL *session, WT_PAGE *page, uint32_t *np)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	const WT_PAGE_HEADER *dsk;
	uint32_t i;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;

	/* Walk the page, counting entries for the repeats array. */
	*np = 0;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		if (__wt_cell_rle(unpack) > 1)
			++*np;
	}
	return (0);
}

/*
 * __inmem_col_var --
 *	Build in-memory index for variable-length, data-only leaf pages in
 *	column-store trees.
 */
static int
__inmem_col_var(WT_SESSION_IMPL *session, WT_PAGE *page, size_t *sizep)
{
	WT_BTREE *btree;
	WT_COL *cip;
	WT_COL_RLE *repeats;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	const WT_PAGE_HEADER *dsk;
	uint64_t recno, rle;
	size_t bytes_allocated;
	uint32_t i, indx, n, repeat_off;

	btree = S2BT(session);
	dsk = page->dsk;
	recno = page->pg_var_recno;

	repeats = NULL;
	repeat_off = 0;
	unpack = &_unpack;
	bytes_allocated = 0;

	/*
	 * Walk the page, building references: the page contains unsorted value
	 * items.  The value items are on-page (WT_CELL_VALUE), overflow items
	 * (WT_CELL_VALUE_OVFL) or deleted items (WT_CELL_DEL).
	 */
	indx = 0;
	cip = page->pg_var_d;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		WT_COL_PTR_SET(cip, WT_PAGE_DISK_OFFSET(page, cell));
		cip++;

		/*
		 * Add records with repeat counts greater than 1 to an array we
		 * use for fast lookups.  The first entry we find needing the
		 * repeats array triggers a re-walk from the start of the page
		 * to determine the size of the array.
		 */
		rle = __wt_cell_rle(unpack);
		if (rle > 1) {
			if (repeats == NULL) {
				WT_RET(
				    __inmem_col_var_repeats(session, page, &n));
				WT_RET(__wt_realloc_def(session,
				    &bytes_allocated, n + 1, &repeats));

				page->pg_var_repeats = repeats;
				page->pg_var_nrepeats = n;
				*sizep += bytes_allocated;
			}
			repeats[repeat_off].indx = indx;
			repeats[repeat_off].recno = recno;
			repeats[repeat_off++].rle = rle;
		}
		indx++;
		recno += rle;
	}

	return (0);
}

/*
 * __inmem_row_int --
 *	Build in-memory index for row-store internal pages.
 */
static int
__inmem_row_int(WT_SESSION_IMPL *session, WT_PAGE *page, size_t *sizep)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	WT_DECL_ITEM(current);
	WT_DECL_RET;
	const WT_PAGE_HEADER *dsk;
	WT_PAGE_INDEX *pindex;
	WT_REF *ref, **refp;
	uint32_t i;

	btree = S2BT(session);
	unpack = &_unpack;
	dsk = page->dsk;

	WT_RET(__wt_scr_alloc(session, 0, &current));

	/*
	 * Walk the page, instantiating keys: the page contains sorted key and
	 * location cookie pairs.  Keys are on-page/overflow items and location
	 * cookies are WT_CELL_ADDR_XXX items.
	 */
	pindex = WT_INTL_INDEX_GET_SAFE(page);
	refp = pindex->index;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		ref = *refp;
		ref->home = page;

		__wt_cell_unpack(cell, unpack);
		switch (unpack->type) {
		case WT_CELL_KEY:
			/*
			 * Note: we don't Huffman encode internal page keys,
			 * there's no decoding work to do.
			 */
			__wt_ref_key_onpage_set(page, ref, unpack);
			break;
		case WT_CELL_KEY_OVFL:
			/* Instantiate any overflow records. */
			WT_ERR(__wt_dsk_cell_data_ref(
			    session, page->type, unpack, current));

			WT_ERR(__wt_row_ikey_incr(session, page,
			    WT_PAGE_DISK_OFFSET(page, cell),
			    current->data, current->size, ref));

			*sizep += sizeof(WT_IKEY) + current->size;
			break;
		case WT_CELL_ADDR_DEL:
			/*
			 * A cell may reference a deleted leaf page: if a leaf
			 * page was deleted without being read (fast truncate),
			 * and the deletion committed, but older transactions
			 * in the system required the previous version of the
			 * page to remain available, a special deleted-address
			 * type cell is written.  The only reason we'd ever see
			 * that cell on a page we're reading is if we crashed
			 * and recovered (otherwise a version of the page w/o
			 * that cell would have eventually been written).  If we
			 * crash and recover to a page with a deleted-address
			 * cell, we want to discard the page from the backing
			 * store (it was never discarded), and, of course, by
			 * definition no earlier transaction will ever need it.
			 *
			 * Re-create the state of a deleted page.
			 */
			ref->addr = cell;
			ref->state = WT_REF_DELETED;
			++refp;

			/*
			 * If the tree is already dirty and so will be written,
			 * mark the page dirty.  (We want to free the deleted
			 * pages, but if the handle is read-only or if the
			 * application never modifies the tree, we're not able
			 * to do so.)
			 */
			if (btree->modified) {
				WT_ERR(__wt_page_modify_init(session, page));
				__wt_page_modify_set(session, page);
			}
			break;
		case WT_CELL_ADDR_INT:
		case WT_CELL_ADDR_LEAF:
		case WT_CELL_ADDR_LEAF_NO:
			ref->addr = cell;
			++refp;
			break;
		WT_ILLEGAL_VALUE_ERR(session);
		}
	}

err:	__wt_scr_free(session, &current);
	return (ret);
}

/*
 * __inmem_row_leaf_entries --
 *	Return the number of entries for row-store leaf pages.
 */
static int
__inmem_row_leaf_entries(
    WT_SESSION_IMPL *session, const WT_PAGE_HEADER *dsk, uint32_t *nindxp)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	uint32_t i, nindx;

	btree = S2BT(session);
	unpack = &_unpack;

	/*
	 * Leaf row-store page entries map to a maximum of one-to-one to the
	 * number of physical entries on the page (each physical entry might be
	 * a key without a subsequent data item).  To avoid over-allocation in
	 * workloads without empty data items, first walk the page counting the
	 * number of keys, then allocate the indices.
	 *
	 * The page contains key/data pairs.  Keys are on-page (WT_CELL_KEY) or
	 * overflow (WT_CELL_KEY_OVFL) items, data are either non-existent or a
	 * single on-page (WT_CELL_VALUE) or overflow (WT_CELL_VALUE_OVFL) item.
	 */
	nindx = 0;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		switch (unpack->type) {
		case WT_CELL_KEY:
		case WT_CELL_KEY_OVFL:
			++nindx;
			break;
		case WT_CELL_VALUE:
		case WT_CELL_VALUE_OVFL:
			break;
		WT_ILLEGAL_VALUE(session);
		}
	}

	*nindxp = nindx;
	return (0);
}

/*
 * __inmem_row_leaf --
 *	Build in-memory index for row-store leaf pages.
 */
static int
__inmem_row_leaf(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	WT_CELL *cell;
	WT_CELL_UNPACK *unpack, _unpack;
	const WT_PAGE_HEADER *dsk;
	WT_ROW *rip;
	uint32_t i;

	btree = S2BT(session);
	dsk = page->dsk;
	unpack = &_unpack;

	/* Walk the page, building indices. */
	rip = page->pg_row_d;
	WT_CELL_FOREACH(btree, dsk, cell, unpack, i) {
		__wt_cell_unpack(cell, unpack);
		switch (unpack->type) {
		case WT_CELL_KEY_OVFL:
			__wt_row_leaf_key_set_cell(page, rip, cell);
			++rip;
			break;
		case WT_CELL_KEY:
			/*
			 * Simple keys without compression (not Huffman encoded
			 * or prefix compressed), can be directly referenced on
			 * the page to avoid repeatedly unpacking their cells.
			 */
			if (!btree->huffman_key && unpack->prefix == 0)
				__wt_row_leaf_key_set(page, rip, unpack);
			else
				__wt_row_leaf_key_set_cell(page, rip, cell);
			++rip;
			break;
		case WT_CELL_VALUE:
			/*
			 * Simple values without compression can be directly
			 * referenced on the page to avoid repeatedly unpacking
			 * their cells.
			 */
			if (!btree->huffman_value)
				__wt_row_leaf_value_set(page, rip - 1, unpack);
			break;
		case WT_CELL_VALUE_OVFL:
			break;
		WT_ILLEGAL_VALUE(session);
		}
	}

	/*
	 * We do not currently instantiate keys on leaf pages when the page is
	 * loaded, they're instantiated on demand.
	 */
	return (0);
}
