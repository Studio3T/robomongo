/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

/*
 * Publish a value to a shared location.  All previous stores must complete
 * before the value is made public.
 */
#define	WT_PUBLISH(v, val) do {						\
	WT_WRITE_BARRIER();						\
	(v) = (val);							\
} while (0)

/*
 * Read a shared location and guarantee that subsequent reads do not see any
 * earlier state.
 */
#define	WT_ORDERED_READ(v, val) do {					\
	(v) = (val);							\
	WT_READ_BARRIER();						\
} while (0)

/*
 * Atomic versions of the flag set/clear macros.
 */
#define	F_ISSET_ATOMIC(p, mask)	((p)->flags_atomic & (uint8_t)(mask))

#define	F_SET_ATOMIC(p, mask) do {					\
	uint8_t __orig;							\
	do {								\
		__orig = (p)->flags_atomic;				\
	} while (!WT_ATOMIC_CAS1((p)->flags_atomic,			\
	    __orig, __orig | (uint8_t)(mask)));				\
} while (0)

#define	F_CAS_ATOMIC(p, mask, ret) do {					\
	uint8_t __orig;						\
	ret = 0;							\
	do {								\
		__orig = (p)->flags_atomic;				\
		if ((__orig & (uint8_t)(mask)) != 0) {			\
			ret = EBUSY;					\
			break;						\
		}							\
	} while (!WT_ATOMIC_CAS1((p)->flags_atomic,			\
	    __orig, __orig | (uint8_t)(mask)));				\
} while (0)

#define	F_CLR_ATOMIC(p, mask)	do {					\
	uint8_t __orig;							\
	do {								\
		__orig = (p)->flags_atomic;				\
	} while (!WT_ATOMIC_CAS1((p)->flags_atomic,			\
	    __orig, __orig & ~(uint8_t)(mask)));			\
} while (0)

#define	WT_CACHE_LINE_ALIGNMENT	64	/* Cache line alignment */
