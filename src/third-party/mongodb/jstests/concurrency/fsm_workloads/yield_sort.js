'use strict';

/*
 * yield_sort.js (extends yield_sort_merge.js)
 *
 * Intersperse queries which use the SORT stage with updates and deletes of documents they may
 * match.
 * This workload is blacklisted until SERVER-17011 is resolved. Updates during sort stage can
 * cause docs to be returned out of order if it's an unindexed query with non-default batch sizes.
 */
load('jstests/concurrency/fsm_libs/extend_workload.js'); // for extendWorkload
load('jstests/concurrency/fsm_workloads/yield_sort_merge.js'); // for $config

var $config = extendWorkload($config, function($config, $super) {

    /*
     * Execute a query that will use the SORT stage.
     */
    $config.states.query = function sort(db, collName) {
        var nMatches = 100;
        // Sort on c, since it's not an indexed field.
        var cursor = db[collName].find({ a: { $lt: nMatches } })
                                 .sort({ c: -1 })
                                 .batchSize(this.batchSize);

        var verifier = function sortVerifier(doc, prevDoc) {
            var correctOrder = true;
            if (prevDoc !== null) {
                correctOrder = (doc._id <= prevDoc._id);
            }
            return doc.a < nMatches && correctOrder;
        };

        this.advanceCursor(cursor, verifier);
    };

    $config.data.genUpdateDoc = function genUpdateDoc() {
        var newA = Random.randInt($config.data.nDocs);
        var newC = Random.randInt($config.data.nDocs);
        return { $set: { a: newA, c: newC } };
    };

    return $config;
});
