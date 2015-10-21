'use strict';

/**
 * findAndModify_update_queue.js
 *
 * A large number of documents are inserted during the workload setup. Each thread repeated updates
 * a document from the collection using the findAndModify command, and stores the _id field of that
 * document in another database. At the end of the workload, the contents of the other database are
 * checked for whether one thread updated the same document as another thread.
 *
 * This workload was designed to reproduce an issue similar to SERVER-18304 for update operations
 * using the findAndModify command where the old version of the document is returned.
 */
load('jstests/concurrency/fsm_libs/extend_workload.js'); // for extendWorkload
load('jstests/concurrency/fsm_workloads/findAndModify_remove_queue.js'); // for $config

var $config = extendWorkload($config, function($config, $super) {

    // Use the workload name as the database name, since the workload name is assumed to be unique.
    $config.data.uniqueDBName = 'findAndModify_update_queue';

    $config.data.newDocForInsert = function newDocForInsert(i) {
        return { _id: i, rand: Random.rand(), counter: 0 };
    };

    $config.data.getIndexSpec = function getIndexSpec() {
        return { counter: 1, rand: -1 };
    };

    $config.data.opName = 'updated';

    var states = (function() {

        function update(db, collName) {
            // Update the counter field to avoid matching the same document again.
            var res = db.runCommand({
                findAndModify: db[collName].getName(),
                query: { counter: 0 },
                sort: { rand: -1 },
                update: { $inc: { counter: 1 } },
                new: false
            });
            assertAlways.commandWorked(res);

            var doc = res.value;
            if (doc !== null) {
                this.saveDocId.call(this, db, collName, doc._id);
            }
        }

        return {
            update: update
        };

    })();

    var transitions = {
        update: { update: 1 }
    };

    $config.startState = 'update';
    $config.states = states;
    $config.transitions = transitions;

    return $config;
});
