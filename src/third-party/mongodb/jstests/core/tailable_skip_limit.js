// Test that tailable cursors work correctly with skip and limit.

// Setup the capped collection.
var collname = "jstests_tailable_skip_limit"
var t = db[collname];
t.drop();
db.createCollection(collname, {capped: true, size: 1024});

t.save({_id: 1});
t.save({_id: 2});

// Non-tailable with skip
var cursor = t.find().skip(1);
assert.eq(2, cursor.next()["_id"]);
assert(!cursor.hasNext());
t.save({_id: 3});
assert(!cursor.hasNext());

// Non-tailable with limit
var cursor = t.find().limit(100);
for (var i = 1; i <= 3; i++) {
    assert.eq(i, cursor.next()["_id"]);
}
assert(!cursor.hasNext());
t.save({_id: 4});
assert(!cursor.hasNext());

// Non-tailable with negative limit
var cursor = t.find().limit(-100);
for (var i = 1; i <= 4; i++) {
    assert.eq(i, cursor.next()["_id"]);
}
assert(!cursor.hasNext());
t.save({_id: 5});
assert(!cursor.hasNext());

// Tailable with skip
cursor = t.find().addOption(2).skip(4);
assert.eq(5, cursor.next()["_id"]);
assert(!cursor.hasNext());
t.save({_id: 6});
assert(cursor.hasNext());
assert.eq(6, cursor.next()["_id"]);

// Tailable with limit
var cursor = t.find().addOption(2).limit(100);
for (var i = 1; i <= 6; i++) {
    assert.eq(i, cursor.next()["_id"]);
}
assert(!cursor.hasNext());
t.save({_id: 7});
assert(cursor.hasNext());
assert.eq(7, cursor.next()["_id"]);

// Tailable with negative limit
var cursor = t.find().addOption(2).limit(-100);
for (var i = 1; i <= 7; i++) {
    assert.eq(i, cursor.next()["_id"]);
}
assert(!cursor.hasNext());
t.save({_id: 8});
assert(cursor.hasNext());
assert.eq(8, cursor.next()["_id"]);
