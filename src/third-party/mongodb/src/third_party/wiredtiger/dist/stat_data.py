# Auto-generate statistics #defines, with initialization, clear and aggregate
# functions.
#
# NOTE: Statistics reports show individual objects as operations per second.
# All objects where that does not make sense should have the word 'currently'
# or the phrase 'in the cache' in their text description, for example, 'files
# currently open'.
# NOTE: All statistics descriptions must have a prefix string followed by ':'.
#
# Optional configuration flags:
#       no_clear        Value not cleared when statistics cleared
#       no_aggregate    Ignore the value when aggregating statistics
#       max_aggregate   Take the maximum value when aggregating statistics
#       no_scale        Don't scale value per second in the logging tool script

from operator import attrgetter
import sys

class Stat:
    def __init__(self, name, tag, desc, flags=''):
        self.name = name
        self.desc = tag + ': ' + desc
        self.flags = flags

    def __cmp__(self, other):
        return cmp(self.desc.lower(), other.desc.lower())

class AsyncStat(Stat):
    prefix = 'async'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, AsyncStat.prefix, desc, flags)
class BlockStat(Stat):
    prefix = 'block-manager'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, BlockStat.prefix, desc, flags)
class BtreeStat(Stat):
    prefix = 'btree'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, BtreeStat.prefix, desc, flags)
class CacheStat(Stat):
    prefix = 'cache'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, CacheStat.prefix, desc, flags)
class CompressStat(Stat):
    prefix = 'compression'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, CompressStat.prefix, desc, flags)
class ConnStat(Stat):
    prefix = 'connection'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, ConnStat.prefix, desc, flags)
class CursorStat(Stat):
    prefix = 'cursor'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, CursorStat.prefix, desc, flags)
class DhandleStat(Stat):
    prefix = 'data-handle'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, DhandleStat.prefix, desc, flags)
class LogStat(Stat):
    prefix = 'log'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, LogStat.prefix, desc, flags)
class LSMStat(Stat):
    prefix = 'LSM'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, LSMStat.prefix, desc, flags)
class RecStat(Stat):
    prefix = 'reconciliation'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, RecStat.prefix, desc, flags)
class SessionStat(Stat):
    prefix = 'session'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, SessionStat.prefix, desc, flags)
class TxnStat(Stat):
    prefix = 'transaction'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, TxnStat.prefix, desc, flags)
class YieldStat(Stat):
    prefix = 'thread-yield'
    def __init__(self, name, desc, flags=''):
        Stat.__init__(self, name, YieldStat.prefix, desc, flags)

##########################################
# Groupings of useful statistics:
# A pre-defined dictionary containing the group name as the key and the
# list of prefix tags that comprise that group.
##########################################
groups = {}
groups['cursor'] = [CursorStat.prefix, SessionStat.prefix]
groups['evict'] = [CacheStat.prefix, ConnStat.prefix, BlockStat.prefix]
groups['lsm'] = [LSMStat.prefix, TxnStat.prefix]
groups['memory'] = [CacheStat.prefix, ConnStat.prefix, RecStat.prefix]
groups['system'] = [ConnStat.prefix, DhandleStat.prefix, SessionStat.prefix]

##########################################
# CONNECTION statistics
##########################################
connection_stats = [
    ##########################################
    # System statistics
    ##########################################
    ConnStat('cond_wait', 'pthread mutex condition wait calls'),
    ConnStat('file_open', 'files currently open', 'no_clear,no_scale'),
    ConnStat('memory_allocation', 'memory allocations'),
    ConnStat('memory_free', 'memory frees'),
    ConnStat('memory_grow', 'memory re-allocations'),
    ConnStat('read_io', 'total read I/Os'),
    ConnStat('rwlock_read', 'pthread mutex shared lock read-lock calls'),
    ConnStat('rwlock_write', 'pthread mutex shared lock write-lock calls'),
    ConnStat('write_io', 'total write I/Os'),

    ##########################################
    # Async API statistics
    ##########################################
    AsyncStat('async_alloc_race', 'number of allocation state races'),
    AsyncStat('async_alloc_view',
        'number of operation slots viewed for allocation'),
    AsyncStat('async_flush', 'number of flush calls'),
    AsyncStat('async_full', 'number of times operation allocation failed'),
    AsyncStat('async_cur_queue', 'current work queue length'),
    AsyncStat('async_max_queue',
        'maximum work queue length', 'max_aggregate,no_scale'),
    AsyncStat('async_nowork', 'number of times worker found no work'),
    AsyncStat('async_op_alloc', 'total allocations'),
    AsyncStat('async_op_compact', 'total compact calls'),
    AsyncStat('async_op_insert', 'total insert calls'),
    AsyncStat('async_op_remove', 'total remove calls'),
    AsyncStat('async_op_search', 'total search calls'),
    AsyncStat('async_op_update', 'total update calls'),

    ##########################################
    # Block manager statistics
    ##########################################
    BlockStat('block_byte_map_read', 'mapped bytes read'),
    BlockStat('block_byte_read', 'bytes read'),
    BlockStat('block_byte_write', 'bytes written'),
    BlockStat('block_map_read', 'mapped blocks read'),
    BlockStat('block_preload', 'blocks pre-loaded'),
    BlockStat('block_read', 'blocks read'),
    BlockStat('block_write', 'blocks written'),

    ##########################################
    # Cache and eviction statistics
    ##########################################
    CacheStat('cache_bytes_dirty',
        'tracked dirty bytes in the cache', 'no_clear,no_scale'),
    CacheStat('cache_bytes_inuse',
        'bytes currently in the cache', 'no_clear,no_scale'),
    CacheStat('cache_bytes_internal',
        'tracked bytes belonging to internal pages in the cache',
        'no_clear,no_scale'),
    CacheStat('cache_bytes_leaf',
        'tracked bytes belonging to leaf pages in the cache',
        'no_clear,no_scale'),
    CacheStat('cache_bytes_max',
        'maximum bytes configured', 'no_clear,no_scale'),
    CacheStat('cache_bytes_overflow',
        'tracked bytes belonging to overflow pages in the cache',
        'no_clear,no_scale'),
    CacheStat('cache_bytes_read', 'bytes read into cache'),
    CacheStat('cache_bytes_write', 'bytes written from cache'),
    CacheStat('cache_eviction_app', 'pages evicted by application threads'),
    CacheStat('cache_eviction_clean', 'unmodified pages evicted'),
    CacheStat('cache_eviction_deepen',
        'page split during eviction deepened the tree'),
    CacheStat('cache_eviction_dirty', 'modified pages evicted'),
    CacheStat('cache_eviction_checkpoint', 'checkpoint blocked page eviction'),
    CacheStat('cache_eviction_fail',
        'pages selected for eviction unable to be evicted'),
    CacheStat('cache_eviction_force',
        'pages evicted because they exceeded the in-memory maximum'),
    CacheStat('cache_eviction_force_delete',
        'pages evicted because they had chains of deleted items'),
    CacheStat('cache_eviction_force_fail',
        'failed eviction of pages that exceeded the in-memory maximum'),
    CacheStat('cache_eviction_hazard', 'hazard pointer blocked page eviction'),
    CacheStat('cache_eviction_internal', 'internal pages evicted'),
    CacheStat('cache_eviction_maximum_page_size',
        'maximum page size at eviction', 'max_aggregate,no_scale'),
    CacheStat('cache_eviction_queue_empty',
        'eviction server candidate queue empty when topping up'),
    CacheStat('cache_eviction_queue_not_empty',
        'eviction server candidate queue not empty when topping up'),
    CacheStat('cache_eviction_server_evicting',
        'eviction server evicting pages'),
    CacheStat('cache_eviction_server_not_evicting',
        'eviction server populating queue, but not evicting pages'),
    CacheStat('cache_eviction_slow',
        'eviction server unable to reach eviction goal'),
    CacheStat('cache_eviction_split', 'pages split during eviction'),
    CacheStat('cache_eviction_walk', 'pages walked for eviction'),
    CacheStat('cache_eviction_worker_evicting',
        'eviction worker thread evicting pages'),
    CacheStat('cache_inmem_split', 'in-memory page splits'),
    CacheStat('cache_overhead', 'percentage overhead', 'no_clear,no_scale'),
    CacheStat('cache_pages_dirty',
        'tracked dirty pages in the cache', 'no_clear,no_scale'),
    CacheStat('cache_pages_inuse',
        'pages currently held in the cache', 'no_clear,no_scale'),
    CacheStat('cache_read', 'pages read into cache'),
    CacheStat('cache_write', 'pages written from cache'),

    ##########################################
    # Dhandle statistics
    ##########################################
    DhandleStat('dh_conn_handle_count',
        'connection data handles currently active', 'no_clear,no_scale'),
    DhandleStat('dh_sweep_close', 'connection sweep dhandles closed'),
    DhandleStat('dh_sweep_remove',
        'connection sweep dhandles removed from hash list'),
    DhandleStat('dh_sweep_ref', 'connection sweep candidate became referenced'),
    DhandleStat('dh_sweep_tod', 'connection sweep time-of-death sets'),
    DhandleStat('dh_sweeps', 'connection sweeps'),
    DhandleStat('dh_session_handles', 'session dhandles swept'),
    DhandleStat('dh_session_sweeps', 'session sweep attempts'),

    ##########################################
    # Logging statistics
    ##########################################
    LogStat('log_buffer_size', 'total log buffer size', 'no_clear,no_scale'),
    LogStat('log_bytes_payload', 'log bytes of payload data'),
    LogStat('log_bytes_written', 'log bytes written'),
    LogStat('log_close_yields', 'yields waiting for previous log file close'),
    LogStat('log_compress_len', 'total size of compressed records'),
    LogStat('log_compress_mem', 'total in-memory size of compressed records'),
    LogStat('log_compress_small', 'log records too small to compress'),
    LogStat('log_compress_writes', 'log records compressed'),
    LogStat('log_compress_write_fails', 'log records not compressed'),
    LogStat('log_max_filesize', 'maximum log file size', 'no_clear,no_scale'),
    LogStat('log_prealloc_files', 'pre-allocated log files prepared'),
    LogStat('log_prealloc_max',
        'number of pre-allocated log files to create', 'no_clear,no_scale'),
    LogStat('log_prealloc_used', 'pre-allocated log files used'),
    LogStat('log_reads', 'log read operations'),
    LogStat('log_release_write_lsn', 'log release advances write LSN'),
    LogStat('log_scan_records', 'records processed by log scan'),
    LogStat('log_scan_rereads', 'log scan records requiring two reads'),
    LogStat('log_scans', 'log scan operations'),
    LogStat('log_sync', 'log sync operations'),
    LogStat('log_sync_dir', 'log sync_dir operations'),
    LogStat('log_writes', 'log write operations'),
    LogStat('log_write_lsn', 'log server thread advances write LSN'),

    LogStat('log_slot_consolidated', 'logging bytes consolidated'),
    LogStat('log_slot_closes', 'consolidated slot closures'),
    LogStat('log_slot_joins', 'consolidated slot joins'),
    LogStat('log_slot_races', 'consolidated slot join races'),
    LogStat('log_slot_switch_fails',
        'slots selected for switching that were unavailable'),
    LogStat('log_slot_toobig', 'record size exceeded maximum'),
    LogStat('log_slot_toosmall',
        'failed to find a slot large enough for record'),
    LogStat('log_slot_transitions', 'consolidated slot join transitions'),

    ##########################################
    # Reconciliation statistics
    ##########################################
    RecStat('rec_pages', 'page reconciliation calls'),
    RecStat('rec_pages_eviction', 'page reconciliation calls for eviction'),
    RecStat('rec_split_stashed_bytes',
        'split bytes currently awaiting free', 'no_clear,no_scale'),
    RecStat('rec_split_stashed_objects',
        'split objects currently awaiting free', 'no_clear,no_scale'),

    ##########################################
    # Transaction statistics
    ##########################################
    TxnStat('txn_begin', 'transaction begins'),
    TxnStat('txn_checkpoint', 'transaction checkpoints'),
    TxnStat('txn_checkpoint_generation',
        'transaction checkpoint generation', 'no_clear,no_scale'),
    TxnStat('txn_checkpoint_time_max',
        'transaction checkpoint max time (msecs)',
        'no_aggregate,no_clear,no_scale'),
    TxnStat('txn_checkpoint_time_min',
        'transaction checkpoint min time (msecs)',
        'max_aggregate,no_clear,no_scale'),
    TxnStat('txn_checkpoint_time_recent',
        'transaction checkpoint most recent time (msecs)',
        'no_aggregate,no_clear,no_scale'),
    TxnStat('txn_checkpoint_time_total',
        'transaction checkpoint total time (msecs)',
        'no_aggregate,no_clear,no_scale'),
    TxnStat('txn_checkpoint_running',
        'transaction checkpoint currently running',
        'no_aggregate,no_clear,no_scale'),
    TxnStat('txn_pinned_checkpoint_range',
        'transaction range of IDs currently pinned by a checkpoint',
        'no_aggregate,no_clear,no_scale'),
    TxnStat('txn_pinned_range',
        'transaction range of IDs currently pinned',
        'no_aggregate,no_clear,no_scale'),
    TxnStat('txn_commit', 'transactions committed'),
    TxnStat('txn_fail_cache', 'transaction failures due to cache overflow'),
    TxnStat('txn_rollback', 'transactions rolled back'),

    ##########################################
    # LSM statistics
    ##########################################
    LSMStat('lsm_checkpoint_throttle', 'sleep for LSM checkpoint throttle'),
    LSMStat('lsm_merge_throttle', 'sleep for LSM merge throttle'),
    LSMStat('lsm_rows_merged', 'rows merged in an LSM tree'),
    LSMStat('lsm_work_queue_app',
        'application work units currently queued', 'no_clear,no_scale'),
    LSMStat('lsm_work_queue_manager',
        'merge work units currently queued', 'no_clear,no_scale'),
    LSMStat('lsm_work_queue_max', 'tree queue hit maximum'),
    LSMStat('lsm_work_queue_switch',
        'switch work units currently queued', 'no_clear,no_scale'),
    LSMStat('lsm_work_units_created', 'tree maintenance operations scheduled'),
    LSMStat('lsm_work_units_discarded',
        'tree maintenance operations discarded'),
    LSMStat('lsm_work_units_done', 'tree maintenance operations executed'),

    ##########################################
    # Session operations
    ##########################################
    SessionStat('session_cursor_open',
        'open cursor count', 'no_clear,no_scale'),
    SessionStat('session_open', 'open session count', 'no_clear,no_scale'),

    ##########################################
    # Total cursor operations
    ##########################################
    CursorStat('cursor_create', 'cursor create calls'),
    CursorStat('cursor_insert', 'cursor insert calls'),
    CursorStat('cursor_next', 'cursor next calls'),
    CursorStat('cursor_prev', 'cursor prev calls'),
    CursorStat('cursor_remove', 'cursor remove calls'),
    CursorStat('cursor_reset', 'cursor reset calls'),
    CursorStat('cursor_search', 'cursor search calls'),
    CursorStat('cursor_search_near', 'cursor search near calls'),
    CursorStat('cursor_update', 'cursor update calls'),

    ##########################################
    # Yield statistics
    ##########################################
    YieldStat('page_busy_blocked', 'page acquire busy blocked'),
    YieldStat('page_forcible_evict_blocked', 'page acquire eviction blocked'),
    YieldStat('page_locked_blocked', 'page acquire locked blocked'),
    YieldStat('page_read_blocked', 'page acquire read blocked'),
    YieldStat('page_sleep', 'page acquire time sleeping (usecs)'),
]

connection_stats = sorted(connection_stats, key=attrgetter('name'))

##########################################
# Data source statistics
##########################################
dsrc_stats = [
    ##########################################
    # Session operations
    ##########################################
    SessionStat('session_compact', 'object compaction'),
    SessionStat('session_cursor_open',
        'open cursor count', 'no_clear,no_scale'),

    ##########################################
    # Cursor operations
    ##########################################
    CursorStat('cursor_create', 'create calls'),
    CursorStat('cursor_insert', 'insert calls'),
    CursorStat('cursor_insert_bulk', 'bulk-loaded cursor-insert calls'),
    CursorStat('cursor_insert_bytes',
        'cursor-insert key and value bytes inserted'),
    CursorStat('cursor_next', 'next calls'),
    CursorStat('cursor_prev', 'prev calls'),
    CursorStat('cursor_remove', 'remove calls'),
    CursorStat('cursor_remove_bytes', 'cursor-remove key bytes removed'),
    CursorStat('cursor_reset', 'reset calls'),
    CursorStat('cursor_search', 'search calls'),
    CursorStat('cursor_search_near', 'search near calls'),
    CursorStat('cursor_update', 'update calls'),
    CursorStat('cursor_update_bytes', 'cursor-update value bytes updated'),

    ##########################################
    # Btree statistics
    ##########################################
    BtreeStat('btree_checkpoint_generation',
        'btree checkpoint generation', 'no_clear,no_scale'),
    BtreeStat('btree_column_deleted',
        'column-store variable-size deleted values', 'no_scale'),
    BtreeStat('btree_column_fix',
        'column-store fixed-size leaf pages', 'no_scale'),
    BtreeStat('btree_column_internal',
        'column-store internal pages', 'no_scale'),
    BtreeStat('btree_column_variable',
        'column-store variable-size leaf pages', 'no_scale'),
    BtreeStat('btree_compact_rewrite', 'pages rewritten by compaction'),
    BtreeStat('btree_entries', 'number of key/value pairs', 'no_scale'),
    BtreeStat('btree_fixed_len', 'fixed-record size', 'no_aggregate,no_scale'),
    BtreeStat('btree_maximum_depth',
        'maximum tree depth', 'max_aggregate,no_scale'),
    BtreeStat('btree_maxintlkey',
        'maximum internal page key size', 'no_aggregate,no_scale'),
    BtreeStat('btree_maxintlpage',
        'maximum internal page size', 'no_aggregate,no_scale'),
    BtreeStat('btree_maxleafkey',
        'maximum leaf page key size', 'no_aggregate,no_scale'),
    BtreeStat('btree_maxleafpage',
        'maximum leaf page size', 'no_aggregate,no_scale'),
    BtreeStat('btree_maxleafvalue',
        'maximum leaf page value size', 'no_aggregate,no_scale'),
    BtreeStat('btree_overflow', 'overflow pages', 'no_scale'),
    BtreeStat('btree_row_internal', 'row-store internal pages', 'no_scale'),
    BtreeStat('btree_row_leaf', 'row-store leaf pages', 'no_scale'),

    ##########################################
    # LSM statistics
    ##########################################
    LSMStat('bloom_count', 'bloom filters in the LSM tree', 'no_scale'),
    LSMStat('bloom_false_positive', 'bloom filter false positives'),
    LSMStat('bloom_hit', 'bloom filter hits'),
    LSMStat('bloom_miss', 'bloom filter misses'),
    LSMStat('bloom_page_evict', 'bloom filter pages evicted from cache'),
    LSMStat('bloom_page_read', 'bloom filter pages read into cache'),
    LSMStat('bloom_size', 'total size of bloom filters', 'no_scale'),
    LSMStat('lsm_checkpoint_throttle', 'sleep for LSM checkpoint throttle'),
    LSMStat('lsm_chunk_count',
        'chunks in the LSM tree', 'no_aggregate,no_scale'),
    LSMStat('lsm_generation_max',
        'highest merge generation in the LSM tree', 'max_aggregate,no_scale'),
    LSMStat('lsm_lookup_no_bloom',
        'queries that could have benefited ' +
        'from a Bloom filter that did not exist'),
    LSMStat('lsm_merge_throttle', 'sleep for LSM merge throttle'),

    ##########################################
    # Block manager statistics
    ##########################################
    BlockStat('block_alloc', 'blocks allocated'),
    BlockStat('allocation_size',
        'file allocation unit size', 'no_aggregate,no_scale'),
    BlockStat('block_checkpoint_size', 'checkpoint size', 'no_scale'),
    BlockStat('block_extension', 'allocations requiring file extension'),
    BlockStat('block_free', 'blocks freed'),
    BlockStat('block_magic', 'file magic number', 'no_aggregate,no_scale'),
    BlockStat('block_major',
        'file major version number', 'no_aggregate,no_scale'),
    BlockStat('block_minor', 'minor version number', 'no_aggregate,no_scale'),
    BlockStat('block_reuse_bytes', 'file bytes available for reuse'),
    BlockStat('block_size', 'file size in bytes', 'no_scale'),

    ##########################################
    # Cache and eviction statistics
    ##########################################
    CacheStat('cache_bytes_read', 'bytes read into cache'),
    CacheStat('cache_bytes_write', 'bytes written from cache'),
    CacheStat('cache_eviction_checkpoint', 'checkpoint blocked page eviction'),
    CacheStat('cache_eviction_clean', 'unmodified pages evicted'),
    CacheStat('cache_eviction_deepen',
        'page split during eviction deepened the tree'),
    CacheStat('cache_eviction_dirty', 'modified pages evicted'),
    CacheStat('cache_eviction_fail',
        'data source pages selected for eviction unable to be evicted'),
    CacheStat('cache_eviction_hazard', 'hazard pointer blocked page eviction'),
    CacheStat('cache_eviction_internal', 'internal pages evicted'),
    CacheStat('cache_eviction_split', 'pages split during eviction'),
    CacheStat('cache_inmem_split', 'in-memory page splits'),
    CacheStat('cache_overflow_value',
        'overflow values cached in memory', 'no_scale'),
    CacheStat('cache_read', 'pages read into cache'),
    CacheStat('cache_read_overflow', 'overflow pages read into cache'),
    CacheStat('cache_write', 'pages written from cache'),

    ##########################################
    # Compression statistics
    ##########################################
    CompressStat('compress_raw_ok', 'raw compression call succeeded'),
    CompressStat('compress_raw_fail',
        'raw compression call failed, no additional data available'),
    CompressStat('compress_raw_fail_temporary',
        'raw compression call failed, additional data available'),
    CompressStat('compress_read', 'compressed pages read'),
    CompressStat('compress_write', 'compressed pages written'),
    CompressStat('compress_write_fail', 'page written failed to compress'),
    CompressStat('compress_write_too_small',
        'page written was too small to compress'),

    ##########################################
    # Reconciliation statistics
    ##########################################
    RecStat('rec_dictionary', 'dictionary matches'),
    RecStat('rec_overflow_key_internal', 'internal-page overflow keys'),
    RecStat('rec_overflow_key_leaf', 'leaf-page overflow keys'),
    RecStat('rec_overflow_value', 'overflow values written'),
    RecStat('rec_page_match', 'page checksum matches'),
    RecStat('rec_page_delete', 'pages deleted'),
    RecStat('rec_pages', 'page reconciliation calls'),
    RecStat('rec_pages_eviction', 'page reconciliation calls for eviction'),
    RecStat('rec_prefix_compression',
        'leaf page key bytes discarded using prefix compression'),
    RecStat('rec_suffix_compression',
        'internal page key bytes discarded using suffix compression'),
    RecStat('rec_multiblock_internal', 'internal page multi-block writes'),
    RecStat('rec_multiblock_leaf', 'leaf page multi-block writes'),
    RecStat('rec_multiblock_max',
        'maximum blocks required for a page', 'max_aggregate,no_scale'),

    ##########################################
    # Transaction statistics
    ##########################################
    TxnStat('txn_update_conflict', 'update conflicts'),
]

dsrc_stats = sorted(dsrc_stats, key=attrgetter('name'))
