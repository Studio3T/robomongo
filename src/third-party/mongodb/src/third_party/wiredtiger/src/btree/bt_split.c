/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

#define	WT_MEM_TRANSFER(from_decr, to_incr, len) do {			\
	size_t __len = (len);						\
	from_decr += __len;						\
	to_incr += __len;						\
} while (0)

/*
 * __split_oldest_gen --
 *	Calculate the oldest active split generation.
 */
static uint64_t
__split_oldest_gen(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	WT_SESSION_IMPL *s;
	uint64_t gen, oldest;
	u_int i, session_cnt;

	conn = S2C(session);
	WT_ORDERED_READ(session_cnt, conn->session_cnt);
	for (i = 0, s = conn->sessions, oldest = conn->split_gen + 1;
	    i < session_cnt;
	    i++, s++)
		if (((gen = s->split_gen) != 0) && gen < oldest)
			oldest = gen;

	return (oldest);
}

/*
 * __split_stash_add --
 *	Add a new entry into the session's split stash list.
 */
static int
__split_stash_add(
    WT_SESSION_IMPL *session, uint64_t split_gen, void *p, size_t len)
{
	WT_SPLIT_STASH *stash;

	WT_ASSERT(session, p != NULL);

	/* Grow the list as necessary. */
	WT_RET(__wt_realloc_def(session, &session->split_stash_alloc,
	    session->split_stash_cnt + 1, &session->split_stash));

	stash = session->split_stash + session->split_stash_cnt++;
	stash->split_gen = split_gen;
	stash->p = p;
	stash->len = len;

	WT_STAT_FAST_CONN_ATOMIC_INCRV(session, rec_split_stashed_bytes, len);
	WT_STAT_FAST_CONN_ATOMIC_INCR(session, rec_split_stashed_objects);

	/* See if we can free any previous entries. */
	if (session->split_stash_cnt > 1)
		__wt_split_stash_discard(session);

	return (0);
}

/*
 * __wt_split_stash_discard --
 *	Discard any memory from a session's split stash that we can.
 */
void
__wt_split_stash_discard(WT_SESSION_IMPL *session)
{
	WT_SPLIT_STASH *stash;
	uint64_t oldest;
	size_t i;

	/* Get the oldest split generation. */
	oldest = __split_oldest_gen(session);

	for (i = 0, stash = session->split_stash;
	    i < session->split_stash_cnt;
	    ++i, ++stash) {
		if (stash->p == NULL)
			continue;
		else if (stash->split_gen >= oldest)
			break;
		/*
		 * It's a bad thing if another thread is in this memory after
		 * we free it, make sure nothing good happens to that thread.
		 */
		WT_STAT_FAST_CONN_ATOMIC_DECRV(
		    session, rec_split_stashed_bytes, stash->len);
		WT_STAT_FAST_CONN_ATOMIC_DECR(
		    session, rec_split_stashed_objects);
		__wt_overwrite_and_free_len(session, stash->p, stash->len);
	}

	/*
	 * If there are enough free slots at the beginning of the list, shuffle
	 * everything down.
	 */
	if (i > 100 || i == session->split_stash_cnt)
		if ((session->split_stash_cnt -= i) > 0)
			memmove(session->split_stash, stash,
			    session->split_stash_cnt * sizeof(*stash));
}

/*
 * __wt_split_stash_discard_all --
 *	Discard all memory from a session's split stash.
 */
void
__wt_split_stash_discard_all(
    WT_SESSION_IMPL *session_safe, WT_SESSION_IMPL *session)
{
	WT_SPLIT_STASH *stash;
	size_t i;

	/*
	 * This function is called during WT_CONNECTION.close to discard any
	 * memory that remains.  For that reason, we take two WT_SESSION_IMPL
	 * arguments: session_safe is still linked to the WT_CONNECTION and
	 * can be safely used for calls to other WiredTiger functions, while
	 * session is the WT_SESSION_IMPL we're cleaning up.
	 */
	for (i = 0, stash = session->split_stash;
	    i < session->split_stash_cnt;
	    ++i, ++stash)
		if (stash->p != NULL)
			__wt_free(session_safe, stash->p);

	__wt_free(session_safe, session->split_stash);
	session->split_stash_cnt = session->split_stash_alloc = 0;
}

/*
 * __split_safe_free --
 *	Free a buffer if we can be sure no thread is accessing it, or schedule
 *	it to be freed otherwise.
 */
static int
__split_safe_free(WT_SESSION_IMPL *session,
    uint64_t split_gen, int exclusive, void *p, size_t s)
{
	/*
	 * We have swapped something in a page: if we don't have exclusive
	 * access, check whether there are other threads in the same tree.
	 */
	if (!exclusive && __split_oldest_gen(session) > split_gen)
		exclusive = 1;

	if (exclusive) {
		__wt_free(session, p);
		return (0);
	}

	return (__split_stash_add(session, split_gen, p, s));
}

/*
 * __split_should_deepen --
 *	Return if we should deepen the tree.
 */
static int
__split_should_deepen(
    WT_SESSION_IMPL *session, WT_REF *ref, uint32_t *childrenp)
{
	WT_BTREE *btree;
	WT_PAGE *page;
	WT_PAGE_INDEX *pindex;

	*childrenp = 0;

	btree = S2BT(session);
	page = ref->page;
	pindex = WT_INTL_INDEX_GET_SAFE(page);

	/*
	 * Deepen the tree if the page's memory footprint is larger than the
	 * maximum size for a page in memory (presumably putting eviction
	 * pressure on the cache).
	 */
	if (page->memory_footprint < btree->maxmempage)
		return (0);

	/*
	 * Ensure the page has enough entries to make it worth splitting and
	 * we get a significant payback (in the case of a set of large keys,
	 * splitting won't help).
	 */
	if (pindex->entries > btree->split_deepen_min_child) {
		*childrenp = pindex->entries / btree->split_deepen_per_child;
		return (1);
	}

	/*
	 * Don't allow a single page to put pressure on cache usage. The root
	 * page cannot be evicted, so if it's larger than the maximum, or if
	 * and page has a quarter of the cache, let it split, a deep tree is
	 * better than making no progress at all. Sanity check for 100 on-page
	 * keys, nothing helps in the case of large keys and a too-small cache.
	 */
	if (pindex->entries >= 100 &&
	    (__wt_ref_is_root(ref) ||
	    page->memory_footprint >= S2C(session)->cache_size / 4)) {
		*childrenp = pindex->entries / 10;
		return (1);
	}

	return (0);
}

/*
 * __split_ovfl_key_cleanup --
 *	Handle cleanup for on-page row-store overflow keys.
 */
static int
__split_ovfl_key_cleanup(WT_SESSION_IMPL *session, WT_PAGE *page, WT_REF *ref)
{
	WT_CELL *cell;
	WT_CELL_UNPACK kpack;
	WT_IKEY *ikey;
	uint32_t cell_offset;

	/*
	 * A key being discarded (page split) or moved to a different page (page
	 * deepening) may be an on-page overflow key.  Clear any reference to an
	 * underlying disk image, and, if the key hasn't been deleted, delete it
	 * along with any backing blocks.
	 */
	if ((ikey = __wt_ref_key_instantiated(ref)) == NULL)
		return (0);
	if ((cell_offset = ikey->cell_offset) == 0)
		return (0);

	/* Leak blocks rather than try this twice. */
	ikey->cell_offset = 0;

	cell = WT_PAGE_REF_OFFSET(page, cell_offset);
	__wt_cell_unpack(cell, &kpack);
	if (kpack.ovfl && kpack.raw != WT_CELL_KEY_OVFL_RM)
		WT_RET(__wt_ovfl_discard(session, cell));

	return (0);
}

/*
 * __split_ref_deepen_move --
 *	Move a WT_REF from a parent to a child in service of a split to deepen
 * the tree, including updating the accounting information.
 */
static int
__split_ref_deepen_move(WT_SESSION_IMPL *session,
    WT_PAGE *parent, WT_REF *ref, size_t *parent_decrp, size_t *child_incrp)
{
	WT_ADDR *addr;
	WT_CELL_UNPACK unpack;
	WT_DECL_RET;
	WT_IKEY *ikey;
	size_t size;
	void *key;

	/*
	 * Instantiate row-store keys, and column- and row-store addresses in
	 * the WT_REF structures referenced by a page that's being split (and
	 * deepening the tree).  The WT_REF structures aren't moving, but the
	 * index references are moving from the page we're splitting to a set
	 * of child pages, and so we can no longer reference the block image
	 * that remains with the page being split.
	 *
	 * No locking is required to update the WT_REF structure because we're
	 * the only thread splitting the parent page, and there's no way for
	 * readers to race with our updates of single pointers.  The changes
	 * have to be written before the page goes away, of course, our caller
	 * owns that problem.
	 *
	 * Row-store keys, first.
	 */
	if (parent->type == WT_PAGE_ROW_INT) {
		if ((ikey = __wt_ref_key_instantiated(ref)) == NULL) {
			__wt_ref_key(parent, ref, &key, &size);
			WT_RET(__wt_row_ikey(session, 0, key, size, ref));
			ikey = ref->key.ikey;
		} else {
			WT_RET(__split_ovfl_key_cleanup(session, parent, ref));
			*parent_decrp += sizeof(WT_IKEY) + ikey->size;
		}
		*child_incrp += sizeof(WT_IKEY) + ikey->size;
	}

	/*
	 * If there's no address (the page has never been written), or the
	 * address has been instantiated, there's no work to do.  Otherwise,
	 * get the address from the on-page cell.
	 */
	addr = ref->addr;
	if (addr != NULL && !__wt_off_page(parent, addr)) {
		__wt_cell_unpack((WT_CELL *)ref->addr, &unpack);
		WT_RET(__wt_calloc_one(session, &addr));
		if ((ret = __wt_strndup(
		    session, unpack.data, unpack.size, &addr->addr)) != 0) {
			__wt_free(session, addr);
			return (ret);
		}
		addr->size = (uint8_t)unpack.size;
		addr->type =
		    unpack.raw == WT_CELL_ADDR_INT ? WT_ADDR_INT : WT_ADDR_LEAF;
		ref->addr = addr;
	}

	/* And finally, the WT_REF itself. */
	WT_MEM_TRANSFER(*parent_decrp, *child_incrp, sizeof(WT_REF));

	return (0);
}

#ifdef HAVE_DIAGNOSTIC
/*
 * __split_verify_intl_key_order --
 *	Verify the key order on an internal page after a split, diagnostic only.
 */
static void
__split_verify_intl_key_order(WT_SESSION_IMPL *session, WT_PAGE *page)
{
	WT_BTREE *btree;
	WT_ITEM *next, _next, *last, _last, *tmp;
	WT_REF *ref;
	uint64_t recno;
	int cmp, first;

	btree = S2BT(session);

	switch (page->type) {
	case WT_PAGE_COL_INT:
		recno = 0;
		WT_INTL_FOREACH_BEGIN_SAFE(session, page, ref) {
			WT_ASSERT(session, ref->key.recno > recno);
			recno = ref->key.recno;
		} WT_INTL_FOREACH_END;
		break;
	case WT_PAGE_ROW_INT:
		next = &_next;
		WT_CLEAR(_next);
		last = &_last;
		WT_CLEAR(_last);

		first = 1;
		WT_INTL_FOREACH_BEGIN_SAFE(session, page, ref) {
			__wt_ref_key(page, ref, &next->data, &next->size);
			if (last->size == 0) {
				if (first)
					first = 0;
				else {
					WT_ASSERT(session, __wt_compare(
					    session, btree->collator, last,
					    next, &cmp) == 0);
					WT_ASSERT(session, cmp < 0);
				}
			}
			tmp = last;
			last = next;
			next = tmp;
		} WT_INTL_FOREACH_END;
		break;
	}
}
#endif

/*
 * __split_deepen --
 *	Split an internal page in-memory, deepening the tree.
 */
static int
__split_deepen(WT_SESSION_IMPL *session, WT_PAGE *parent, uint32_t children)
{
	WT_DECL_RET;
	WT_PAGE *child;
	WT_PAGE_INDEX *alloc_index, *child_pindex, *pindex;
	WT_REF **alloc_refp;
	WT_REF *child_ref, **child_refp, *parent_ref, **parent_refp, *ref;
	size_t child_incr, parent_decr, parent_incr, size;
	uint64_t split_gen;
	uint32_t chunk, i, j, remain, slots;
	int panic;
	void *p;

	alloc_index = NULL;
	parent_incr = parent_decr = 0;
	panic = 0;

	pindex = WT_INTL_INDEX_GET_SAFE(parent);

	WT_STAT_FAST_CONN_INCR(session, cache_eviction_deepen);
	WT_STAT_FAST_DATA_INCR(session, cache_eviction_deepen);
	WT_ERR(__wt_verbose(session, WT_VERB_SPLIT,
	    "%p: %" PRIu32 " elements, splitting into %" PRIu32 " children",
	    parent, pindex->entries, children));

	/*
	 * If the workload is prepending/appending to the tree, we could deepen
	 * without bound.  Don't let that happen, keep the first/last pages of
	 * the tree at their current level.
	 *
	 * XXX
	 * To improve this, we could track which pages were last merged into
	 * this page by eviction, and leave those pages alone, to prevent any
	 * sustained insert into the tree from deepening a single location.
	 */
#undef	SPLIT_CORRECT_1
#define	SPLIT_CORRECT_1	1		/* First page correction */
#undef	SPLIT_CORRECT_2
#define	SPLIT_CORRECT_2	2		/* First/last page correction */

	/*
	 * Allocate a new WT_PAGE_INDEX and set of WT_REF objects.  Initialize
	 * the first/last slots of the allocated WT_PAGE_INDEX to point to the
	 * first/last pages we're keeping at the current level, and the rest of
	 * the slots to point to new WT_REF objects.
	 */
	size = sizeof(WT_PAGE_INDEX) +
	    (children + SPLIT_CORRECT_2) * sizeof(WT_REF *);
	WT_ERR(__wt_calloc(session, 1, size, &alloc_index));
	parent_incr += size;
	alloc_index->index = (WT_REF **)(alloc_index + 1);
	alloc_index->entries = children + SPLIT_CORRECT_2;
	alloc_index->index[0] = pindex->index[0];
	alloc_index->index[alloc_index->entries - 1] =
	    pindex->index[pindex->entries - 1];
	for (alloc_refp = alloc_index->index + SPLIT_CORRECT_1,
	    i = 0; i < children; ++alloc_refp, ++i) {
		WT_ERR(__wt_calloc_one(session, alloc_refp));
		parent_incr += sizeof(WT_REF);
	}

	/* Allocate child pages, and connect them into the new page index. */
	chunk = (pindex->entries - SPLIT_CORRECT_2) / children;
	remain = (pindex->entries - SPLIT_CORRECT_2) - chunk * (children - 1);
	for (parent_refp = pindex->index + SPLIT_CORRECT_1,
	    alloc_refp = alloc_index->index + SPLIT_CORRECT_1,
	    i = 0; i < children; ++i) {
		slots = i == children - 1 ? remain : chunk;
		WT_ERR(__wt_page_alloc(
		    session, parent->type, 0, slots, 0, &child));

		/*
		 * Initialize the parent page's child reference; we need a copy
		 * of the page's key.
		 */
		ref = *alloc_refp++;
		ref->home = parent;
		ref->page = child;
		ref->addr = NULL;
		if (parent->type == WT_PAGE_ROW_INT) {
			__wt_ref_key(parent, *parent_refp, &p, &size);
			WT_ERR(__wt_row_ikey(session, 0, p, size, ref));
			parent_incr += sizeof(WT_IKEY) + size;
		} else
			ref->key.recno = (*parent_refp)->key.recno;
		ref->state = WT_REF_MEM;

		/* Initialize the child page. */
		if (parent->type == WT_PAGE_COL_INT)
			child->pg_intl_recno = (*parent_refp)->key.recno;
		child->pg_intl_parent_ref = ref;

		/* Mark it dirty. */
		WT_ERR(__wt_page_modify_init(session, child));
		__wt_page_modify_set(session, child);

		/*
		 * Once the split goes live, the newly created internal pages
		 * might be evicted and their WT_REF structures freed.  If those
		 * pages are evicted before threads exit the previous page index
		 * array, a thread might see a freed WT_REF.  Set the eviction
		 * transaction requirement for the newly created internal pages.
		 */
		child->modify->mod_split_txn = __wt_txn_new_id(session);

		/*
		 * The newly allocated child's page index references the same
		 * structures as the parent.  (We cannot move WT_REF structures,
		 * threads may be underneath us right now changing the structure
		 * state.)  However, if the WT_REF structures reference on-page
		 * information, we have to fix that, because the disk image for
		 * the page that has an page index entry for the WT_REF is about
		 * to change.
		 */
		child_incr = 0;
		child_pindex = WT_INTL_INDEX_GET_SAFE(child);
		for (child_refp = child_pindex->index, j = 0; j < slots; ++j) {
			WT_ERR(__split_ref_deepen_move(session,
			    parent, *parent_refp, &parent_decr, &child_incr));
			*child_refp++ = *parent_refp++;
		}
		__wt_cache_page_inmem_incr(session, child, child_incr);
	}
	WT_ASSERT(session, alloc_refp -
	    alloc_index->index == alloc_index->entries - SPLIT_CORRECT_1);
	WT_ASSERT(session,
	    parent_refp - pindex->index == pindex->entries - SPLIT_CORRECT_1);

	/*
	 * Update the parent's index; this is the update which splits the page,
	 * making the change visible to threads descending the tree.  From now
	 * on, we're committed to the split.  If any subsequent work fails, we
	 * have to panic because we potentially have threads of control using
	 * the new page index we just swapped in.
	 *
	 * A note on error handling: until this point, there's no problem with
	 * unwinding on error.  We allocated a new page index, a new set of
	 * WT_REFs and a new set of child pages -- if an error occurred, the
	 * parent remained unchanged, although it may have an incorrect memory
	 * footprint.  From now on we've modified the parent page, attention
	 * needs to be paid.
	 */
	WT_ASSERT(session, WT_INTL_INDEX_GET_SAFE(parent) == pindex);
	WT_INTL_INDEX_SET(parent, alloc_index);
	split_gen = WT_ATOMIC_ADD8(S2C(session)->split_gen, 1);
	panic = 1;

#ifdef HAVE_DIAGNOSTIC
	__split_verify_intl_key_order(session, parent);
#endif

	/*
	 * The moved reference structures now reference the wrong parent page,
	 * and we have to fix that up.  The problem is revealed when a thread
	 * of control searches for a page's reference structure slot, and fails
	 * to find it because the page it's searching no longer references it.
	 * When that failure happens, the thread waits for the reference's home
	 * page to be updated, which we do here: walk the children and fix them
	 * up.
	 *
	 * We're not acquiring hazard pointers on these pages, they cannot be
	 * evicted because of the eviction transaction value set above.
	 */
	for (parent_refp = alloc_index->index,
	    i = alloc_index->entries; i > 0; ++parent_refp, --i) {
		parent_ref = *parent_refp;
		WT_ASSERT(session, parent_ref->home == parent);
		if (parent_ref->state != WT_REF_MEM)
			continue;

		/*
		 * We left the first/last children of the parent at the current
		 * level to avoid bad split patterns, they might be leaf pages;
		 * check the page type before we continue.
		 */
		child = parent_ref->page;
		if (!WT_PAGE_IS_INTERNAL(child))
			continue;
#ifdef HAVE_DIAGNOSTIC
		__split_verify_intl_key_order(session, child);
#endif
		WT_INTL_FOREACH_BEGIN_SAFE(session, child, child_ref) {
			/*
			 * The page's parent reference may not be wrong, as we
			 * opened up access from the top of the tree already,
			 * pages may have been read in since then.  Check and
			 * only update pages that reference the original page,
			 * they must be wrong.
			 */
			if (child_ref->home == parent) {
				child_ref->home = child;
				child_ref->pindex_hint = 0;
			}
		} WT_INTL_FOREACH_END;
	}

	/*
	 * Push out the changes: not required for correctness, but don't let
	 * threads spin on incorrect page references longer than necessary.
	 */
	WT_FULL_BARRIER();
	alloc_index = NULL;

	/*
	 * We can't free the previous parent's index, there may be threads using
	 * it.  Add to the session's discard list, to be freed once we know no
	 * threads can still be using it.
	 *
	 * This change requires care with error handling: we have already
	 * updated the page with a new index.  Even if stashing the old value
	 * fails, we don't roll back that change, because threads may already
	 * be using the new index.
	 */
	size = sizeof(WT_PAGE_INDEX) + pindex->entries * sizeof(WT_REF *);
	WT_ERR(__split_safe_free(session, split_gen, 0, pindex, size));
	parent_decr += size;

	/*
	 * Adjust the parent's memory footprint.
	 */
	__wt_cache_page_inmem_incr(session, parent, parent_incr);
	__wt_cache_page_inmem_decr(session, parent, parent_decr);

	if (0) {
err:		__wt_free_ref_index(session, parent, alloc_index, 1);

		/*
		 * If panic is set, we saw an error after opening up the tree
		 * to descent through the parent page's new index.  There is
		 * nothing we can do, the tree is inconsistent and there are
		 * threads potentially active in both versions of the tree.
		 */
		if (panic)
			ret = __wt_panic(session);
	}
	return (ret);
}

/*
 * __split_multi_inmem --
 *	Instantiate a page in a multi-block set, when an update couldn't be
 * written.
 */
static int
__split_multi_inmem(
    WT_SESSION_IMPL *session, WT_PAGE *orig, WT_REF *ref, WT_MULTI *multi)
{
	WT_CURSOR_BTREE cbt;
	WT_DECL_ITEM(key);
	WT_DECL_RET;
	WT_PAGE *page;
	WT_UPDATE *upd;
	WT_UPD_SKIPPED *skip;
	uint64_t recno;
	uint32_t i, slot;

	WT_CLEAR(cbt);
	cbt.iface.session = &session->iface;
	cbt.btree = S2BT(session);

	/*
	 * We can find unresolved updates when attempting to evict a page, which
	 * can't be written. This code re-creates the in-memory page and applies
	 * the unresolved updates to that page.
	 *
	 * Clear the disk image and link the page into the passed-in WT_REF to
	 * simplify error handling: our caller will not discard the disk image
	 * when discarding the original page, and our caller will discard the
	 * allocated page on error, when discarding the allocated WT_REF.
	 */
	WT_RET(__wt_page_inmem(session, ref,
	    multi->skip_dsk, ((WT_PAGE_HEADER *)multi->skip_dsk)->mem_size,
	    WT_PAGE_DISK_ALLOC, &page));
	multi->skip_dsk = NULL;

	if (orig->type == WT_PAGE_ROW_LEAF)
		WT_RET(__wt_scr_alloc(session, 0, &key));

	/* Re-create each modification we couldn't write. */
	for (i = 0, skip = multi->skip; i < multi->skip_entries; ++i, ++skip)
		switch (orig->type) {
		case WT_PAGE_COL_FIX:
		case WT_PAGE_COL_VAR:
			/* Build a key. */
			upd = skip->ins->upd;
			skip->ins->upd = NULL;
			recno = WT_INSERT_RECNO(skip->ins);

			/* Search the page. */
			WT_ERR(__wt_col_search(session, recno, ref, &cbt));

			/* Apply the modification. */
			WT_ERR(__wt_col_modify(
			    session, &cbt, recno, NULL, upd, 0));
			break;
		case WT_PAGE_ROW_LEAF:
			/* Build a key. */
			if (skip->ins == NULL) {
				slot = WT_ROW_SLOT(orig, skip->rip);
				upd = orig->pg_row_upd[slot];
				orig->pg_row_upd[slot] = NULL;

				WT_ERR(__wt_row_leaf_key(
				    session, orig, skip->rip, key, 0));
			} else {
				upd = skip->ins->upd;
				skip->ins->upd = NULL;

				key->data = WT_INSERT_KEY(skip->ins);
				key->size = WT_INSERT_KEY_SIZE(skip->ins);
			}

			/* Search the page. */
			WT_ERR(__wt_row_search(session, key, ref, &cbt, 1));

			/* Apply the modification. */
			WT_ERR(
			    __wt_row_modify(session, &cbt, key, NULL, upd, 0));
			break;
		WT_ILLEGAL_VALUE_ERR(session);
		}

	/*
	 * We modified the page above, which will have set the first dirty
	 * transaction to the last transaction current running.  However, the
	 * updates we installed may be older than that.  Set the first dirty
	 * transaction to an impossibly old value so this page is never skipped
	 * in a checkpoint.
	 */
	page->modify->first_dirty_txn = WT_TXN_FIRST;

err:	/* Free any resources that may have been cached in the cursor. */
	WT_TRET(__wt_btcur_close(&cbt));

	__wt_scr_free(session, &key);
	return (ret);
}

/*
 * __wt_multi_to_ref --
 *	Move a multi-block list into an array of WT_REF structures.
 */
int
__wt_multi_to_ref(WT_SESSION_IMPL *session,
    WT_PAGE *page, WT_MULTI *multi, WT_REF **refp, size_t *incrp)
{
	WT_ADDR *addr;
	WT_IKEY *ikey;
	WT_REF *ref;
	size_t incr;

	addr = NULL;
	incr = 0;

	/* In some cases, the underlying WT_REF has not yet been allocated. */
	if (*refp == NULL)
		WT_RET(__wt_calloc_one(session, refp));
	ref = *refp;
	incr += sizeof(WT_REF);

	/*
	 * Any parent reference must be filled in by our caller; the primary
	 * use of this function is when splitting into a parent page, and we
	 * aren't holding any locks here that would allow us to know which
	 * parent we'll eventually split into, if the tree is simultaneously
	 * being deepened.
	 */
	ref->home = NULL;

	if (multi->skip == NULL) {
		/*
		 * Copy the address: we could simply take the buffer, but that
		 * would complicate error handling, freeing the reference array
		 * would have to avoid freeing the memory, and it's not worth
		 * the confusion.
		 */
		WT_RET(__wt_calloc_one(session, &addr));
		ref->addr = addr;
		addr->size = multi->addr.size;
		addr->type = multi->addr.type;
		WT_RET(__wt_strndup(session,
		    multi->addr.addr, addr->size, &addr->addr));
	} else
		WT_RET(__split_multi_inmem(session, page, ref, multi));

	switch (page->type) {
	case WT_PAGE_ROW_INT:
	case WT_PAGE_ROW_LEAF:
		ikey = multi->key.ikey;
		WT_RET(__wt_row_ikey(
		    session, 0, WT_IKEY_DATA(ikey), ikey->size, ref));
		incr += sizeof(WT_IKEY) + ikey->size;
		break;
	default:
		ref->key.recno = multi->key.recno;
		break;
	}

	ref->state = multi->skip == NULL ? WT_REF_DISK : WT_REF_MEM;

	/*
	 * If our caller wants to track the memory allocations, we have a return
	 * reference.
	 */
	if (incrp != NULL)
		*incrp += incr;
	return (0);
}

/*
 * __split_parent --
 *	Resolve a multi-page split, inserting new information into the parent.
 */
static int
__split_parent(WT_SESSION_IMPL *session, WT_REF *ref,
    WT_REF **ref_new, uint32_t new_entries, size_t parent_incr, int exclusive)
{
	WT_DECL_RET;
	WT_IKEY *ikey;
	WT_PAGE *parent;
	WT_PAGE_INDEX *alloc_index, *pindex;
	WT_REF **alloc_refp, *next_ref, *parent_ref;
	size_t parent_decr, size;
	uint64_t split_gen;
	uint32_t children, i, j;
	uint32_t deleted_entries, parent_entries, result_entries;
	int complete, hazard;

	parent = NULL;			/* -Wconditional-uninitialized */
	alloc_index = pindex = NULL;
	parent_ref = NULL;
	parent_decr = 0;
	parent_entries = 0;
	complete = hazard = 0;

	/*
	 * Get a page-level lock on the parent to single-thread splits into the
	 * page because we need to single-thread sizing/growing the page index.
	 * It's OK to queue up multiple splits as the child pages split, but the
	 * actual split into the parent has to be serialized.  Note we allocate
	 * memory inside of the lock and may want to invest effort in making the
	 * locked period shorter.
	 *
	 * We could race with another thread deepening our parent.  To deal
	 * with that, read the parent pointer each time we try to lock it, and
	 * check that it's still correct after it is locked.
	 */
	for (;;) {
		parent = ref->home;
		F_CAS_ATOMIC(parent, WT_PAGE_SPLIT_LOCKED, ret);
		if (ret == 0) {
			if (parent == ref->home)
				break;
			F_CLR_ATOMIC(parent, WT_PAGE_SPLIT_LOCKED);
			continue;
		}
		__wt_yield();
	}

	/*
	 * We have exclusive access to split the parent, and at this point, the
	 * child prevents the parent from being evicted.  However, once we
	 * update the parent's index, it will no longer refer to the child, and
	 * could conceivably be evicted.  Get a hazard pointer on the parent
	 * now, so that we can safely access it after updating the index.
	 */
	if (!__wt_ref_is_root(parent_ref = parent->pg_intl_parent_ref)) {
		WT_ERR(__wt_page_in(session, parent_ref, WT_READ_NO_EVICT));
		hazard = 1;
	}

	pindex = WT_INTL_INDEX_GET_SAFE(parent);
	parent_entries = pindex->entries;

	/*
	 * Remove any refs to deleted pages while we are splitting, we have
	 * the internal page locked down, and are copying the refs into a new
	 * array anyway.  Switch them to the special split state, so that any
	 * reading thread will restart.  Include the ref we are splitting in
	 * the count to be deleted.
	 */
	for (i = 0, deleted_entries = 1; i < parent_entries; ++i) {
		next_ref = pindex->index[i];
		WT_ASSERT(session, next_ref->state != WT_REF_SPLIT);
		if (next_ref->state == WT_REF_DELETED &&
		    __wt_delete_page_skip(session, next_ref) &&
		    WT_ATOMIC_CAS4(next_ref->state,
		    WT_REF_DELETED, WT_REF_SPLIT))
			deleted_entries++;
	}

	/*
	 * The final entry count consists of: The original count, plus any
	 * new pages, less any refs we are removing.
	 */
	result_entries = (parent_entries + new_entries) - deleted_entries;

	/*
	 * Allocate and initialize a new page index array for the parent, then
	 * copy references from the original index array, plus references from
	 * the newly created split array, into place.
	 */
	size = sizeof(WT_PAGE_INDEX) + result_entries * sizeof(WT_REF *);
	WT_ERR(__wt_calloc(session, 1, size, &alloc_index));
	parent_incr += size;
	alloc_index->index = (WT_REF **)(alloc_index + 1);
	alloc_index->entries = result_entries;
	for (alloc_refp = alloc_index->index, i = 0; i < parent_entries; ++i) {
		next_ref = pindex->index[i];
		if (next_ref == ref)
			for (j = 0; j < new_entries; ++j) {
				ref_new[j]->home = parent;
				*alloc_refp++ = ref_new[j];

				/*
				 * Clear the split reference as it moves to the
				 * allocated page index, so it never appears on
				 * both after an error.
				 */
				ref_new[j] = NULL;
			}
		else if (next_ref->state != WT_REF_SPLIT)
			/* Skip refs we have marked for deletion. */
			*alloc_refp++ = next_ref;
	}

	/*
	 * Update the parent page's index: this update makes the split visible
	 * to threads descending the tree.
	 */
	WT_ASSERT(session, WT_INTL_INDEX_GET_SAFE(parent) == pindex);
	WT_INTL_INDEX_SET(parent, alloc_index);
	split_gen = WT_ATOMIC_ADD8(S2C(session)->split_gen, 1);
	alloc_index = NULL;

#ifdef HAVE_DIAGNOSTIC
	__split_verify_intl_key_order(session, parent);
#endif

	/*
	 * Reset the page's original WT_REF field to split.  Threads cursoring
	 * through the tree were blocked because that WT_REF state was set to
	 * locked.  This update changes the locked state to split, unblocking
	 * those threads and causing them to re-calculate their position based
	 * on the updated parent page's index.
	 */
	WT_PUBLISH(ref->state, WT_REF_SPLIT);

	/*
	 * A note on error handling: failures before we swapped the new page
	 * index into the parent can be resolved by freeing allocated memory
	 * because the original page is unchanged, we can continue to use it
	 * and we have not yet modified the parent.  Failures after we swap
	 * the new page index into the parent are also relatively benign, the
	 * split is OK and complete. For those reasons, we ignore errors past
	 * this point unless there's a panic.
	 */
	complete = 1;

	WT_ERR(__wt_verbose(session, WT_VERB_SPLIT,
	    "%s split into parent %" PRIu32 " -> %" PRIu32
	    " (%" PRIu32 ")",
	    __wt_page_type_string(ref->page->type), parent_entries,
	    result_entries, result_entries - parent_entries));

	/*
	 * The new page index is in place, free the WT_REF we were splitting
	 * and any deleted WT_REFs we found, modulo the usual safe free
	 * semantics.
	 */
	for (i = 0; deleted_entries > 0 && i < parent_entries; ++i) {
		next_ref = pindex->index[i];
		if (next_ref->state != WT_REF_SPLIT)
			continue;
		--deleted_entries;

		/*
		 * We set the WT_REF to split, discard it, freeing any resources
		 * it holds.
		 *
		 * Row-store trees where the old version of the page is being
		 * discarded: the previous parent page's key for this child page
		 * may have been an on-page overflow key.  In that case, if the
		 * key hasn't been deleted, delete it now, including its backing
		 * blocks.  We are exchanging the WT_REF that referenced it for
		 * the split page WT_REFs and their keys, and there's no longer
		 * any reference to it.  Done after completing the split (if we
		 * failed, we'd leak the underlying blocks, but the parent page
		 * would be unaffected).
		 */
		if (parent->type == WT_PAGE_ROW_INT) {
			WT_TRET(__split_ovfl_key_cleanup(
			    session, parent, next_ref));
			ikey = __wt_ref_key_instantiated(next_ref);
			if (ikey != NULL) {
				size = sizeof(WT_IKEY) + ikey->size;
				WT_TRET(__split_safe_free(
				    session, split_gen, 0, ikey, size));
				parent_decr += size;
			}
			/*
			 * The page_del structure can be freed immediately: it
			 * is only read when the ref state is WT_REF_DELETED.
			 * The size of the structure wasn't added to the parent,
			 * don't decrement.
			 */
			if (next_ref->page_del != NULL) {
				__wt_free(session,
				    next_ref->page_del->update_list);
				__wt_free(session, next_ref->page_del);
			}
		}

		WT_TRET(__split_safe_free(
		    session, split_gen, 0, next_ref, sizeof(WT_REF)));
		parent_decr += sizeof(WT_REF);
	}

	/* We freed the reference that was split in the loop above. */
	ref = NULL;

	/*
	 * We can't free the previous page index, there may be threads using it.
	 * Add it to the session discard list, to be freed when it's safe.
	 */
	size = sizeof(WT_PAGE_INDEX) + pindex->entries * sizeof(WT_REF *);
	WT_TRET(__split_safe_free(session, split_gen, exclusive, pindex, size));
	parent_decr += size;

	/*
	 * Adjust the parent's memory footprint.
	 */
	__wt_cache_page_inmem_incr(session, parent, parent_incr);
	__wt_cache_page_inmem_decr(session, parent, parent_decr);

	/*
	 * Simple page splits trickle up the tree, that is, as leaf pages grow
	 * large enough and are evicted, they'll split into their parent.  And,
	 * as that parent grows large enough and is evicted, it will split into
	 * its parent and so on.  When the page split wave reaches the root,
	 * the tree will permanently deepen as multiple root pages are written.
	 *	However, this only helps if first, the pages are evicted (and
	 * we resist evicting internal pages for obvious reasons), and second,
	 * if the tree is closed and re-opened from a disk image, which may be
	 * a rare event.
	 *	To avoid the case of internal pages becoming too large when they
	 * aren't being evicted, check internal pages each time a leaf page is
	 * split into them.  If it's big enough, deepen the tree at that point.
	 *	Do the check here because we've just grown the parent page and
	 * are holding it locked.
	 */
	if (ret == 0 && !exclusive &&
	    __split_should_deepen(session, parent_ref, &children))
		ret = __split_deepen(session, parent, children);

err:	if (!complete)
		for (i = 0; i < parent_entries; ++i) {
			next_ref = pindex->index[i];
			if (next_ref->state == WT_REF_SPLIT)
				next_ref->state = WT_REF_DELETED;
		}
	F_CLR_ATOMIC(parent, WT_PAGE_SPLIT_LOCKED);

	if (hazard)
		WT_TRET(__wt_hazard_clear(session, parent));

	__wt_free_ref_index(session, NULL, alloc_index, 0);

	/*
	 * A note on error handling: if we completed the split, return success,
	 * nothing really bad can have happened, and our caller has to proceed
	 * with the split.
	 */
	if (ret != 0 && ret != WT_PANIC)
		__wt_err(session, ret,
		    "ignoring not-fatal error during parent page split");
	return (ret == WT_PANIC || !complete ? ret : 0);
}

/*
 * __wt_split_insert --
 *	Check for pages with append-only workloads and split their last insert
 * list into a separate page.
 */
int
__wt_split_insert(WT_SESSION_IMPL *session, WT_REF *ref, int *splitp)
{
	WT_BTREE *btree;
	WT_DECL_RET;
	WT_DECL_ITEM(key);
	WT_INSERT *ins, **insp, *moved_ins, *prev_ins;
	WT_INSERT_HEAD *ins_head;
	WT_PAGE *page, *right;
	WT_REF *child, *split_ref[2] = { NULL, NULL };
	size_t page_decr, parent_incr, right_incr;
	int i;

	*splitp = 0;

	btree = S2BT(session);
	page = ref->page;
	right = NULL;
	page_decr = parent_incr = right_incr = 0;

	/*
	 * Check for pages with append-only workloads. A common application
	 * pattern is to have multiple threads frantically appending to the
	 * tree. We want to reconcile and evict this page, but we'd like to
	 * do it without making the appending threads wait. If we're not
	 * discarding the tree, check and see if it's worth doing a split to
	 * let the threads continue before doing eviction.
	 *
	 * Ignore anything other than large, dirty row-store leaf pages.
	 *
	 * XXX KEITH
	 * Need a better test for append-only workloads.
	 */
	if (page->type != WT_PAGE_ROW_LEAF ||
	    page->memory_footprint < btree->maxmempage ||
	    !__wt_page_is_modified(page))
		return (0);

	/*
	 * There is no point splitting if the list is small, no deep items is
	 * our heuristic for that. (A 1/4 probability of adding a new skiplist
	 * level means there will be a new 6th level for roughly each 4KB of
	 * entries in the list. If we have at least two 6th level entries, the
	 * list is at least large enough to work with.)
	 *
	 * The following code requires at least two items on the insert list,
	 * this test serves the additional purpose of confirming that.
	 */
#define	WT_MIN_SPLIT_SKIPLIST_DEPTH	WT_MIN(6, WT_SKIP_MAXDEPTH - 1)
	ins_head = page->pg_row_entries == 0 ?
	    WT_ROW_INSERT_SMALLEST(page) :
	    WT_ROW_INSERT_SLOT(page, page->pg_row_entries - 1);
	if (ins_head == NULL ||
	    ins_head->head[WT_MIN_SPLIT_SKIPLIST_DEPTH] == NULL ||
	    ins_head->head[WT_MIN_SPLIT_SKIPLIST_DEPTH] ==
	    ins_head->tail[WT_MIN_SPLIT_SKIPLIST_DEPTH])
		return (0);

	/* Find the last item in the insert list. */
	moved_ins = WT_SKIP_LAST(ins_head);

	/*
	 * Only split a page once, otherwise workloads that update in the middle
	 * of the page could continually split without benefit.
	 */
	if (F_ISSET_ATOMIC(page, WT_PAGE_SPLIT_INSERT))
		return (0);
	F_SET_ATOMIC(page, WT_PAGE_SPLIT_INSERT);

	/*
	 * The first page in the split is the current page, but we still have
	 * to create a replacement WT_REF, the original WT_REF will be set to
	 * split status and eventually freed.
	 */
	WT_ERR(__wt_calloc_one(session, &split_ref[0]));
	child = split_ref[0];
	*child = *ref;

	/*
	 * The new WT_REF is not quite identical: we have to instantiate a key,
	 * and the new reference is visible to readers once the split completes.
	 *
	 * The key-instantiation code checks for races, clear the key fields so
	 * we don't trigger them.
	 */
	child->key.recno = 0;
	child->key.ikey = NULL;
	child->state = WT_REF_MEM;

	/*
	 * Copy the first key from the original page into first ref in the new
	 * parent.  Pages created in memory always have a "smallest" insert
	 * list, so look there first.  If we don't find one, get the first key
	 * from the disk image.
	 *
	 * We can't just use the key from the original ref: it may have been
	 * suffix-compressed, and after the split the truncated key may not be
	 * valid.
	 */
	WT_ERR(__wt_scr_alloc(session, 0, &key));
	if ((ins = WT_SKIP_FIRST(WT_ROW_INSERT_SMALLEST(page))) != NULL) {
		key->data = WT_INSERT_KEY(ins);
		key->size = WT_INSERT_KEY_SIZE(ins);
	} else
		WT_ERR(__wt_row_leaf_key(
		    session, page, &page->pg_row_d[0], key, 1));
	WT_ERR(__wt_row_ikey(session, 0, key->data, key->size, child));
	parent_incr += sizeof(WT_REF) + sizeof(WT_IKEY) + key->size;
	__wt_scr_free(session, &key);

	/*
	 * The second page in the split is a new WT_REF/page pair.
	 */
	WT_ERR(__wt_page_alloc(session, WT_PAGE_ROW_LEAF, 0, 0, 0, &right));
	WT_ERR(__wt_calloc_one(session, &right->pg_row_ins));
	WT_ERR(__wt_calloc_one(session, &right->pg_row_ins[0]));
	right_incr += sizeof(WT_INSERT_HEAD);
	right_incr += sizeof(WT_INSERT_HEAD *);

	WT_ERR(__wt_calloc_one(session, &split_ref[1]));
	child = split_ref[1];
	child->page = right;
	child->state = WT_REF_MEM;
	WT_ERR(__wt_row_ikey(session, 0,
	    WT_INSERT_KEY(moved_ins), WT_INSERT_KEY_SIZE(moved_ins),
	    child));
	parent_incr +=
	    sizeof(WT_REF) + sizeof(WT_IKEY) + WT_INSERT_KEY_SIZE(moved_ins);

	/* The new page is dirty by definition. */
	WT_ERR(__wt_page_modify_init(session, right));
	__wt_page_modify_set(session, right);

	/*
	 * We modified the page above, which will have set the first dirty
	 * transaction to the last transaction current running.  However, the
	 * updates we installed may be older than that.  Set the first dirty
	 * transaction to an impossibly old value so this page is never skipped
	 * in a checkpoint.
	 */
	right->modify->first_dirty_txn = WT_TXN_FIRST;

	/*
	 * Calculate how much memory we're moving: figure out how deep the skip
	 * list stack is for the element we are moving, and the memory used by
	 * the item's list of updates.
	 */
	for (i = 0; i < WT_SKIP_MAXDEPTH && ins_head->tail[i] == moved_ins; ++i)
		;
	WT_MEM_TRANSFER(page_decr, right_incr, sizeof(WT_INSERT) +
	    (size_t)i * sizeof(WT_INSERT *) + WT_INSERT_KEY_SIZE(moved_ins));
	WT_MEM_TRANSFER(
	    page_decr, right_incr, __wt_update_list_memsize(moved_ins->upd));

	/*
	 * Allocation operations completed, move the last insert list item from
	 * the original page to the new page.
	 *
	 * First, update the item to the new child page. (Just append the entry
	 * for simplicity, the previous skip list pointers originally allocated
	 * can be ignored.)
	 */
	right->pg_row_ins[0]->head[0] =
	    right->pg_row_ins[0]->tail[0] = moved_ins;

	/*
	 * Remove the entry from the orig page (i.e truncate the skip list).
	 * Following is an example skip list that might help.
	 *
	 *               __
	 *              |c3|
	 *               |
	 *   __		 __    __
	 *  |a2|--------|c2|--|d2|
	 *   |		 |	|
	 *   __		 __    __	   __
	 *  |a1|--------|c1|--|d1|--------|f1|
	 *   |		 |	|	   |
	 *   __    __    __    __    __    __
	 *  |a0|--|b0|--|c0|--|d0|--|e0|--|f0|
	 *
	 *   From the above picture.
	 *   The head array will be: a0, a1, a2, c3, NULL
	 *   The tail array will be: f0, f1, d2, c3, NULL
	 *   We are looking for: e1, d2, NULL
	 *   If there were no f1, we'd be looking for: e0, NULL
	 *   If there were an f2, we'd be looking for: e0, d1, d2, NULL
	 *
	 *   The algorithm does:
	 *   1) Start at the top of the head list.
	 *   2) Step down until we find a level that contains more than one
	 *      element.
	 *   3) Step across until we reach the tail of the level.
	 *   4) If the tail is the item being moved, remove it.
	 *   5) Drop down a level, and go to step 3 until at level 0.
	 */
	prev_ins = NULL;		/* -Wconditional-uninitialized */
	for (i = WT_SKIP_MAXDEPTH - 1, insp = &ins_head->head[i];
	    i >= 0;
	    i--, insp--) {
		/* Level empty, or a single element. */
		if (ins_head->head[i] == NULL ||
		     ins_head->head[i] == ins_head->tail[i]) {
			/* Remove if it is the element being moved. */
			if (ins_head->head[i] == moved_ins)
				ins_head->head[i] = ins_head->tail[i] = NULL;
			continue;
		}

		for (ins = *insp; ins != ins_head->tail[i]; ins = ins->next[i])
			prev_ins = ins;

		/*
		 * Update the stack head so that we step down as far to the
		 * right as possible. We know that prev_ins is valid since
		 * levels must contain at least two items to be here.
		 */
		insp = &prev_ins->next[i];
		if (ins == moved_ins) {
			/* Remove the item being moved. */
			WT_ASSERT(session, ins_head->head[i] != moved_ins);
			WT_ASSERT(session, prev_ins->next[i] == moved_ins);
			*insp = NULL;
			ins_head->tail[i] = prev_ins;
		}
	}

#ifdef HAVE_DIAGNOSTIC
	/*
	 * Verify the moved insert item appears nowhere on the skip list.
	 */
	for (i = WT_SKIP_MAXDEPTH - 1, insp = &ins_head->head[i];
	    i >= 0;
	    i--, insp--)
		for (ins = *insp; ins != NULL; ins = ins->next[i])
			WT_ASSERT(session, ins != moved_ins);
#endif

	/*
	 * Save the transaction ID when the split happened.  Application
	 * threads will not try to forcibly evict the page again until
	 * all concurrent transactions commit.
	 */
	page->modify->inmem_split_txn = __wt_txn_new_id(session);

	/*
	 * Update the page accounting.
	 *
	 * XXX
	 * If we fail to split the parent, the page's accounting will be wrong.
	 */
	__wt_cache_page_inmem_decr(session, page, page_decr);
	__wt_cache_page_inmem_incr(session, right, right_incr);

	/*
	 * Split into the parent.  After this, the original page is no
	 * longer locked, so we cannot safely look at it.
	 */
	page = NULL;
	if ((ret = __split_parent(
	    session, ref, split_ref, 2, parent_incr, 0)) != 0) {
		/*
		 * Move the insert list element back to the original page list.
		 * For simplicity, the previous skip list pointers originally
		 * allocated can be ignored, just append the entry to the end of
		 * the level 0 list. As before, we depend on the list having
		 * multiple elements and ignore the edge cases small lists have.
		 */
		right->pg_row_ins[0]->head[0] =
		    right->pg_row_ins[0]->tail[0] = NULL;
		ins_head->tail[0]->next[0] = moved_ins;
		ins_head->tail[0] = moved_ins;

		/*
		 * We marked the new page dirty; we're going to discard it, but
		 * first mark it clean and fix up the cache statistics.
		 */
		 right->modify->write_gen = 0;
		 __wt_cache_dirty_decr(session, right);

		WT_ERR(ret);
	}

	/* Let our caller know that we split. */
	*splitp = 1;

	WT_STAT_FAST_CONN_INCR(session, cache_inmem_split);
	WT_STAT_FAST_DATA_INCR(session, cache_inmem_split);

	return (0);

err:	if (split_ref[0] != NULL) {
		__wt_free(session, split_ref[0]->key.ikey);
		__wt_free(session, split_ref[0]);
	}
	if (split_ref[1] != NULL) {
		__wt_free(session, split_ref[1]->key.ikey);
		__wt_free(session, split_ref[1]);
	}
	if (right != NULL)
		__wt_page_out(session, &right);
	__wt_scr_free(session, &key);
	return (ret);
}

/*
 * __wt_split_rewrite --
 *	Resolve a failed reconciliation by replacing a page with a new version.
 */
int
__wt_split_rewrite(WT_SESSION_IMPL *session, WT_REF *ref)
{
	WT_PAGE *page;
	WT_PAGE_MODIFY *mod;
	WT_REF new;

	page = ref->page;
	mod = page->modify;

	/*
	 * This isn't a split: a reconciliation failed because we couldn't write
	 * something, and in the case of forced eviction, we need to stop this
	 * page from being such a problem. We have exclusive access, rewrite the
	 * page in memory. The code lives here because the split code knows how
	 * to re-create a page in memory after it's been reconciled, and that's
	 * exactly what we want to do.
	 *
	 * Build the new page.
	 */
	memset(&new, 0, sizeof(new));
	WT_RET(__split_multi_inmem(session, page, &new, &mod->mod_multi[0]));

	/*
	 * Discard the original page.
	 *
	 * Pages with unresolved changes are not marked clean during
	 * reconciliation, do it now.
	 */
	mod->write_gen = 0;
	__wt_cache_dirty_decr(session, page);
	__wt_ref_out(session, ref);

	/* Swap the new page into place. */
	ref->page = new.page;
	WT_PUBLISH(ref->state, WT_REF_MEM);

	return (0);
}

/*
 * __wt_split_multi --
 *	Resolve a page split.
 */
int
__wt_split_multi(WT_SESSION_IMPL *session, WT_REF *ref, int exclusive)
{
	WT_DECL_RET;
	WT_PAGE *page;
	WT_PAGE_MODIFY *mod;
	WT_REF **ref_new;
	size_t parent_incr;
	uint32_t i, new_entries;

	page = ref->page;
	mod = page->modify;
	new_entries = mod->mod_multi_entries;

	parent_incr = 0;

	/*
	 * Convert the split page's multiblock reconciliation information into
	 * an array of page reference structures.
	 */
	WT_RET(__wt_calloc_def(session, new_entries, &ref_new));
	for (i = 0; i < new_entries; ++i)
		WT_ERR(__wt_multi_to_ref(session,
		    page, &mod->mod_multi[i], &ref_new[i], &parent_incr));

	/* Split into the parent. */
	WT_ERR(__split_parent(
	    session, ref, ref_new, new_entries, parent_incr, exclusive));

	WT_STAT_FAST_CONN_INCR(session, cache_eviction_split);
	WT_STAT_FAST_DATA_INCR(session, cache_eviction_split);

	__wt_free(session, ref_new);

	/*
	 * The split succeeded, discard the page.
	 *
	 * Pages with unresolved changes are not marked clean during
	 * reconciliation, do it now.
	 */
	if (__wt_page_is_modified(page)) {
		 mod->write_gen = 0;
		 __wt_cache_dirty_decr(session, page);
	}
	__wt_page_out(session, &page);

	return (0);

err:	/*
	 * A note on error handling: in the case of evicting a page that has
	 * unresolved changes, we just instantiated some in-memory pages that
	 * reflect those unresolved changes.  The problem is those pages
	 * reference the same WT_UPDATE chains as the page we're splitting,
	 * that is, we simply copied references into the new pages.  If the
	 * split fails, the original page is fine, but discarding the created
	 * page would free those update chains, and that's wrong.  There isn't
	 * an easy solution, there's a lot of small memory allocations in some
	 * common code paths, and unwinding those changes will be difficult.
	 * For now, leak the memory by not discarding the instantiated pages.
	 */
	for (i = 0; i < new_entries; ++i)
		__wt_free_ref(session, page, ref_new[i], 0);
	__wt_free(session, ref_new);
	return (ret);
}
