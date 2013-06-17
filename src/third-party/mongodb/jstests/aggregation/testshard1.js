/*
> ShardingTest
function (testName, numShards, verboseLevel, numMongos, otherParams) {
*/
var shardedAggTest = new ShardingTest({
    shards: 2,
    verbose: 1,
    mongos: 1,
    other: { chunksize: 1}
    }
);

shardedAggTest.adminCommand( { enablesharding : "aggShard" } );
db = shardedAggTest.getDB( "aggShard" );

/* make sure its cleaned up */
db.ts1.drop();

shardedAggTest.adminCommand( { shardcollection : "aggShard.ts1", key : { "_id" : 1 } } );


/*
Test combining results in mongos for operations that sub-aggregate on shards.

The unusual operators here are $avg, $pushToSet, $push.   In the case of $avg,
the shard pipeline produces an object with the current subtotal and item count
so that these can be combined in mongos by totalling the subtotals counts
before performing the final division.  For $pushToSet and $push, the shard
pipelines produce arrays, but in mongos these are combined rather than simply
being added as arrays within arrays.
*/

var count = 0;
var strings = [
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
    "eleven",
    "twelve",
    "thirteen",
    "fourteen",
    "fifteen",
    "sixteen",
    "seventeen",
    "eighteen",
    "nineteen",
    "twenty"
];

var nItems = 200000;
for(i = 1; i <= nItems; ++i) {
    db.ts1.save(
        {counter: ++count, number: strings[i % 20], random: Math.random(),
         filler: "0123456789012345678901234567890123456789"});
}

// wait for all writebacks to be applied
assert.eq(db.getLastError(), null);

// a project and group in shards, result combined in mongos
var a1 = db.ts1.aggregate(
    { $project: {
        cMod10: {$mod:["$counter", 10]},
        number: 1,
        counter: 1
    }},
    { $group: {
        _id: "$cMod10",
        numberSet: {$addToSet: "$number"},
        avgCounter: {$avg: "$cMod10"}
    }},
    { $sort: {_id:1} }
);

var a1result = a1.result;
for(i = 0 ; i < 10; ++i) {
    assert.eq(a1result[i].avgCounter, a1result[i]._id,
           'agg sharded test avgCounter failed');
    assert.eq(a1result[i].numberSet.length, 2,
           'agg sharded test numberSet length failed');
}

// an initial group starts the group in the shards, and combines them in mongos
var a2 = db.ts1.aggregate(
    { $group: {
        _id: "all",
        total: {$sum: "$counter"}
    }}
);

// sum of an arithmetic progression S(n) = (n/2)(a(1) + a(n));
assert.eq(a2.result[0].total, (nItems/2)*(1 + nItems),
       'agg sharded test counter sum failed');

// an initial group starts the group in the shards, and combines them in mongos
var a3 = db.ts1.aggregate(
    { $group: {
        _id: "$number",
        total: {$sum: 1}
    }},
    { $sort: {_id:1} }
);

var a3result = a3.result;
for(i = 0 ; i < strings.length; ++i) {
    assert.eq(a3result[i].total, nItems/strings.length,
           'agg sharded test sum numbers failed');
}

// a match takes place in the shards; just returning the results from mongos
var a4 = db.ts1.aggregate(
    { $match: {$or:[{counter:55}, {counter:1111},
                    {counter: 2222}, {counter: 33333},
                    {counter: 99999}, {counter: 55555}]}
    }
);

var a4result = a4.result;
for(i = 0; i < 6; ++i) {
    c = a4result[i].counter;
    printjson({c:c})
    assert((c == 55) || (c == 1111) || (c == 2222) ||
           (c == 33333) || (c = 99999) || (c == 55555),
           'agg sharded test simple match failed');
}

function testSkipLimit(ops, expectedCount) {
    if (expectedCount > 10) {
        // make shard -> mongos intermediate results less than 16MB
        ops.unshift({$project: {_id:1}})
    }

    ops.push({$group: {_id:1, count: {$sum: 1}}});

    var out = db.ts1.aggregate(ops);
    assert.commandWorked(out);
    assert.eq(out.result[0].count, expectedCount);
}

testSkipLimit([], nItems); // control
testSkipLimit([{$skip:10}], nItems - 10);
testSkipLimit([{$limit:10}], 10);
testSkipLimit([{$skip:5}, {$limit:10}], 10);
testSkipLimit([{$limit:10}, {$skip:5}], 10 - 5);
testSkipLimit([{$skip:5}, {$skip: 3}, {$limit:10}], 10);
testSkipLimit([{$skip:5}, {$limit:10}, {$skip: 3}], 10 - 3);
testSkipLimit([{$limit:10}, {$skip:5}, {$skip: 3}], 10 - 3 - 5);

// test sort + limit (using random to pull from both shards)
function testSortLimit(limit, direction) {
    var from_cursor = db.ts1.find({},{random:1, _id:0})
                            .sort({random: direction})
                            .limit(limit)
                            .toArray();
    var from_agg = db.ts1.aggregate({$project: {random:1, _id:0}}
                                   ,{$sort: {random: direction}}
                                   ,{$limit: limit}
                                   ).result;
    assert.eq(from_cursor, from_agg);
}
testSortLimit(1,  1);
testSortLimit(1, -1);
testSortLimit(10,  1);
testSortLimit(10, -1);
testSortLimit(100,  1);
testSortLimit(100, -1);


// shut everything down
shardedAggTest.stop();
