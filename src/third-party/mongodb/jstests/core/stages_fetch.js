// Test basic fetch functionality.
t = db.stages_fetch;
t.drop();
var collname = "stages_fetch";

var N = 50;
for (var i = 0; i < N; ++i) {
    t.insert({foo: i, bar: N - i, baz: i});
}

t.ensureIndex({foo: 1});

// 20 <= foo <= 30
// bar == 25 (not covered, should error.)
ixscan1 = {ixscan: {args:{keyPattern:{foo:1},
                          startKey: {"": 20},
                          endKey: {"" : 30}, endKeyInclusive: true,
                          direction: 1},
                    filter: {bar: 25}}};
res = db.runCommand({stageDebug: {collection: collname, plan: ixscan1}});
assert.eq(res.ok, 0);

// Now, add a fetch.  We should be able to filter on the non-covered field since we fetched the obj.
ixscan2 = {ixscan: {args:{keyPattern:{foo:1},
                          startKey: {"": 20},
                          endKey: {"" : 30}, endKeyInclusive: true,
                          direction: 1}}}
fetch = {fetch: {args: {node: ixscan2}, filter: {bar: 25}}}
res = db.runCommand({stageDebug: {collection: collname, plan: fetch}});
printjson(res);
assert.eq(res.ok, 1);
assert.eq(res.results.length, 1);
