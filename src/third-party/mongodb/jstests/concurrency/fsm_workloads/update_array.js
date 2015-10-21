'use strict';

/**
 * update_array.js
 *
 * Each thread does a $push or $pull on a random doc, pushing or pulling its
 * thread id. After each push or pull, the thread does a .findOne() to verify
 * that its thread id is present or absent (respectively). This is correct even
 * though other threads in the workload may be modifying the array between the
 * update and the find, because thread ids are unique.
 */
load('jstests/concurrency/fsm_workload_helpers/server_types.js'); // for isMongod and isMMAPv1

var $config = (function() {

    var states = (function() {

        // db: explicitly passed to avoid accidentally using the global `db`
        // res: WriteResult
        // nModifiedPossibilities: array of allowed values for res.nModified
        function assertUpdateSuccess(db, res, nModifiedPossibilities) {
            assertAlways.eq(0, res.nUpserted, tojson(res));

            if (isMongod(db) && !isMMAPv1(db)) {
                assertWhenOwnColl.contains(1, nModifiedPossibilities, tojson(res));
                if (db.getMongo().writeMode() === 'commands') {
                    assertWhenOwnColl.contains(res.nModified, nModifiedPossibilities, tojson(res));
                }
            }
            else {
                // Zero matches are possible for MMAP v1 because the update will skip a document
                // that was invalidated during a yield.
                assertWhenOwnColl.contains(res.nMatched, [0, 1], tojson(res));
                if (db.getMongo().writeMode() === 'commands') {
                    assertWhenOwnColl.contains(res.nModified, [0, 1], tojson(res));
                }
            }
        }

        function doPush(db, collName, docIndex, value) {
            var res = db[collName].update({ _id: docIndex }, { $push: { arr: value } });

            // assert the update reported success
            assertUpdateSuccess(db, res, [1]);

            // find the doc and make sure it was updated
            var doc = db[collName].findOne({ _id: docIndex });
            assertWhenOwnColl(function() {
                assertWhenOwnColl.neq(null, doc);
                assertWhenOwnColl(doc.hasOwnProperty('arr'),
                                  'doc should have contained a field named "arr": ' + tojson(doc));
                assertWhenOwnColl.contains(value, doc.arr,
                                           "doc.arr doesn't contain value (" + value +
                                           ') after $push: ' + tojson(doc.arr));
            });
        }

        function doPull(db, collName, docIndex, value) {
            var res = db[collName].update({ _id: docIndex }, { $pull: { arr: value } });

            // assert the update reported success
            assertUpdateSuccess(db, res, [0, 1]);

            // find the doc and make sure it was updated
            var doc = db[collName].findOne({ _id: docIndex });
            assertWhenOwnColl(function() {
                assertWhenOwnColl.neq(null, doc);
                assertWhenOwnColl.eq(-1, doc.arr.indexOf(value),
                                     'doc.arr contains removed value (' + value +
                                     ') after $pull: ' + tojson(doc.arr));
            });
        }

        return {
            push: function push(db, collName) {
                var docIndex = Random.randInt(this.numDocs);
                var value = this.tid;

                doPush(db, collName, docIndex, value);
            },

            pull: function pull(db, collName) {
                var docIndex = Random.randInt(this.numDocs);
                var value = this.tid;

                doPull(db, collName, docIndex, value);
            }
        };

    })();

    var transitions = {
        push: {
            push: 0.8,
            pull: 0.2
        },
        pull: {
            push: 0.8,
            pull: 0.2
        }
    };

    function setup(db, collName, cluster) {
        // index on 'arr', the field being updated
        assertAlways.commandWorked(db[collName].ensureIndex({ arr: 1 }));
        for (var i = 0; i < this.numDocs; ++i) {
            var res = db[collName].insert({ _id: i, arr: [] });
            assertWhenOwnColl.writeOK(res);
            assertWhenOwnColl.eq(1, res.nInserted);
        }
    }

    return {
        threadCount: 5,
        iterations: 10,
        startState: 'push',
        states: states,
        transitions: transitions,
        data: {
            numDocs: 10
        },
        setup: setup
    };

})();
