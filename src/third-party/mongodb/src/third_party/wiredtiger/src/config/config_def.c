/* DO NOT EDIT: automatically built by dist/api_config.py. */

#include "wt_internal.h"

static const WT_CONFIG_CHECK confchk_colgroup_meta[] = {
	{ "app_metadata", "string", NULL, NULL, NULL },
	{ "collator", "string", __wt_collator_confchk, NULL, NULL },
	{ "columns", "list", NULL, NULL, NULL },
	{ "source", "string", NULL, NULL, NULL },
	{ "type", "string", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_connection_async_new_op[] = {
	{ "append", "boolean", NULL, NULL, NULL },
	{ "overwrite", "boolean", NULL, NULL, NULL },
	{ "raw", "boolean", NULL, NULL, NULL },
	{ "timeout", "int", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_connection_close[] = {
	{ "leak_memory", "boolean", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_connection_load_extension[] = {
	{ "config", "string", NULL, NULL, NULL },
	{ "entry", "string", NULL, NULL, NULL },
	{ "terminate", "string", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_connection_open_session[] = {
	{ "isolation", "string",
	    NULL, "choices=[\"read-uncommitted\",\"read-committed\","
	    "\"snapshot\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_async_subconfigs[] = {
	{ "enabled", "boolean", NULL, NULL, NULL },
	{ "ops_max", "int", NULL, "min=1,max=4096", NULL },
	{ "threads", "int", NULL, "min=1,max=20", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_checkpoint_subconfigs[] = {
	{ "log_size", "int", NULL, "min=0,max=2GB", NULL },
	{ "name", "string", NULL, NULL, NULL },
	{ "wait", "int", NULL, "min=0,max=100000", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_eviction_subconfigs[] = {
	{ "threads_max", "int", NULL, "min=1,max=20", NULL },
	{ "threads_min", "int", NULL, "min=1,max=20", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_file_manager_subconfigs[] = {
	{ "close_handle_minimum", "string", NULL, NULL, NULL },
	{ "close_idle_time", "int", NULL, "min=0,max=100000", NULL },
	{ "close_scan_interval", "int",
	    NULL, "min=1,max=100000",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_lsm_manager_subconfigs[] = {
	{ "merge", "boolean", NULL, NULL, NULL },
	{ "worker_thread_max", "int", NULL, "min=3,max=20", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_shared_cache_subconfigs[] = {
	{ "chunk", "int", NULL, "min=1MB,max=10TB", NULL },
	{ "name", "string", NULL, NULL, NULL },
	{ "reserve", "int", NULL, NULL, NULL },
	{ "size", "int", NULL, "min=1MB,max=10TB", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_statistics_log_subconfigs[] = {
	{ "on_close", "boolean", NULL, NULL, NULL },
	{ "path", "string", NULL, NULL, NULL },
	{ "sources", "list", NULL, NULL, NULL },
	{ "timestamp", "string", NULL, NULL, NULL },
	{ "wait", "int", NULL, "min=0,max=100000", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_connection_reconfigure[] = {
	{ "async", "category", NULL, NULL, confchk_async_subconfigs },
	{ "cache_overhead", "int", NULL, "min=0,max=30", NULL },
	{ "cache_size", "int", NULL, "min=1MB,max=10TB", NULL },
	{ "checkpoint", "category",
	    NULL, NULL,
	    confchk_checkpoint_subconfigs },
	{ "error_prefix", "string", NULL, NULL, NULL },
	{ "eviction", "category",
	    NULL, NULL,
	    confchk_eviction_subconfigs },
	{ "eviction_dirty_target", "int",
	    NULL, "min=10,max=99",
	    NULL },
	{ "eviction_target", "int", NULL, "min=10,max=99", NULL },
	{ "eviction_trigger", "int", NULL, "min=10,max=99", NULL },
	{ "file_manager", "category",
	    NULL, NULL,
	    confchk_file_manager_subconfigs },
	{ "lsm_manager", "category",
	    NULL, NULL,
	    confchk_lsm_manager_subconfigs },
	{ "lsm_merge", "boolean", NULL, NULL, NULL },
	{ "shared_cache", "category",
	    NULL, NULL,
	    confchk_shared_cache_subconfigs },
	{ "statistics", "list",
	    NULL, "choices=[\"all\",\"fast\",\"none\",\"clear\"]",
	    NULL },
	{ "statistics_log", "category",
	    NULL, NULL,
	    confchk_statistics_log_subconfigs },
	{ "verbose", "list",
	    NULL, "choices=[\"api\",\"block\",\"checkpoint\",\"compact\","
	    "\"evict\",\"evictserver\",\"fileops\",\"log\",\"lsm\","
	    "\"metadata\",\"mutex\",\"overflow\",\"read\",\"reconcile\","
	    "\"recovery\",\"salvage\",\"shared_cache\",\"split\","
	    "\"temporary\",\"transaction\",\"verify\",\"version\",\"write\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_cursor_reconfigure[] = {
	{ "append", "boolean", NULL, NULL, NULL },
	{ "overwrite", "boolean", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_file_meta[] = {
	{ "allocation_size", "int", NULL, "min=512B,max=128MB", NULL },
	{ "app_metadata", "string", NULL, NULL, NULL },
	{ "block_allocation", "string",
	    NULL, "choices=[\"first\",\"best\"]",
	    NULL },
	{ "block_compressor", "string",
	    __wt_compressor_confchk, NULL,
	    NULL },
	{ "cache_resident", "boolean", NULL, NULL, NULL },
	{ "checkpoint", "string", NULL, NULL, NULL },
	{ "checkpoint_lsn", "string", NULL, NULL, NULL },
	{ "checksum", "string",
	    NULL, "choices=[\"on\",\"off\",\"uncompressed\"]",
	    NULL },
	{ "collator", "string", __wt_collator_confchk, NULL, NULL },
	{ "columns", "list", NULL, NULL, NULL },
	{ "dictionary", "int", NULL, "min=0", NULL },
	{ "format", "string", NULL, "choices=[\"btree\"]", NULL },
	{ "huffman_key", "string", __wt_huffman_confchk, NULL, NULL },
	{ "huffman_value", "string",
	    __wt_huffman_confchk, NULL,
	    NULL },
	{ "id", "string", NULL, NULL, NULL },
	{ "internal_item_max", "int", NULL, "min=0", NULL },
	{ "internal_key_max", "int", NULL, "min=0", NULL },
	{ "internal_key_truncate", "boolean", NULL, NULL, NULL },
	{ "internal_page_max", "int",
	    NULL, "min=512B,max=512MB",
	    NULL },
	{ "key_format", "format", __wt_struct_confchk, NULL, NULL },
	{ "key_gap", "int", NULL, "min=0", NULL },
	{ "leaf_item_max", "int", NULL, "min=0", NULL },
	{ "leaf_key_max", "int", NULL, "min=0", NULL },
	{ "leaf_page_max", "int", NULL, "min=512B,max=512MB", NULL },
	{ "leaf_value_max", "int", NULL, "min=0", NULL },
	{ "memory_page_max", "int", NULL, "min=512B,max=10TB", NULL },
	{ "os_cache_dirty_max", "int", NULL, "min=0", NULL },
	{ "os_cache_max", "int", NULL, "min=0", NULL },
	{ "prefix_compression", "boolean", NULL, NULL, NULL },
	{ "prefix_compression_min", "int", NULL, "min=0", NULL },
	{ "split_deepen_min_child", "int", NULL, NULL, NULL },
	{ "split_deepen_per_child", "int", NULL, NULL, NULL },
	{ "split_pct", "int", NULL, "min=25,max=100", NULL },
	{ "value_format", "format", __wt_struct_confchk, NULL, NULL },
	{ "version", "string", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_index_meta[] = {
	{ "app_metadata", "string", NULL, NULL, NULL },
	{ "collator", "string", __wt_collator_confchk, NULL, NULL },
	{ "columns", "list", NULL, NULL, NULL },
	{ "extractor", "string", __wt_extractor_confchk, NULL, NULL },
	{ "immutable", "boolean", NULL, NULL, NULL },
	{ "index_key_columns", "int", NULL, NULL, NULL },
	{ "key_format", "format", __wt_struct_confchk, NULL, NULL },
	{ "source", "string", NULL, NULL, NULL },
	{ "type", "string", NULL, NULL, NULL },
	{ "value_format", "format", __wt_struct_confchk, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_begin_transaction[] = {
	{ "isolation", "string",
	    NULL, "choices=[\"read-uncommitted\",\"read-committed\","
	    "\"snapshot\"]",
	    NULL },
	{ "name", "string", NULL, NULL, NULL },
	{ "priority", "int", NULL, "min=-100,max=100", NULL },
	{ "sync", "boolean", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_checkpoint[] = {
	{ "drop", "list", NULL, NULL, NULL },
	{ "force", "boolean", NULL, NULL, NULL },
	{ "name", "string", NULL, NULL, NULL },
	{ "target", "list", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_compact[] = {
	{ "timeout", "int", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_lsm_subconfigs[] = {
	{ "auto_throttle", "boolean", NULL, NULL, NULL },
	{ "bloom", "boolean", NULL, NULL, NULL },
	{ "bloom_bit_count", "int", NULL, "min=2,max=1000", NULL },
	{ "bloom_config", "string", NULL, NULL, NULL },
	{ "bloom_hash_count", "int", NULL, "min=2,max=100", NULL },
	{ "bloom_oldest", "boolean", NULL, NULL, NULL },
	{ "chunk_count_limit", "int", NULL, NULL, NULL },
	{ "chunk_max", "int", NULL, "min=100MB,max=10TB", NULL },
	{ "chunk_size", "int", NULL, "min=512K,max=500MB", NULL },
	{ "merge_max", "int", NULL, "min=2,max=100", NULL },
	{ "merge_min", "int", NULL, "max=100", NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_create[] = {
	{ "allocation_size", "int", NULL, "min=512B,max=128MB", NULL },
	{ "app_metadata", "string", NULL, NULL, NULL },
	{ "block_allocation", "string",
	    NULL, "choices=[\"first\",\"best\"]",
	    NULL },
	{ "block_compressor", "string",
	    __wt_compressor_confchk, NULL,
	    NULL },
	{ "cache_resident", "boolean", NULL, NULL, NULL },
	{ "checksum", "string",
	    NULL, "choices=[\"on\",\"off\",\"uncompressed\"]",
	    NULL },
	{ "colgroups", "list", NULL, NULL, NULL },
	{ "collator", "string", __wt_collator_confchk, NULL, NULL },
	{ "columns", "list", NULL, NULL, NULL },
	{ "dictionary", "int", NULL, "min=0", NULL },
	{ "exclusive", "boolean", NULL, NULL, NULL },
	{ "extractor", "string", __wt_extractor_confchk, NULL, NULL },
	{ "format", "string", NULL, "choices=[\"btree\"]", NULL },
	{ "huffman_key", "string", __wt_huffman_confchk, NULL, NULL },
	{ "huffman_value", "string",
	    __wt_huffman_confchk, NULL,
	    NULL },
	{ "immutable", "boolean", NULL, NULL, NULL },
	{ "internal_item_max", "int", NULL, "min=0", NULL },
	{ "internal_key_max", "int", NULL, "min=0", NULL },
	{ "internal_key_truncate", "boolean", NULL, NULL, NULL },
	{ "internal_page_max", "int",
	    NULL, "min=512B,max=512MB",
	    NULL },
	{ "key_format", "format", __wt_struct_confchk, NULL, NULL },
	{ "key_gap", "int", NULL, "min=0", NULL },
	{ "leaf_item_max", "int", NULL, "min=0", NULL },
	{ "leaf_key_max", "int", NULL, "min=0", NULL },
	{ "leaf_page_max", "int", NULL, "min=512B,max=512MB", NULL },
	{ "leaf_value_max", "int", NULL, "min=0", NULL },
	{ "lsm", "category", NULL, NULL, confchk_lsm_subconfigs },
	{ "memory_page_max", "int", NULL, "min=512B,max=10TB", NULL },
	{ "os_cache_dirty_max", "int", NULL, "min=0", NULL },
	{ "os_cache_max", "int", NULL, "min=0", NULL },
	{ "prefix_compression", "boolean", NULL, NULL, NULL },
	{ "prefix_compression_min", "int", NULL, "min=0", NULL },
	{ "source", "string", NULL, NULL, NULL },
	{ "split_deepen_min_child", "int", NULL, NULL, NULL },
	{ "split_deepen_per_child", "int", NULL, NULL, NULL },
	{ "split_pct", "int", NULL, "min=25,max=100", NULL },
	{ "type", "string", NULL, NULL, NULL },
	{ "value_format", "format", __wt_struct_confchk, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_drop[] = {
	{ "force", "boolean", NULL, NULL, NULL },
	{ "remove_files", "boolean", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_open_cursor[] = {
	{ "append", "boolean", NULL, NULL, NULL },
	{ "bulk", "string", NULL, NULL, NULL },
	{ "checkpoint", "string", NULL, NULL, NULL },
	{ "dump", "string",
	    NULL, "choices=[\"hex\",\"json\",\"print\"]",
	    NULL },
	{ "next_random", "boolean", NULL, NULL, NULL },
	{ "overwrite", "boolean", NULL, NULL, NULL },
	{ "raw", "boolean", NULL, NULL, NULL },
	{ "readonly", "boolean", NULL, NULL, NULL },
	{ "skip_sort_check", "boolean", NULL, NULL, NULL },
	{ "statistics", "list",
	    NULL, "choices=[\"all\",\"fast\",\"clear\",\"size\"]",
	    NULL },
	{ "target", "list", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_reconfigure[] = {
	{ "isolation", "string",
	    NULL, "choices=[\"read-uncommitted\",\"read-committed\","
	    "\"snapshot\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_salvage[] = {
	{ "force", "boolean", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_session_verify[] = {
	{ "dump_address", "boolean", NULL, NULL, NULL },
	{ "dump_blocks", "boolean", NULL, NULL, NULL },
	{ "dump_offsets", "list", NULL, NULL, NULL },
	{ "dump_pages", "boolean", NULL, NULL, NULL },
	{ "dump_shape", "boolean", NULL, NULL, NULL },
	{ "strict", "boolean", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_table_meta[] = {
	{ "app_metadata", "string", NULL, NULL, NULL },
	{ "colgroups", "list", NULL, NULL, NULL },
	{ "collator", "string", __wt_collator_confchk, NULL, NULL },
	{ "columns", "list", NULL, NULL, NULL },
	{ "key_format", "format", __wt_struct_confchk, NULL, NULL },
	{ "value_format", "format", __wt_struct_confchk, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_log_subconfigs[] = {
	{ "archive", "boolean", NULL, NULL, NULL },
	{ "compressor", "string", NULL, NULL, NULL },
	{ "enabled", "boolean", NULL, NULL, NULL },
	{ "file_max", "int", NULL, "min=100KB,max=2GB", NULL },
	{ "path", "string", NULL, NULL, NULL },
	{ "prealloc", "boolean", NULL, NULL, NULL },
	{ "recover", "string",
	    NULL, "choices=[\"error\",\"on\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_transaction_sync_subconfigs[] = {
	{ "enabled", "boolean", NULL, NULL, NULL },
	{ "method", "string",
	    NULL, "choices=[\"dsync\",\"fsync\",\"none\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_wiredtiger_open[] = {
	{ "async", "category", NULL, NULL, confchk_async_subconfigs },
	{ "buffer_alignment", "int", NULL, "min=-1,max=1MB", NULL },
	{ "cache_overhead", "int", NULL, "min=0,max=30", NULL },
	{ "cache_size", "int", NULL, "min=1MB,max=10TB", NULL },
	{ "checkpoint", "category",
	    NULL, NULL,
	    confchk_checkpoint_subconfigs },
	{ "checkpoint_sync", "boolean", NULL, NULL, NULL },
	{ "config_base", "boolean", NULL, NULL, NULL },
	{ "create", "boolean", NULL, NULL, NULL },
	{ "direct_io", "list",
	    NULL, "choices=[\"checkpoint\",\"data\",\"log\"]",
	    NULL },
	{ "error_prefix", "string", NULL, NULL, NULL },
	{ "eviction", "category",
	    NULL, NULL,
	    confchk_eviction_subconfigs },
	{ "eviction_dirty_target", "int",
	    NULL, "min=10,max=99",
	    NULL },
	{ "eviction_target", "int", NULL, "min=10,max=99", NULL },
	{ "eviction_trigger", "int", NULL, "min=10,max=99", NULL },
	{ "exclusive", "boolean", NULL, NULL, NULL },
	{ "extensions", "list", NULL, NULL, NULL },
	{ "file_extend", "list",
	    NULL, "choices=[\"data\",\"log\"]",
	    NULL },
	{ "file_manager", "category",
	    NULL, NULL,
	    confchk_file_manager_subconfigs },
	{ "hazard_max", "int", NULL, "min=15", NULL },
	{ "log", "category", NULL, NULL, confchk_log_subconfigs },
	{ "lsm_manager", "category",
	    NULL, NULL,
	    confchk_lsm_manager_subconfigs },
	{ "lsm_merge", "boolean", NULL, NULL, NULL },
	{ "mmap", "boolean", NULL, NULL, NULL },
	{ "multiprocess", "boolean", NULL, NULL, NULL },
	{ "session_max", "int", NULL, "min=1", NULL },
	{ "session_scratch_max", "int", NULL, NULL, NULL },
	{ "shared_cache", "category",
	    NULL, NULL,
	    confchk_shared_cache_subconfigs },
	{ "statistics", "list",
	    NULL, "choices=[\"all\",\"fast\",\"none\",\"clear\"]",
	    NULL },
	{ "statistics_log", "category",
	    NULL, NULL,
	    confchk_statistics_log_subconfigs },
	{ "transaction_sync", "category",
	    NULL, NULL,
	    confchk_transaction_sync_subconfigs },
	{ "use_environment_priv", "boolean", NULL, NULL, NULL },
	{ "verbose", "list",
	    NULL, "choices=[\"api\",\"block\",\"checkpoint\",\"compact\","
	    "\"evict\",\"evictserver\",\"fileops\",\"log\",\"lsm\","
	    "\"metadata\",\"mutex\",\"overflow\",\"read\",\"reconcile\","
	    "\"recovery\",\"salvage\",\"shared_cache\",\"split\","
	    "\"temporary\",\"transaction\",\"verify\",\"version\",\"write\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_wiredtiger_open_all[] = {
	{ "async", "category", NULL, NULL, confchk_async_subconfigs },
	{ "buffer_alignment", "int", NULL, "min=-1,max=1MB", NULL },
	{ "cache_overhead", "int", NULL, "min=0,max=30", NULL },
	{ "cache_size", "int", NULL, "min=1MB,max=10TB", NULL },
	{ "checkpoint", "category",
	    NULL, NULL,
	    confchk_checkpoint_subconfigs },
	{ "checkpoint_sync", "boolean", NULL, NULL, NULL },
	{ "config_base", "boolean", NULL, NULL, NULL },
	{ "create", "boolean", NULL, NULL, NULL },
	{ "direct_io", "list",
	    NULL, "choices=[\"checkpoint\",\"data\",\"log\"]",
	    NULL },
	{ "error_prefix", "string", NULL, NULL, NULL },
	{ "eviction", "category",
	    NULL, NULL,
	    confchk_eviction_subconfigs },
	{ "eviction_dirty_target", "int",
	    NULL, "min=10,max=99",
	    NULL },
	{ "eviction_target", "int", NULL, "min=10,max=99", NULL },
	{ "eviction_trigger", "int", NULL, "min=10,max=99", NULL },
	{ "exclusive", "boolean", NULL, NULL, NULL },
	{ "extensions", "list", NULL, NULL, NULL },
	{ "file_extend", "list",
	    NULL, "choices=[\"data\",\"log\"]",
	    NULL },
	{ "file_manager", "category",
	    NULL, NULL,
	    confchk_file_manager_subconfigs },
	{ "hazard_max", "int", NULL, "min=15", NULL },
	{ "log", "category", NULL, NULL, confchk_log_subconfigs },
	{ "lsm_manager", "category",
	    NULL, NULL,
	    confchk_lsm_manager_subconfigs },
	{ "lsm_merge", "boolean", NULL, NULL, NULL },
	{ "mmap", "boolean", NULL, NULL, NULL },
	{ "multiprocess", "boolean", NULL, NULL, NULL },
	{ "session_max", "int", NULL, "min=1", NULL },
	{ "session_scratch_max", "int", NULL, NULL, NULL },
	{ "shared_cache", "category",
	    NULL, NULL,
	    confchk_shared_cache_subconfigs },
	{ "statistics", "list",
	    NULL, "choices=[\"all\",\"fast\",\"none\",\"clear\"]",
	    NULL },
	{ "statistics_log", "category",
	    NULL, NULL,
	    confchk_statistics_log_subconfigs },
	{ "transaction_sync", "category",
	    NULL, NULL,
	    confchk_transaction_sync_subconfigs },
	{ "use_environment_priv", "boolean", NULL, NULL, NULL },
	{ "verbose", "list",
	    NULL, "choices=[\"api\",\"block\",\"checkpoint\",\"compact\","
	    "\"evict\",\"evictserver\",\"fileops\",\"log\",\"lsm\","
	    "\"metadata\",\"mutex\",\"overflow\",\"read\",\"reconcile\","
	    "\"recovery\",\"salvage\",\"shared_cache\",\"split\","
	    "\"temporary\",\"transaction\",\"verify\",\"version\",\"write\"]",
	    NULL },
	{ "version", "string", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_wiredtiger_open_basecfg[] = {
	{ "async", "category", NULL, NULL, confchk_async_subconfigs },
	{ "buffer_alignment", "int", NULL, "min=-1,max=1MB", NULL },
	{ "cache_overhead", "int", NULL, "min=0,max=30", NULL },
	{ "cache_size", "int", NULL, "min=1MB,max=10TB", NULL },
	{ "checkpoint", "category",
	    NULL, NULL,
	    confchk_checkpoint_subconfigs },
	{ "checkpoint_sync", "boolean", NULL, NULL, NULL },
	{ "direct_io", "list",
	    NULL, "choices=[\"checkpoint\",\"data\",\"log\"]",
	    NULL },
	{ "error_prefix", "string", NULL, NULL, NULL },
	{ "eviction", "category",
	    NULL, NULL,
	    confchk_eviction_subconfigs },
	{ "eviction_dirty_target", "int",
	    NULL, "min=10,max=99",
	    NULL },
	{ "eviction_target", "int", NULL, "min=10,max=99", NULL },
	{ "eviction_trigger", "int", NULL, "min=10,max=99", NULL },
	{ "extensions", "list", NULL, NULL, NULL },
	{ "file_extend", "list",
	    NULL, "choices=[\"data\",\"log\"]",
	    NULL },
	{ "file_manager", "category",
	    NULL, NULL,
	    confchk_file_manager_subconfigs },
	{ "hazard_max", "int", NULL, "min=15", NULL },
	{ "log", "category", NULL, NULL, confchk_log_subconfigs },
	{ "lsm_manager", "category",
	    NULL, NULL,
	    confchk_lsm_manager_subconfigs },
	{ "lsm_merge", "boolean", NULL, NULL, NULL },
	{ "mmap", "boolean", NULL, NULL, NULL },
	{ "multiprocess", "boolean", NULL, NULL, NULL },
	{ "session_max", "int", NULL, "min=1", NULL },
	{ "session_scratch_max", "int", NULL, NULL, NULL },
	{ "shared_cache", "category",
	    NULL, NULL,
	    confchk_shared_cache_subconfigs },
	{ "statistics", "list",
	    NULL, "choices=[\"all\",\"fast\",\"none\",\"clear\"]",
	    NULL },
	{ "statistics_log", "category",
	    NULL, NULL,
	    confchk_statistics_log_subconfigs },
	{ "transaction_sync", "category",
	    NULL, NULL,
	    confchk_transaction_sync_subconfigs },
	{ "verbose", "list",
	    NULL, "choices=[\"api\",\"block\",\"checkpoint\",\"compact\","
	    "\"evict\",\"evictserver\",\"fileops\",\"log\",\"lsm\","
	    "\"metadata\",\"mutex\",\"overflow\",\"read\",\"reconcile\","
	    "\"recovery\",\"salvage\",\"shared_cache\",\"split\","
	    "\"temporary\",\"transaction\",\"verify\",\"version\",\"write\"]",
	    NULL },
	{ "version", "string", NULL, NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_CHECK confchk_wiredtiger_open_usercfg[] = {
	{ "async", "category", NULL, NULL, confchk_async_subconfigs },
	{ "buffer_alignment", "int", NULL, "min=-1,max=1MB", NULL },
	{ "cache_overhead", "int", NULL, "min=0,max=30", NULL },
	{ "cache_size", "int", NULL, "min=1MB,max=10TB", NULL },
	{ "checkpoint", "category",
	    NULL, NULL,
	    confchk_checkpoint_subconfigs },
	{ "checkpoint_sync", "boolean", NULL, NULL, NULL },
	{ "direct_io", "list",
	    NULL, "choices=[\"checkpoint\",\"data\",\"log\"]",
	    NULL },
	{ "error_prefix", "string", NULL, NULL, NULL },
	{ "eviction", "category",
	    NULL, NULL,
	    confchk_eviction_subconfigs },
	{ "eviction_dirty_target", "int",
	    NULL, "min=10,max=99",
	    NULL },
	{ "eviction_target", "int", NULL, "min=10,max=99", NULL },
	{ "eviction_trigger", "int", NULL, "min=10,max=99", NULL },
	{ "extensions", "list", NULL, NULL, NULL },
	{ "file_extend", "list",
	    NULL, "choices=[\"data\",\"log\"]",
	    NULL },
	{ "file_manager", "category",
	    NULL, NULL,
	    confchk_file_manager_subconfigs },
	{ "hazard_max", "int", NULL, "min=15", NULL },
	{ "log", "category", NULL, NULL, confchk_log_subconfigs },
	{ "lsm_manager", "category",
	    NULL, NULL,
	    confchk_lsm_manager_subconfigs },
	{ "lsm_merge", "boolean", NULL, NULL, NULL },
	{ "mmap", "boolean", NULL, NULL, NULL },
	{ "multiprocess", "boolean", NULL, NULL, NULL },
	{ "session_max", "int", NULL, "min=1", NULL },
	{ "session_scratch_max", "int", NULL, NULL, NULL },
	{ "shared_cache", "category",
	    NULL, NULL,
	    confchk_shared_cache_subconfigs },
	{ "statistics", "list",
	    NULL, "choices=[\"all\",\"fast\",\"none\",\"clear\"]",
	    NULL },
	{ "statistics_log", "category",
	    NULL, NULL,
	    confchk_statistics_log_subconfigs },
	{ "transaction_sync", "category",
	    NULL, NULL,
	    confchk_transaction_sync_subconfigs },
	{ "verbose", "list",
	    NULL, "choices=[\"api\",\"block\",\"checkpoint\",\"compact\","
	    "\"evict\",\"evictserver\",\"fileops\",\"log\",\"lsm\","
	    "\"metadata\",\"mutex\",\"overflow\",\"read\",\"reconcile\","
	    "\"recovery\",\"salvage\",\"shared_cache\",\"split\","
	    "\"temporary\",\"transaction\",\"verify\",\"version\",\"write\"]",
	    NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static const WT_CONFIG_ENTRY config_entries[] = {
	{ "colgroup.meta",
	  "app_metadata=,collator=,columns=,source=,type=file",
	  confchk_colgroup_meta
	},
	{ "connection.add_collator",
	  "",
	  NULL
	},
	{ "connection.add_compressor",
	  "",
	  NULL
	},
	{ "connection.add_data_source",
	  "",
	  NULL
	},
	{ "connection.add_extractor",
	  "",
	  NULL
	},
	{ "connection.async_new_op",
	  "append=0,overwrite=,raw=0,timeout=1200",
	  confchk_connection_async_new_op
	},
	{ "connection.close",
	  "leak_memory=0",
	  confchk_connection_close
	},
	{ "connection.load_extension",
	  "config=,entry=wiredtiger_extension_init,"
	  "terminate=wiredtiger_extension_terminate",
	  confchk_connection_load_extension
	},
	{ "connection.open_session",
	  "isolation=read-committed",
	  confchk_connection_open_session
	},
	{ "connection.reconfigure",
	  "async=(enabled=0,ops_max=1024,threads=2),cache_overhead=8,"
	  "cache_size=100MB,checkpoint=(log_size=0,"
	  "name=\"WiredTigerCheckpoint\",wait=0),error_prefix=,"
	  "eviction=(threads_max=1,threads_min=1),eviction_dirty_target=80,"
	  "eviction_target=80,eviction_trigger=95,"
	  "file_manager=(close_handle_minimum=250,close_idle_time=30,"
	  "close_scan_interval=10),lsm_manager=(merge=,worker_thread_max=4)"
	  ",lsm_merge=,shared_cache=(chunk=10MB,name=,reserve=0,size=500MB)"
	  ",statistics=none,statistics_log=(on_close=0,"
	  "path=\"WiredTigerStat.%d.%H\",sources=,"
	  "timestamp=\"%b %d %H:%M:%S\",wait=0),verbose=",
	  confchk_connection_reconfigure
	},
	{ "cursor.close",
	  "",
	  NULL
	},
	{ "cursor.reconfigure",
	  "append=0,overwrite=",
	  confchk_cursor_reconfigure
	},
	{ "file.meta",
	  "allocation_size=4KB,app_metadata=,block_allocation=best,"
	  "block_compressor=,cache_resident=0,checkpoint=,checkpoint_lsn=,"
	  "checksum=uncompressed,collator=,columns=,dictionary=0,"
	  "format=btree,huffman_key=,huffman_value=,id=,internal_item_max=0"
	  ",internal_key_max=0,internal_key_truncate=,internal_page_max=4KB"
	  ",key_format=u,key_gap=10,leaf_item_max=0,leaf_key_max=0,"
	  "leaf_page_max=32KB,leaf_value_max=0,memory_page_max=5MB,"
	  "os_cache_dirty_max=0,os_cache_max=0,prefix_compression=0,"
	  "prefix_compression_min=4,split_deepen_min_child=0,"
	  "split_deepen_per_child=0,split_pct=75,value_format=u,"
	  "version=(major=0,minor=0)",
	  confchk_file_meta
	},
	{ "index.meta",
	  "app_metadata=,collator=,columns=,extractor=,immutable=0,"
	  "index_key_columns=,key_format=u,source=,type=file,value_format=u",
	  confchk_index_meta
	},
	{ "session.begin_transaction",
	  "isolation=,name=,priority=0,sync=",
	  confchk_session_begin_transaction
	},
	{ "session.checkpoint",
	  "drop=,force=0,name=,target=",
	  confchk_session_checkpoint
	},
	{ "session.close",
	  "",
	  NULL
	},
	{ "session.commit_transaction",
	  "",
	  NULL
	},
	{ "session.compact",
	  "timeout=1200",
	  confchk_session_compact
	},
	{ "session.create",
	  "allocation_size=4KB,app_metadata=,block_allocation=best,"
	  "block_compressor=,cache_resident=0,checksum=uncompressed,"
	  "colgroups=,collator=,columns=,dictionary=0,exclusive=0,"
	  "extractor=,format=btree,huffman_key=,huffman_value=,immutable=0,"
	  "internal_item_max=0,internal_key_max=0,internal_key_truncate=,"
	  "internal_page_max=4KB,key_format=u,key_gap=10,leaf_item_max=0,"
	  "leaf_key_max=0,leaf_page_max=32KB,leaf_value_max=0,"
	  "lsm=(auto_throttle=,bloom=,bloom_bit_count=16,bloom_config=,"
	  "bloom_hash_count=8,bloom_oldest=0,chunk_count_limit=0,"
	  "chunk_max=5GB,chunk_size=10MB,merge_max=15,merge_min=0),"
	  "memory_page_max=5MB,os_cache_dirty_max=0,os_cache_max=0,"
	  "prefix_compression=0,prefix_compression_min=4,source=,"
	  "split_deepen_min_child=0,split_deepen_per_child=0,split_pct=75,"
	  "type=file,value_format=u",
	  confchk_session_create
	},
	{ "session.drop",
	  "force=0,remove_files=",
	  confchk_session_drop
	},
	{ "session.log_printf",
	  "",
	  NULL
	},
	{ "session.open_cursor",
	  "append=0,bulk=0,checkpoint=,dump=,next_random=0,overwrite=,raw=0"
	  ",readonly=0,skip_sort_check=0,statistics=,target=",
	  confchk_session_open_cursor
	},
	{ "session.reconfigure",
	  "isolation=read-committed",
	  confchk_session_reconfigure
	},
	{ "session.rename",
	  "",
	  NULL
	},
	{ "session.rollback_transaction",
	  "",
	  NULL
	},
	{ "session.salvage",
	  "force=0",
	  confchk_session_salvage
	},
	{ "session.strerror",
	  "",
	  NULL
	},
	{ "session.truncate",
	  "",
	  NULL
	},
	{ "session.upgrade",
	  "",
	  NULL
	},
	{ "session.verify",
	  "dump_address=0,dump_blocks=0,dump_offsets=,dump_pages=0,"
	  "dump_shape=0,strict=0",
	  confchk_session_verify
	},
	{ "table.meta",
	  "app_metadata=,colgroups=,collator=,columns=,key_format=u,"
	  "value_format=u",
	  confchk_table_meta
	},
	{ "wiredtiger_open",
	  "async=(enabled=0,ops_max=1024,threads=2),buffer_alignment=-1,"
	  "cache_overhead=8,cache_size=100MB,checkpoint=(log_size=0,"
	  "name=\"WiredTigerCheckpoint\",wait=0),checkpoint_sync=,"
	  "config_base=,create=0,direct_io=,error_prefix=,"
	  "eviction=(threads_max=1,threads_min=1),eviction_dirty_target=80,"
	  "eviction_target=80,eviction_trigger=95,exclusive=0,extensions=,"
	  "file_extend=,file_manager=(close_handle_minimum=250,"
	  "close_idle_time=30,close_scan_interval=10),hazard_max=1000,"
	  "log=(archive=,compressor=,enabled=0,file_max=100MB,path=,"
	  "prealloc=,recover=on),lsm_manager=(merge=,worker_thread_max=4),"
	  "lsm_merge=,mmap=,multiprocess=0,session_max=100,"
	  "session_scratch_max=2MB,shared_cache=(chunk=10MB,name=,reserve=0"
	  ",size=500MB),statistics=none,statistics_log=(on_close=0,"
	  "path=\"WiredTigerStat.%d.%H\",sources=,"
	  "timestamp=\"%b %d %H:%M:%S\",wait=0),transaction_sync=(enabled=0"
	  ",method=fsync),use_environment_priv=0,verbose=",
	  confchk_wiredtiger_open
	},
	{ "wiredtiger_open_all",
	  "async=(enabled=0,ops_max=1024,threads=2),buffer_alignment=-1,"
	  "cache_overhead=8,cache_size=100MB,checkpoint=(log_size=0,"
	  "name=\"WiredTigerCheckpoint\",wait=0),checkpoint_sync=,"
	  "config_base=,create=0,direct_io=,error_prefix=,"
	  "eviction=(threads_max=1,threads_min=1),eviction_dirty_target=80,"
	  "eviction_target=80,eviction_trigger=95,exclusive=0,extensions=,"
	  "file_extend=,file_manager=(close_handle_minimum=250,"
	  "close_idle_time=30,close_scan_interval=10),hazard_max=1000,"
	  "log=(archive=,compressor=,enabled=0,file_max=100MB,path=,"
	  "prealloc=,recover=on),lsm_manager=(merge=,worker_thread_max=4),"
	  "lsm_merge=,mmap=,multiprocess=0,session_max=100,"
	  "session_scratch_max=2MB,shared_cache=(chunk=10MB,name=,reserve=0"
	  ",size=500MB),statistics=none,statistics_log=(on_close=0,"
	  "path=\"WiredTigerStat.%d.%H\",sources=,"
	  "timestamp=\"%b %d %H:%M:%S\",wait=0),transaction_sync=(enabled=0"
	  ",method=fsync),use_environment_priv=0,verbose=,version=(major=0,"
	  "minor=0)",
	  confchk_wiredtiger_open_all
	},
	{ "wiredtiger_open_basecfg",
	  "async=(enabled=0,ops_max=1024,threads=2),buffer_alignment=-1,"
	  "cache_overhead=8,cache_size=100MB,checkpoint=(log_size=0,"
	  "name=\"WiredTigerCheckpoint\",wait=0),checkpoint_sync=,"
	  "direct_io=,error_prefix=,eviction=(threads_max=1,threads_min=1),"
	  "eviction_dirty_target=80,eviction_target=80,eviction_trigger=95,"
	  "extensions=,file_extend=,file_manager=(close_handle_minimum=250,"
	  "close_idle_time=30,close_scan_interval=10),hazard_max=1000,"
	  "log=(archive=,compressor=,enabled=0,file_max=100MB,path=,"
	  "prealloc=,recover=on),lsm_manager=(merge=,worker_thread_max=4),"
	  "lsm_merge=,mmap=,multiprocess=0,session_max=100,"
	  "session_scratch_max=2MB,shared_cache=(chunk=10MB,name=,reserve=0"
	  ",size=500MB),statistics=none,statistics_log=(on_close=0,"
	  "path=\"WiredTigerStat.%d.%H\",sources=,"
	  "timestamp=\"%b %d %H:%M:%S\",wait=0),transaction_sync=(enabled=0"
	  ",method=fsync),verbose=,version=(major=0,minor=0)",
	  confchk_wiredtiger_open_basecfg
	},
	{ "wiredtiger_open_usercfg",
	  "async=(enabled=0,ops_max=1024,threads=2),buffer_alignment=-1,"
	  "cache_overhead=8,cache_size=100MB,checkpoint=(log_size=0,"
	  "name=\"WiredTigerCheckpoint\",wait=0),checkpoint_sync=,"
	  "direct_io=,error_prefix=,eviction=(threads_max=1,threads_min=1),"
	  "eviction_dirty_target=80,eviction_target=80,eviction_trigger=95,"
	  "extensions=,file_extend=,file_manager=(close_handle_minimum=250,"
	  "close_idle_time=30,close_scan_interval=10),hazard_max=1000,"
	  "log=(archive=,compressor=,enabled=0,file_max=100MB,path=,"
	  "prealloc=,recover=on),lsm_manager=(merge=,worker_thread_max=4),"
	  "lsm_merge=,mmap=,multiprocess=0,session_max=100,"
	  "session_scratch_max=2MB,shared_cache=(chunk=10MB,name=,reserve=0"
	  ",size=500MB),statistics=none,statistics_log=(on_close=0,"
	  "path=\"WiredTigerStat.%d.%H\",sources=,"
	  "timestamp=\"%b %d %H:%M:%S\",wait=0),transaction_sync=(enabled=0"
	  ",method=fsync),verbose=",
	  confchk_wiredtiger_open_usercfg
	},
	{ NULL, NULL, NULL }
};

int
__wt_conn_config_init(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;
	const WT_CONFIG_ENTRY *ep, **epp;

	conn = S2C(session);

	/* Build a list of pointers to the configuration information. */
	WT_RET(__wt_calloc_def(session,
	    sizeof(config_entries) / sizeof(config_entries[0]), &epp));
	conn->config_entries = epp;

	/* Fill in the list to reference the default information. */
	for (ep = config_entries;;) {
		*epp++ = ep++;
		if (ep->method == NULL)
			break;
	}
	return (0);
}

void
__wt_conn_config_discard(WT_SESSION_IMPL *session)
{
	WT_CONNECTION_IMPL *conn;

	conn = S2C(session);

	__wt_free(session, conn->config_entries);
}
