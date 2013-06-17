// Top level match fields within an $elemMatch clause may constrain multiple subfields from a
// compound multikey index.  SERVER-3104
//
// Given a multikey index { 'a.b':1, 'a.c':1 } and query { 'a.b':3, 'a.c':3 } only the index field
// 'a.b' is constrained to the range [3, 3], while the index field 'a.c' is just constrained
// to be within minkey and maxkey.  This implementation ensures that the document
// { a:[ { b:3 }, { c:3 } ] }, which generates index keys { 'a.b':3, 'a.c':null } and
// { 'a.b':null and 'a.c':3 } will be retrieved for the query.  (See SERVER-958 for more
// information.)
//
// If the query is instead { a:{ $elemMatch:{ b:3, c:3 } } } then the document
// { a:[ { b:3 }, { c:3 } ] } does not match.  Until SERVER-3104 was implemented, the index
// constraints would be [3,3] on the 'a.b' field and [minkey,maxkey] on the 'a.c' field, the same as
// for the non $elemMatch query in the previous paragraph.  With the SERVER-3104 implementation,
// constraints on two fields within a $elemMatch parent can both be applied to an index.  Due to the
// SERVER-3104 implementation, the index constraints become [3,3] on the 'a.b' field _and_ [3,3] on
// the 'a.c' field.

t = db.jstests_index13;
t.drop();

function assertConsistentResults( query ) {
    assert.eq( t.find( query ).hint( { $natural:1 } ).sort( { _id:1 } ).toArray(),
               t.find( query ).hint( index ).sort( { _id:1 } ).toArray() );
}

function assertResultsAndIndexBounds( expectedBounds, query ) {
    explain = t.find( query ).hint( index ).explain();
    // printjson( explain ); // debug
    assert.eq( expectedBounds, explain.indexBounds );
    assertConsistentResults( query );
}

MIN = { $minElement:1 };
MAX = { $maxElement:1 };


// Cases with single dotted index fied names.
index = { 'a.b':1, 'a.c':1 };
t.ensureIndex( index );
t.save( { a:[ { b:1 }, { c:1 } ] } );
t.save( { a:[ { b:1, c:1 } ] } );
assert.eq( 2, t.count() );
// Without $elemMatch.
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ MIN, MAX ]] },
                             { 'a.b':1, 'a.c':1 } );
// With $elemMatch.
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ 1, 1 ]] },
                             { a:{ $elemMatch:{ b:1, c:1 } } } );
// Without shared $elemMatch.
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ MIN, MAX ]] },
                             { 'a.b':1, a:{ $elemMatch:{ c:1 } } } );
// Two different $elemMatch expressions.
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ MIN, MAX ]] },
                             { $and:[ { a:{ $elemMatch:{ b:1 } } },
                                      { a:{ $elemMatch:{ c:1 } } } ] } );


// Cases relating to parse order and inclusion of intersected ranges.
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ MIN, MAX ]] },
                             { 'a.b':1, a:{ $elemMatch:{ b:{ $gt:0 }, c:1 } } } );
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ MIN, MAX ]] },
                             { 'a.b':1, a:{ $elemMatch:{ b:{ $gt:0 }, c:1 } } } );
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ MIN, MAX ]] },
                             { a:{ $elemMatch:{ b:1, c:1 } }, 'a.b':1 } );
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ 1, 1 ]] },
                             { 'a.b':1, a:{ $elemMatch:{ b:1, c:1 } } } );
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ 1, 1 ]] },
                             { 'a.c':1, a:{ $elemMatch:{ b:1, c:1 } } } );
assertResultsAndIndexBounds( { 'a.b':[[ 1, 1 ]], 'a.c':[[ 1, 1 ]] },
                             { a:{ $elemMatch:{ b:1, c:1 } }, 'a.b':{ $gt:0 } } );


// Cases with double dotted index field names.
t.drop();
index = { 'a.b.x':1, 'a.b.y':1 };
t.ensureIndex( index );
t.save( { a:{ b:{ x:1, y:1 } } } );
t.save( { a:[ { b:{ x:1 } }, { b:{ y:1 } } ] } );
t.save( { a:[ { b:[ { x:1 }, { y:1 } ] } ] } );
t.save( { a:[ { b:[ { x:1, y:1 } ] } ] } );
assert.eq( 4, t.count() );
// No $elemMatch.
assertResultsAndIndexBounds( { 'a.b.x':[[ 1, 1 ]], 'a.b.y':[[ MIN, MAX ]] },
                             { 'a.b.x':1, 'a.b.y':1 } );
// $elemMatch with dotted children.
assertResultsAndIndexBounds( { 'a.b.x':[[ 1, 1 ]], 'a.b.y':[[ MIN, MAX ]] },
                             { a:{ $elemMatch:{ 'b.x':1, 'b.y':1 } } } );
// $elemMatch with undotted children.
assertResultsAndIndexBounds( { 'a.b.x':[[ 1, 1 ]], 'a.b.y':[[ 1, 1 ]] },
                             { 'a.b':{ $elemMatch:{ x:1, y:1 } } } );

// Cases where a field is indexed along with its children.
t.dropIndexes();
index = { 'a':1, 'a.b.x':1, 'a.b.y':1 };
t.ensureIndex( index );
// Constraint on a prior parent field.
assertResultsAndIndexBounds( { a:[[ MIN, 4 ], [ 4, MAX ]], 'a.b.x':[[ MIN, MAX ]],
                               'a.b.y':[[ MIN, MAX ]] },
                             { a:{ $ne:4 }, 'a.b':{ $elemMatch:{ x:1, y:1 } } } );
// No constraint on a prior parent field.
assertResultsAndIndexBounds( { a:[[ MIN, MAX ]], 'a.b.x':[[ 1, 1 ]], 'a.b.y':[[ 1, 1 ]] },
                             { 'a.b':{ $elemMatch:{ x:1, y:1 } } } );


// Cases with double dotted index field names branching to different fields at each dot.
t.drop();
index = { 'a.b.c':1, 'a.e.f':1, 'a.b.d':1, 'a.e.g':1 }
t.ensureIndex( index );
t.save( { a:{ b:{ c:1, d:1 }, e:{ f:1, g:1 } } } );
t.save( { a:[ { b:{ c:1 }, e:{ f:1 } }, { b:{ d:1 }, e:{ g:1 } } ] } );
t.save( { a:[ { b:{ c:1 } }, { e:{ f:1 } }, { b:{ d:1 } }, { e:{ g:1 } } ] } );
t.save( { a:[ { b:[ { c:1 }, { d:1 } ] }, { e:[ { f:1 }, { g:1 } ] } ] } );
t.save( { a:[ { b:[ { c:[ 1 ] }, { d:[ 1 ] } ] }, { e:[ { f:[ 1 ] }, { g:[ 1 ] } ] } ] } );
t.save( { a:[ { b:[ { c:1, d:1 } ] }, { e:[ { f:1 }, { g:1 } ] } ] } );
t.save( { a:[ { b:[ { c:1, d:1 } ] }, { e:[ { f:1, g:1 } ] } ] } );
assert.eq( 7, t.count() );
// Constraint on a prior cousin field.
assertResultsAndIndexBounds( { 'a.b.c':[[ 1, 1 ]], 'a.e.f':[[ MIN, MAX ]],
                               'a.b.d':[[ 1, 1 ]], 'a.e.g':[[ MIN, MAX ]] },
                             { 'a.b':{ $elemMatch:{ c:1, d:1 } },
                               'a.e':{ $elemMatch:{ f:1, g:1 } } } );
// Different constraint on a prior cousin field.
assertResultsAndIndexBounds( { 'a.b.c':[[ MIN, MAX ]], 'a.e.f':[[ 1, 1 ]],
                               'a.b.d':[[ MIN, MAX ]], 'a.e.g':[[ 1, 1 ]] },
                             { 'a.b':{ $elemMatch:{ d:1 } },
                               'a.e':{ $elemMatch:{ f:1, g:1 } } } );


// Cases with double dotted index field names branching to different fields at each dot, and the
// same field name strings after the second dot.
t.drop();
index = { 'a.b.c':1, 'a.e.c':1, 'a.b.d':1, 'a.e.d':1 }
t.ensureIndex( index );
t.save( { a:[ { b:[ { c:1, d:1 } ] }, { e:[ { c:1, d:1 } ] } ] } );
assert.eq( 1, t.count() );
// Constraint on a prior cousin field with the same field names.
assertResultsAndIndexBounds( { 'a.b.c':[[ 1, 1 ]], 'a.e.c':[[ MIN, MAX ]],
                               'a.b.d':[[ 1, 1 ]], 'a.e.d':[[ MIN, MAX ]] },
                             { 'a.b':{ $elemMatch:{ c:1, d:1 } },
                               'a.e':{ $elemMatch:{ c:1, d:1 } } } );
// Constraint on a prior cousin field with the same field names.
assertResultsAndIndexBounds( { 'a.b.c':[[ MIN, MAX ]], 'a.e.c':[[ 1, 1 ]],
                               'a.b.d':[[ MIN, MAX ]], 'a.e.d':[[ 1, 1 ]] },
                             { 'a.b':{ $elemMatch:{ d:1 } },
                               'a.e':{ $elemMatch:{ c:1, d:1 } } } );
