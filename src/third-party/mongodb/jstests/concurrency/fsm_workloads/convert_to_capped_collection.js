/**
 * convert_to_capped_collection.js
 *
 * Creates a non-capped collection. Converts it to a
 * capped collection. After each iteration, truncates the
 * collection, ensuring that the storage size of the
 * collection is still a multiple of 256.
 *
 * MongoDB raises the storage size of a capped collection
 * to an integer multiple of 256.
 */
load('jstests/concurrency/fsm_workload_helpers/drop_utils.js');

var $config = (function() {
    var iter = 20;
    var data = {
        prefix: 'convert_to_capped_collection',

        // initial size should not be a power of 256
        size: Math.pow(2, iter + 5) + 1
    };

    var states = (function() {

        function uniqueCollectionName(prefix, tid) {
            return prefix + '_' + tid;
        }

        function isMultiple256(num) {
            return num % 256 === 0;
        }

        function init(db, collName) {
            this.threadCollName = uniqueCollectionName(this.prefix, this.tid);

            var bulk = db[this.threadCollName].initializeUnorderedBulkOp();
            for (var i = 0; i < (this.tid + 1) * 200; i++) {
                bulk.insert({ i: i, rand: Random.rand() });
            }

            var res = bulk.execute();
            assertAlways.writeOK(res);
            assertAlways.eq((this.tid + 1) * 200, res.nInserted);

            assertWhenOwnDB(!db[this.threadCollName].isCapped());
            assertWhenOwnDB.commandWorked(db[this.threadCollName].convertToCapped(this.size));
            assertWhenOwnDB(db[this.threadCollName].isCapped());
            assertWhenOwnDB(isMultiple256(db[this.threadCollName].storageSize()));
        }

        function convertToCapped(db, collName) {
            // divide size by 1.5 so that the resulting size
            // is not a multiple of 256
            this.size /= 1.5;

            assertWhenOwnDB.commandWorked(db[this.threadCollName].convertToCapped(this.size));
            assertWhenOwnDB(db[this.threadCollName].isCapped());
            assertWhenOwnDB(isMultiple256(db[this.threadCollName].storageSize()));

            // only the _id index should remain after running convertToCapped
            var indexKeys = db[this.threadCollName].getIndexKeys();
            assertWhenOwnDB.eq(1, indexKeys.length);
            assertWhenOwnDB(function() {
                assertWhenOwnDB.docEq({ _id: 1 }, indexKeys[0]);
            });
        }

        return {
            init: init,
            convertToCapped: convertToCapped
        };
    })();

    var transitions = {
        init: { convertToCapped: 1 },
        convertToCapped: { convertToCapped: 1 }
    };

    function teardown(db, collName, cluster) {
        var pattern = new RegExp('^' + this.prefix + '_\\d+$');
        dropCollections(db, pattern);
    }

    return {
        threadCount: 10,
        iterations: iter,
        data: data,
        states: states,
        transitions: transitions,
        teardown: teardown
    };

})();
