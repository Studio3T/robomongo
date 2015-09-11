# This file is a python script that describes the WiredTiger API.

class Method:
    def __init__(self, config, **flags):
        self.config = config
        self.flags = flags

class Config:
    def __init__(self, name, default, desc, subconfig=None, **flags):
        self.name = name
        self.default = default
        self.desc = desc
        self.subconfig = subconfig
        self.flags = flags

    def __cmp__(self, other):
        return cmp(self.name, other.name)

# Metadata shared by all schema objects
common_meta = [
    Config('app_metadata', '', r'''
        application-owned metadata for this object'''),
    Config('collator', 'none', r'''
        configure custom collation for keys.  Permitted values are
        \c "none" or a custom collator name created with
        WT_CONNECTION::add_collator''',
        func='__wt_collator_confchk'),
    Config('columns', '', r'''
        list of the column names.  Comma-separated list of the form
        <code>(column[,...])</code>.  For tables, the number of entries
        must match the total number of values in \c key_format and \c
        value_format.  For colgroups and indices, all column names must
        appear in the list of columns for the table''',
        type='list'),
]

source_meta = [
    Config('source', '', r'''
        set a custom data source URI for a column group, index or simple
        table.  By default, the data source URI is derived from the \c
        type and the column group or index name.  Applications can
        create tables from existing data sources by supplying a \c
        source configuration''', undoc=True),
    Config('type', 'file', r'''
        set the type of data source used to store a column group, index
        or simple table.  By default, a \c "file:" URI is derived from
        the object name.  The \c type configuration can be used to
        switch to a different data source, such as LSM or an extension
        configured by the application'''),
]

format_meta = common_meta + [
    Config('key_format', 'u', r'''
        the format of the data packed into key items.  See @ref
        schema_format_types for details.  By default, the key_format is
        \c 'u' and applications use WT_ITEM structures to manipulate
        raw byte arrays. By default, records are stored in row-store
        files: keys of type \c 'r' are record numbers and records
        referenced by record number are stored in column-store files''',
        type='format', func='__wt_struct_confchk'),
    Config('value_format', 'u', r'''
        the format of the data packed into value items.  See @ref
        schema_format_types for details.  By default, the value_format
        is \c 'u' and applications use a WT_ITEM structure to
        manipulate raw byte arrays. Value items of type 't' are
        bitfields, and when configured with record number type keys,
        will be stored using a fixed-length store''',
        type='format', func='__wt_struct_confchk'),
]

lsm_config = [
    Config('lsm', '', r'''
        options only relevant for LSM data sources''',
        type='category', subconfig=[
        Config('auto_throttle', 'true', r'''
            Throttle inserts into LSM trees if flushing to disk isn't
            keeping up''',
            type='boolean'),
        Config('bloom', 'true', r'''
            create bloom filters on LSM tree chunks as they are merged''',
            type='boolean'),
        Config('bloom_config', '', r'''
            config string used when creating Bloom filter files, passed
            to WT_SESSION::create'''),
        Config('bloom_bit_count', '16', r'''
            the number of bits used per item for LSM bloom filters''',
            min='2', max='1000'),
        Config('bloom_hash_count', '8', r'''
            the number of hash values per item used for LSM bloom
            filters''',
            min='2', max='100'),
        Config('bloom_oldest', 'false', r'''
            create a bloom filter on the oldest LSM tree chunk. Only
            supported if bloom filters are enabled''',
            type='boolean'),
        Config('chunk_count_limit', '0', r'''
            the maximum number of chunks to allow in an LSM tree. This
            option automatically times out old data. As new chunks are
            added old chunks will be removed. Enabling this option
            disables LSM background merges''',
            type='int'),
        Config('chunk_max', '5GB', r'''
            the maximum size a single chunk can be. Chunks larger than this
            size are not considered for further merges. This is a soft
            limit, and chunks larger than this value can be created.  Must
            be larger than chunk_size''',
            min='100MB', max='10TB'),
        Config('chunk_size', '10MB', r'''
            the maximum size of the in-memory chunk of an LSM tree.  This
            limit is soft - it is possible for chunks to be temporarily
            larger than this value.  This overrides the \c memory_page_max
            setting''',
            min='512K', max='500MB'),
        Config('merge_max', '15', r'''
            the maximum number of chunks to include in a merge operation''',
            min='2', max='100'),
        Config('merge_min', '0', r'''
            the minimum number of chunks to include in a merge operation. If
            set to 0 or 1 half the value of merge_max is used''',
            max='100'),
    ]),
]

# Per-file configuration
file_config = format_meta + [
    Config('block_allocation', 'best', r'''
        configure block allocation. Permitted values are \c "first" or
        \c "best"; the \c "first" configuration uses a first-available
        algorithm during block allocation, the \c "best" configuration
        uses a best-fit algorithm''',
        choices=['first', 'best',]),
    Config('allocation_size', '4KB', r'''
        the file unit allocation size, in bytes, must a power-of-two;
        smaller values decrease the file space required by overflow
        items, and the default value of 4KB is a good choice absent
        requirements from the operating system or storage device''',
        min='512B', max='128MB'),
    Config('block_compressor', 'none', r'''
        configure a compressor for file blocks.  Permitted values are \c "none"
        or custom compression engine name created with
        WT_CONNECTION::add_compressor.  If WiredTiger has builtin support for
        \c "bzip2", \c "snappy", \c "lz4" or \c "zlib" compression, these names
        are also available.  See @ref compression for more information''',
        func='__wt_compressor_confchk'),
    Config('cache_resident', 'false', r'''
        do not ever evict the object's pages; see @ref
        tuning_cache_resident for more information''',
        type='boolean'),
    Config('checksum', 'uncompressed', r'''
        configure block checksums; permitted values are <code>on</code>
        (checksum all blocks), <code>off</code> (checksum no blocks) and
        <code>uncompresssed</code> (checksum only blocks which are not
        compressed for any reason).  The \c uncompressed setting is for
        applications which can rely on decompression to fail if a block
        has been corrupted''',
        choices=['on', 'off', 'uncompressed']),
    Config('dictionary', '0', r'''
        the maximum number of unique values remembered in the Btree
        row-store leaf page value dictionary; see
        @ref file_formats_compression for more information''',
        min='0'),
    Config('format', 'btree', r'''
        the file format''',
        choices=['btree']),
    Config('huffman_key', 'none', r'''
        configure Huffman encoding for keys.  Permitted values are
        \c "none", \c "english", \c "utf8<file>" or \c "utf16<file>".
        See @ref huffman for more information''',
        func='__wt_huffman_confchk'),
    Config('huffman_value', 'none', r'''
        configure Huffman encoding for values.  Permitted values are
        \c "none", \c "english", \c "utf8<file>" or \c "utf16<file>".
        See @ref huffman for more information''',
        func='__wt_huffman_confchk'),
    Config('internal_key_truncate', 'true', r'''
        configure internal key truncation, discarding unnecessary
        trailing bytes on internal keys (ignored for custom
        collators)''',
        type='boolean'),
    Config('internal_page_max', '4KB', r'''
        the maximum page size for internal nodes, in bytes; the size
        must be a multiple of the allocation size and is significant
        for applications wanting to avoid excessive L2 cache misses
        while searching the tree.  The page maximum is the bytes of
        uncompressed data, that is, the limit is applied before any
        block compression is done''',
        min='512B', max='512MB'),
    Config('internal_item_max', '0', r'''
        historic term for internal_key_max''',
        min=0, undoc=True),
    Config('internal_key_max', '0', r'''
        the largest key stored in an internal node, in bytes.  If set, keys
        larger than the specified size are stored as overflow items (which
        may require additional I/O to access).  The default and the maximum
        allowed value are both one-tenth the size of a newly split internal
        page''',
        min='0'),
    Config('key_gap', '10', r'''
        the maximum gap between instantiated keys in a Btree leaf page,
        constraining the number of keys processed to instantiate a
        random Btree leaf page key''',
        min='0', undoc=True),
    Config('leaf_key_max', '0', r'''
        the largest key stored in a leaf node, in bytes.  If set, keys
        larger than the specified size are stored as overflow items (which
        may require additional I/O to access).  The default value is
        one-tenth the size of a newly split leaf page''',
        min='0'),
    Config('leaf_page_max', '32KB', r'''
        the maximum page size for leaf nodes, in bytes; the size must
        be a multiple of the allocation size, and is significant for
        applications wanting to maximize sequential data transfer from
        a storage device.  The page maximum is the bytes of uncompressed
        data, that is, the limit is applied before any block compression
        is done''',
        min='512B', max='512MB'),
    Config('leaf_value_max', '0', r'''
        the largest value stored in a leaf node, in bytes.  If set, values
        larger than the specified size are stored as overflow items (which
        may require additional I/O to access). If the size is larger than
        the maximum leaf page size, the page size is temporarily ignored
        when large values are written. The default is one-half the size of
        a newly split leaf page''',
        min='0'),
    Config('leaf_item_max', '0', r'''
        historic term for leaf_key_max and leaf_value_max''',
        min=0, undoc=True),
    Config('memory_page_max', '5MB', r'''
        the maximum size a page can grow to in memory before being
        reconciled to disk.  The specified size will be adjusted to a lower
        bound of <code>50 * leaf_page_max</code>, and an upper bound of
        <code>cache_size / 2</code>.  This limit is soft - it is possible
        for pages to be temporarily larger than this value.  This setting
        is ignored for LSM trees, see \c chunk_size''',
        min='512B', max='10TB'),
    Config('os_cache_max', '0', r'''
        maximum system buffer cache usage, in bytes.  If non-zero, evict
        object blocks from the system buffer cache after that many bytes
        from this object are read or written into the buffer cache''',
        min=0),
    Config('os_cache_dirty_max', '0', r'''
        maximum dirty system buffer cache usage, in bytes.  If non-zero,
        schedule writes for dirty blocks belonging to this object in the
        system buffer cache after that many bytes from this object are
        written into the buffer cache''',
        min=0),
    Config('prefix_compression', 'false', r'''
        configure prefix compression on row-store leaf pages''',
        type='boolean'),
    Config('prefix_compression_min', '4', r'''
        minimum gain before prefix compression will be used on row-store
        leaf pages''',
        min=0),
    Config('split_deepen_min_child', '0', r'''
        minimum entries in a page to consider deepening the tree''',
        type='int', undoc=True),
    Config('split_deepen_per_child', '0', r'''
        entries allocated per child when deepening the tree''',
        type='int', undoc=True),
    Config('split_pct', '75', r'''
        the Btree page split size as a percentage of the maximum Btree
        page size, that is, when a Btree page is split, it will be
        split into smaller pages, where each page is the specified
        percentage of the maximum Btree page size''',
        min='25', max='100'),
]

# File metadata, including both configurable and non-configurable (internal)
file_meta = file_config + [
    Config('checkpoint', '', r'''
        the file checkpoint entries'''),
    Config('checkpoint_lsn', '', r'''
        LSN of the last checkpoint'''),
    Config('id', '', r'''
        the file's ID number'''),
    Config('version', '(major=0,minor=0)', r'''
        the file version'''),
]

table_only_config = [
    Config('colgroups', '', r'''
        comma-separated list of names of column groups.  Each column
        group is stored separately, keyed by the primary key of the
        table.  If no column groups are specified, all columns are
        stored together in a single file.  All value columns in the
        table must appear in at least one column group.  Each column
        group must be created with a separate call to
        WT_SESSION::create''', type='list'),
]

index_only_config = [
    Config('extractor', 'none', r'''
        configure custom extractor for indices.  Permitted values are
        \c "none" or an extractor name created with
        WT_CONNECTION::add_extractor''',
        func='__wt_extractor_confchk'),
    Config('immutable', 'false', r'''
        configure the index to be immutable - that is an index is not changed
        by any update to a record in the table''', type='boolean'),
]

colgroup_meta = common_meta + source_meta

index_meta = format_meta + source_meta + index_only_config + [
    Config('index_key_columns', '', r'''
        number of public key columns''', type='int', undoc=True),
]

table_meta = format_meta + table_only_config

# Connection runtime config, shared by conn.reconfigure and wiredtiger_open
connection_runtime_config = [
    Config('async', '', r'''
        asynchronous operations configuration options''',
        type='category', subconfig=[
        Config('enabled', 'false', r'''
            enable asynchronous operation''',
            type='boolean'),
        Config('ops_max', '1024', r'''
            maximum number of expected simultaneous asynchronous
                operations''', min='1', max='4096'),
        Config('threads', '2', r'''
            the number of worker threads to service asynchronous
                requests''',
                min='1', max='20'), # !!! Must match WT_ASYNC_MAX_WORKERS
            ]),
    Config('cache_size', '100MB', r'''
        maximum heap memory to allocate for the cache. A database should
        configure either \c cache_size or \c shared_cache but not both''',
        min='1MB', max='10TB'),
    Config('cache_overhead', '8', r'''
        assume the heap allocator overhead is the specified percentage, and
        adjust the cache usage by that amount (for example, if there is 10GB
        of data in cache, a percentage of 10 means WiredTiger treats this as
        11GB).  This value is configurable because different heap allocators
        have different overhead and different workloads will have different
        heap allocation sizes and patterns, therefore applications may need to
        adjust this value based on allocator choice and behavior in measured
        workloads''',
        min='0', max='30'),
    Config('checkpoint', '', r'''
        periodically checkpoint the database''',
        type='category', subconfig=[
        Config('name', '"WiredTigerCheckpoint"', r'''
            the checkpoint name'''),
        Config('log_size', '0', r'''
            wait for this amount of log record bytes to be written to
                the log between each checkpoint.  A database can configure
                both log_size and wait to set an upper bound for checkpoints;
                setting this value above 0 configures periodic checkpoints''',
            min='0', max='2GB'),
        Config('wait', '0', r'''
            seconds to wait between each checkpoint; setting this value
            above 0 configures periodic checkpoints''',
            min='0', max='100000'),
        ]),
    Config('error_prefix', '', r'''
        prefix string for error messages'''),
    Config('eviction_dirty_target', '80', r'''
        continue evicting until the cache has less dirty memory than the
        value, as a percentage of the total cache size. Dirty pages will
        only be evicted if the cache is full enough to trigger eviction''',
        min=10, max=99),
    Config('eviction_target', '80', r'''
        continue evicting until the cache has less total memory than the
        value, as a percentage of the total cache size. Must be less than
        \c eviction_trigger''',
        min=10, max=99),
    Config('eviction_trigger', '95', r'''
        trigger eviction when the cache is using this much memory, as a
        percentage of the total cache size''', min=10, max=99),
    Config('file_manager', '', r'''
        control how file handles are managed''',
        type='category', subconfig=[
        Config('close_idle_time', '30', r'''
            amount of time in seconds a file handle needs to be idle
            before attempting to close it. A setting of 0 means that idle
            handles are not closed''', min=0, max=100000),
        Config('close_handle_minimum', '250', r'''
            number of handles open before the file manager will look for handles
            to close'''),
        Config('close_scan_interval', '10', r'''
            interval in seconds at which to check for files that are
            inactive and close them''', min=1, max=100000),
        ]),
    Config('lsm_manager', '', r'''
        configure database wide options for LSM tree management''',
        type='category', subconfig=[
        Config('worker_thread_max', '4', r'''
            Configure a set of threads to manage merging LSM trees in
            the database.''',
            min='3',     # !!! Must match WT_LSM_MIN_WORKERS
            max='20'),     # !!! Must match WT_LSM_MAX_WORKERS
        Config('merge', 'true', r'''
            merge LSM chunks where possible''',
            type='boolean')
        ]),
    Config('lsm_merge', 'true', r'''
        merge LSM chunks where possible (deprecated)''',
        type='boolean', undoc=True),
    Config('eviction', '', r'''
        eviction configuration options.''',
        type='category', subconfig=[
            Config('threads_max', '1', r'''
                maximum number of threads WiredTiger will start to help evict
                pages from cache. The number of threads started will vary
                depending on the current eviction load''',
                min=1, max=20),
            Config('threads_min', '1', r'''
                minimum number of threads WiredTiger will start to help evict
                pages from cache. The number of threads currently running will
                vary depending on the current eviction load''',
                min=1, max=20),
            ]),
    Config('shared_cache', '', r'''
        shared cache configuration options. A database should configure
        either a cache_size or a shared_cache not both''',
        type='category', subconfig=[
        Config('chunk', '10MB', r'''
            the granularity that a shared cache is redistributed''',
            min='1MB', max='10TB'),
        Config('reserve', '0', r'''
            amount of cache this database is guaranteed to have
            available from the shared cache. This setting is per
            database. Defaults to the chunk size''', type='int'),
        Config('name', 'none', r'''
            the name of a cache that is shared between databases or
            \c "none" when no shared cache is configured'''),
        Config('size', '500MB', r'''
            maximum memory to allocate for the shared cache. Setting
            this will update the value if one is already set''',
            min='1MB', max='10TB')
        ]),
    Config('statistics', 'none', r'''
        Maintain database statistics, which may impact performance.
        Choosing "all" maintains all statistics regardless of cost,
        "fast" maintains a subset of statistics that are relatively
        inexpensive, "none" turns off all statistics.  The "clear"
        configuration resets statistics after they are gathered,
        where appropriate (for example, a cache size statistic is
        not cleared, while the count of cursor insert operations will
        be cleared).   When "clear" is configured for the database,
        gathered statistics are reset each time a statistics cursor
        is used to gather statistics, as well as each time statistics
        are logged using the \c statistics_log configuration.  See
        @ref statistics for more information''',
        type='list', choices=['all', 'fast', 'none', 'clear']),
    Config('statistics_log', '', r'''
        log any statistics the database is configured to maintain,
        to a file.  See @ref statistics for more information''',
        type='category', subconfig=[
        Config('on_close', 'false', r'''log statistics on database close''',
            type='boolean'),
        Config('path', '"WiredTigerStat.%d.%H"', r'''
            the pathname to a file into which the log records are written,
            may contain ISO C standard strftime conversion specifications.
            If the value is not an absolute path name, the file is created
            relative to the database home'''),
        Config('sources', '', r'''
            if non-empty, include statistics for the list of data source
            URIs, if they are open at the time of the statistics logging.
            The list may include URIs matching a single data source
            ("table:mytable"), or a URI matching all data sources of a
            particular type ("table:")''',
            type='list'),
        Config('timestamp', '"%b %d %H:%M:%S"', r'''
            a timestamp prepended to each log record, may contain strftime
            conversion specifications'''),
        Config('wait', '0', r'''
            seconds to wait between each write of the log records; setting
            this value above 0 configures statistics logging''',
            min='0', max='100000'),
        ]),
    Config('verbose', '', r'''
        enable messages for various events. Only available if WiredTiger
        is configured with --enable-verbose. Options are given as a
        list, such as <code>"verbose=[evictserver,read]"</code>''',
        type='list', choices=[
            'api',
            'block',
            'checkpoint',
            'compact',
            'evict',
            'evictserver',
            'fileops',
            'log',
            'lsm',
            'metadata',
            'mutex',
            'overflow',
            'read',
            'reconcile',
            'recovery',
            'salvage',
            'shared_cache',
            'split',
            'temporary',
            'transaction',
            'verify',
            'version',
            'write']),
]

session_config = [
    Config('isolation', 'read-committed', r'''
        the default isolation level for operations in this session''',
        choices=['read-uncommitted', 'read-committed', 'snapshot']),
]

common_wiredtiger_open = [
    Config('buffer_alignment', '-1', r'''
        in-memory alignment (in bytes) for buffers used for I/O.  The
        default value of -1 indicates a platform-specific alignment value
        should be used (4KB on Linux systems when direct I/O is configured,
        zero elsewhere)''',
        min='-1', max='1MB'),
    Config('checkpoint_sync', 'true', r'''
        flush files to stable storage when closing or writing
        checkpoints''',
        type='boolean'),
    Config('direct_io', '', r'''
        Use \c O_DIRECT to access files.  Options are given as a list,
        such as <code>"direct_io=[data]"</code>.  Configuring
        \c direct_io requires care, see @ref
        tuning_system_buffer_cache_direct_io for important warnings.
        Including \c "data" will cause WiredTiger data files to use
        \c O_DIRECT, including \c "log" will cause WiredTiger log files
        to use \c O_DIRECT, and including \c "checkpoint" will cause
        WiredTiger data files opened at a checkpoint (i.e: read only) to
        use \c O_DIRECT''',
        type='list', choices=['checkpoint', 'data', 'log']),
    Config('extensions', '', r'''
        list of shared library extensions to load (using dlopen).
        Any values specified to an library extension are passed to
        WT_CONNECTION::load_extension as the \c config parameter
        (for example,
        <code>extensions=(/path/ext.so={entry=my_entry})</code>)''',
        type='list'),
    Config('file_extend', '', r'''
        file extension configuration.  If set, extend files of the set
        type in allocations of the set size, instead of a block at a
        time as each new block is written.  For example,
        <code>file_extend=(data=16MB)</code>''',
        type='list', choices=['data', 'log']),
    Config('hazard_max', '1000', r'''
        maximum number of simultaneous hazard pointers per session
        handle''',
        min='15'),
    Config('log', '', r'''
        enable logging''',
        type='category', subconfig=[
        Config('archive', 'true', r'''
            automatically archive unneeded log files''',
            type='boolean'),
        Config('compressor', 'none', r'''
            configure a compressor for log records.  Permitted values are
            \c "none" or custom compression engine name created with
            WT_CONNECTION::add_compressor.  If WiredTiger has builtin support
            for \c "bzip2", \c "snappy", \c "lz4" or \c "zlib" compression,
            these names are also available. See @ref compression for more
            information'''),
        Config('enabled', 'false', r'''
            enable logging subsystem''',
            type='boolean'),
        Config('file_max', '100MB', r'''
            the maximum size of log files''',
            min='100KB', max='2GB'),
        Config('path', '', r'''
            the path to a directory into which the log files are written.
            If the value is not an absolute path name, the files are created
            relative to the database home'''),
        Config('prealloc', 'true', r'''
            pre-allocate log files.''',
            type='boolean'),
        Config('recover', 'on', r'''
            run recovery or error if recovery needs to run after an
            unclean shutdown.''',
            choices=['error','on']),
        ]),
    Config('mmap', 'true', r'''
        Use memory mapping to access files when possible''',
        type='boolean'),
    Config('multiprocess', 'false', r'''
        permit sharing between processes (will automatically start an
        RPC server for primary processes and use RPC for secondary
        processes). <b>Not yet supported in WiredTiger</b>''',
        type='boolean'),
    Config('session_max', '100', r'''
        maximum expected number of sessions (including server
        threads)''',
        min='1'),
    Config('session_scratch_max', '2MB', r'''
        maximum memory to cache in each session''',
        type='int', undoc=True),
    Config('transaction_sync', '', r'''
        how to sync log records when the transaction commits''',
        type='category', subconfig=[
        Config('enabled', 'false', r'''
            whether to sync the log on every commit by default, can be
            overridden by the \c sync setting to
            WT_SESSION::begin_transaction''',
            type='boolean'),
        Config('method', 'fsync', r'''
            the method used to ensure log records are stable on disk, see
            @ref tune_durability for more information''',
            choices=['dsync', 'fsync', 'none']),
        ]),
]

cursor_runtime_config = [
    Config('append', 'false', r'''
        append the value as a new record, creating a new record
        number key; valid only for cursors with record number keys''',
        type='boolean'),
    Config('overwrite', 'true', r'''
        configures whether the cursor's insert, update and remove
        methods check the existing state of the record.  If \c overwrite
        is \c false, WT_CURSOR::insert fails with ::WT_DUPLICATE_KEY
        if the record exists, WT_CURSOR::update and WT_CURSOR::remove
        fail with ::WT_NOTFOUND if the record does not exist''',
        type='boolean'),
]

methods = {
'file.meta' : Method(file_meta),

'colgroup.meta' : Method(colgroup_meta),

'index.meta' : Method(index_meta),

'table.meta' : Method(table_meta),

'cursor.close' : Method([]),

'cursor.reconfigure' : Method(cursor_runtime_config),

'session.close' : Method([]),

'session.compact' : Method([
    Config('timeout', '1200', r'''
        maximum amount of time to allow for compact in seconds. The
        actual amount of time spent in compact may exceed the configured
        value. A value of zero disables the timeout''',
        type='int'),
]),

'session.create' : Method(file_config + lsm_config + source_meta + 
        index_only_config + table_only_config + [
    Config('exclusive', 'false', r'''
        fail if the object exists.  When false (the default), if the
        object exists, check that its settings match the specified
        configuration''',
        type='boolean'),
]),

'session.drop' : Method([
    Config('force', 'false', r'''
        return success if the object does not exist''',
        type='boolean'),
    Config('remove_files', 'true', r'''
        should the underlying files be removed?''',
        type='boolean'),
]),

'session.log_printf' : Method([]),

'session.open_cursor' : Method(cursor_runtime_config + [
    Config('bulk', 'false', r'''
        configure the cursor for bulk-loading, a fast, initial load
        path (see @ref tune_bulk_load for more information).  Bulk-load
        may only be used for newly created objects and cursors
        configured for bulk-load only support the WT_CURSOR::insert
        and WT_CURSOR::close methods.  When bulk-loading row-store
        objects, keys must be loaded in sorted order.  The value is
        usually a true/false flag; when bulk-loading fixed-length
        column store objects, the special value \c bitmap allows
        chunks of a memory resident bitmap to be loaded directly into
        a file by passing a \c WT_ITEM to WT_CURSOR::set_value where
        the \c size field indicates the number of records in the
        bitmap (as specified by the object's \c value_format
        configuration). Bulk-loaded bitmap values must end on a byte
        boundary relative to the bit count (except for the last set
        of values loaded)'''),
    Config('checkpoint', '', r'''
        the name of a checkpoint to open (the reserved name
        "WiredTigerCheckpoint" opens the most recent internal
        checkpoint taken for the object).  The cursor does not
        support data modification'''),
    Config('dump', '', r'''
        configure the cursor for dump format inputs and outputs: "hex"
        selects a simple hexadecimal format, "json" selects a JSON format
        with each record formatted as fields named by column names if
        available, and "print" selects a format where only non-printing
        characters are hexadecimal encoded.  These formats are compatible
        with the @ref util_dump and @ref util_load commands''',
        choices=['hex', 'json', 'print']),
    Config('next_random', 'false', r'''
        configure the cursor to return a pseudo-random record from
        the object; valid only for row-store cursors.  Cursors
        configured with \c next_random=true only support the
        WT_CURSOR::next and WT_CURSOR::close methods.  See @ref
        cursor_random for details''',
        type='boolean'),
    Config('raw', 'false', r'''
        ignore the encodings for the key and value, manage data as if
        the formats were \c "u".  See @ref cursor_raw for details''',
        type='boolean'),
    Config('readonly', 'false', r'''
        only query operations are supported by this cursor. An error is
        returned if a modification is attempted using the cursor.  The
        default is false for all cursor types except for log and metadata
        cursors''',
        type='boolean'),
    Config('skip_sort_check', 'false', r'''
        skip the check of the sort order of each bulk-loaded key''',
        type='boolean', undoc=True),
    Config('statistics', '', r'''
        Specify the statistics to be gathered.  Choosing "all" gathers
        statistics regardless of cost and may include traversing on-disk files;
        "fast" gathers a subset of relatively inexpensive statistics.  The
        selection must agree with the database \c statistics configuration
        specified to ::wiredtiger_open or WT_CONNECTION::reconfigure.  For
        example, "all" or "fast" can be configured when the database is
        configured with "all", but the cursor open will fail if "all" is
        specified when the database is configured with "fast", and the cursor
        open will fail in all cases when the database is configured with
        "none".  If "size" is configured, only the underlying size of the
        object on disk is filled in and the object is not opened.  If \c
        statistics is not configured, the default configuration is the database
        configuration.  The "clear" configuration resets statistics after
        gathering them, where appropriate (for example, a cache size statistic
        is not cleared, while the count of cursor insert operations will be
        cleared).  See @ref statistics for more information''',
        type='list', choices=['all', 'fast', 'clear', 'size']),
    Config('target', '', r'''
        if non-empty, backup the list of objects; valid only for a
        backup data source''',
        type='list'),
]),

'session.rename' : Method([]),
'session.salvage' : Method([
    Config('force', 'false', r'''
        force salvage even of files that do not appear to be WiredTiger
        files''',
        type='boolean'),
]),
'session.strerror' : Method([]),
'session.truncate' : Method([]),
'session.upgrade' : Method([]),
'session.verify' : Method([
    Config('dump_address', 'false', r'''
        Display addresses and page types as pages are verified,
        using the application's message handler, intended for debugging''',
        type='boolean'),
    Config('dump_blocks', 'false', r'''
        Display the contents of on-disk blocks as they are verified,
        using the application's message handler, intended for debugging''',
        type='boolean'),
    Config('dump_offsets', '', r'''
        Display the contents of specific on-disk blocks,
        using the application's message handler, intended for debugging''',
        type='list'),
    Config('dump_pages', 'false', r'''
        Display the contents of in-memory pages as they are verified,
        using the application's message handler, intended for debugging''',
        type='boolean'),
    Config('dump_shape', 'false', r'''
        Display the shape of the tree after verification,
        using the application's message handler, intended for debugging''',
        type='boolean'),
    Config('strict', 'false', r'''
        Treat any verification problem as an error; by default, verify will
        warn, but not fail, in the case of errors that won't affect future
        behavior (for example, a leaked block)''',
        type='boolean')
]),

'session.begin_transaction' : Method([
    Config('isolation', '', r'''
        the isolation level for this transaction; defaults to the
        session's isolation level''',
        choices=['read-uncommitted', 'read-committed', 'snapshot']),
    Config('name', '', r'''
        name of the transaction for tracing and debugging'''),
    Config('priority', 0, r'''
        priority of the transaction for resolving conflicts.
        Transactions with higher values are less likely to abort''',
        min='-100', max='100'),
    Config('sync', '', r'''
        whether to sync log records when the transaction commits,
        inherited from ::wiredtiger_open \c transaction_sync''',
        type='boolean'),
]),

'session.commit_transaction' : Method([]),
'session.rollback_transaction' : Method([]),

'session.checkpoint' : Method([
    Config('drop', '', r'''
        specify a list of checkpoints to drop.
        The list may additionally contain one of the following keys:
        \c "from=all" to drop all checkpoints,
        \c "from=<checkpoint>" to drop all checkpoints after and
        including the named checkpoint, or
        \c "to=<checkpoint>" to drop all checkpoints before and
        including the named checkpoint.  Checkpoints cannot be
        dropped while a hot backup is in progress or if open in
        a cursor''', type='list'),
    Config('force', 'false', r'''
        by default, checkpoints may be skipped if the underlying object
        has not been modified, this option forces the checkpoint''',
        type='boolean'),
    Config('name', '', r'''
        if set, specify a name for the checkpoint (note that checkpoints
        including LSM trees may not be named)'''),
    Config('target', '', r'''
        if non-empty, checkpoint the list of objects''', type='list'),
]),

'connection.add_collator' : Method([]),
'connection.add_compressor' : Method([]),
'connection.add_data_source' : Method([]),
'connection.add_extractor' : Method([]),
'connection.async_new_op' : Method([
    Config('append', 'false', r'''
        append the value as a new record, creating a new record
        number key; valid only for operations with record number keys''',
        type='boolean'),
    Config('overwrite', 'true', r'''
        configures whether the cursor's insert, update and remove
        methods check the existing state of the record.  If \c overwrite
        is \c false, WT_CURSOR::insert fails with ::WT_DUPLICATE_KEY
        if the record exists, WT_CURSOR::update and WT_CURSOR::remove
        fail with ::WT_NOTFOUND if the record does not exist''',
        type='boolean'),
    Config('raw', 'false', r'''
        ignore the encodings for the key and value, manage data as if
        the formats were \c "u".  See @ref cursor_raw for details''',
        type='boolean'),
    Config('timeout', '1200', r'''
        maximum amount of time to allow for compact in seconds. The
        actual amount of time spent in compact may exceed the configured
        value. A value of zero disables the timeout''',
        type='int'),
]),
'connection.close' : Method([
    Config('leak_memory', 'false', r'''
        don't free memory during close''',
        type='boolean'),
]),
'connection.reconfigure' : Method(connection_runtime_config),

'connection.load_extension' : Method([
    Config('config', '', r'''
        configuration string passed to the entry point of the
        extension as its WT_CONFIG_ARG argument'''),
    Config('entry', 'wiredtiger_extension_init', r'''
        the entry point of the extension, called to initialize the
        extension when it is loaded.  The signature of the function
        must match ::wiredtiger_extension_init'''),
    Config('terminate', 'wiredtiger_extension_terminate', r'''
        an optional function in the extension that is called before
        the extension is unloaded during WT_CONNECTION::close.  The
        signature of the function must match
        ::wiredtiger_extension_terminate'''),
]),

'connection.open_session' : Method(session_config),

'session.reconfigure' : Method(session_config),

# There are 4 variants of the wiredtiger_open configurations.
# wiredtiger_open:
#    Configuration values allowed in the application's configuration
#    argument to the wiredtiger_open call.
# wiredtiger_open_basecfg:
#    Configuration values allowed in the WiredTiger.basecfg file (remove
# creation-specific configuration strings and add a version string).
# wiredtiger_open_usercfg:
#    Configuration values allowed in the WiredTiger.config file (remove
# creation-specific configuration strings).
# wiredtiger_open_all:
#    All of the above configuration values combined
'wiredtiger_open' : Method(
    connection_runtime_config +
    common_wiredtiger_open + [
    Config('config_base', 'true', r'''
        write the base configuration file if creating the database,
        see @ref config_base for more information''',
        type='boolean'),
    Config('create', 'false', r'''
        create the database if it does not exist''',
        type='boolean'),
    Config('exclusive', 'false', r'''
        fail if the database already exists, generally used with the
        \c create option''',
        type='boolean'),
    Config('use_environment_priv', 'false', r'''
        use the \c WIREDTIGER_CONFIG and \c WIREDTIGER_HOME environment
        variables regardless of whether or not the process is running
        with special privileges.  See @ref home for more information''',
        type='boolean'),
]),
'wiredtiger_open_basecfg' : Method(
    connection_runtime_config +
    common_wiredtiger_open + [
    Config('version', '(major=0,minor=0)', r'''
        the file version'''),
]),
'wiredtiger_open_usercfg' : Method(
    connection_runtime_config +
    common_wiredtiger_open
),
'wiredtiger_open_all' : Method(
    connection_runtime_config +
    common_wiredtiger_open + [
    Config('config_base', 'true', r'''
        write the base configuration file if creating the database,
        see @ref config_base for more information''',
        type='boolean'),
    Config('create', 'false', r'''
        create the database if it does not exist''',
        type='boolean'),
    Config('exclusive', 'false', r'''
        fail if the database already exists, generally used with the
        \c create option''',
        type='boolean'),
    Config('use_environment_priv', 'false', r'''
        use the \c WIREDTIGER_CONFIG and \c WIREDTIGER_HOME environment
        variables regardless of whether or not the process is running
        with special privileges.  See @ref home for more information''',
        type='boolean'),
    Config('version', '(major=0,minor=0)', r'''
        the file version'''),
]),
}
