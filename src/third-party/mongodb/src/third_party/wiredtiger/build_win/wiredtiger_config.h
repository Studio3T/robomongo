/* wiredtiger_config.h.  Generated from config.hin by configure.  */
/* build_posix/config.hin.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 to pause for debugger attach on failure. */
/* #undef HAVE_ATTACH */

/* Build the LevelDB API with Basho LevelDB support. */
/* #undef HAVE_BASHOLEVELDB */

/* LZ4 support automatically loaded. */
/* #undef HAVE_BUILTIN_EXTENSION_LZ4 */

/* Snappy support automatically loaded. */
/* #undef HAVE_BUILTIN_EXTENSION_SNAPPY */

/* Zlib support automatically loaded. */
/* #undef HAVE_BUILTIN_EXTENSION_ZLIB */

/* Define to 1 if you have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define to 1 for diagnostic tests. */
/* #undef HAVE_DIAGNOSTIC */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `fallocate' function. */
/* #undef HAVE_FALLOCATE */

/* Define to 1 if you have the `fcntl' function. */
/* #undef HAVE_FCNTL 1  */

/* Define to 1 if you have the `fdatasync' function. */
/* #undef HAVE_FDATASYNC */

/* Define to 1 if you have the `fread_unlocked' function. */
/* #undef HAVE_FREAD_UNLOCKED */

/* Define to 1 if you have the `ftruncate' function. */
/* #undef HAVE_FTRUNCATE */

/* Define to 1 if you have the `gettimeofday' function. */
/* #undef HAVE_GETTIMEOFDAY */

/* Build the LevelDB API with HyperLevelDB support. */
/* #undef HAVE_HYPERLEVELDB */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `bz2' library (-lbz2). */
/* #undef HAVE_LIBBZ2 */

/* Define to 1 if you have the `dl' library (-ldl). */
/* #undef HAVE_LIBDL */

/* Define to 1 if you have the `lz4' library (-llz4). */
/* #undef HAVE_LIBLZ4 */

/* Define to 1 if you have the `pthread' library (-lpthread). */
/* #undef HAVE_LIBPTHREAD */

/* Define to 1 if you have the `rt' library (-lrt). */
/* #undef HAVE_LIBRT */

/* Define to 1 if you have the `snappy' library (-lsnappy). */
/* #undef HAVE_LIBSNAPPY */

/* Define to 1 if you have the `tcmalloc' library (-ltcmalloc). */
/* #undef HAVE_LIBTCMALLOC */

/* Define to 1 if you have the `z' library (-lz). */
/* #undef HAVE_LIBZ */

/* Define to 1 if you have the <memory.h> header file. */
/* #undef HAVE_MEMORY_H */

/* Define to 1 if you have the `posix_fadvise' function. */
/* #undef HAVE_POSIX_FADVISE */

/* Define to 1 if you have the `posix_fallocate' function. */
/* #undef HAVE_POSIX_FALLOCATE */

/* Define to 1 if you have the `posix_madvise' function. */
/* #undef HAVE_POSIX_MADVISE */

/* Define to 1 if you have the `posix_memalign' function. */
/* #undef HAVE_POSIX_MEMALIGN */

/* Define to 1 if you have the <pthread_np.h> header file. */
/* #undef HAVE_PTHREAD_NP_H */

/* Build the LevelDB API with RocksDB support. */
/* #undef HAVE_ROCKSDB */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
/* #undef HAVE_STRINGS_H */

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtouq' function. */
/* #undef HAVE_STRTOUQ */

/* Define to 1 if you have the `sync_file_range' function. */
/* #undef HAVE_SYNC_FILE_RANGE */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Enable verbose message configuration. */
/* #undef HAVE_VERBOSE */

/* Spinlock type from mutex.h. */
#define SPINLOCK_TYPE SPINLOCK_MSVC

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Default alignment of buffers used for I/O */
#define WT_BUFFER_ALIGNMENT_DEFAULT 0

/* Enable large inode numbers on Mac OS X 10.5.  */
/* #ifndef _DARWIN_USE_64_BIT_INODE */
/* # define _DARWIN_USE_64_BIT_INODE 1 */
/* #endif */

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */
