// The positional operator allows an update modifier field path to contain a sentinel ('$') path
// part that is replaced with the numeric position of an array element matched by the update's query
// spec.  <http://docs.mongodb.org/manual/reference/operators/#_S_>

// If no array element position from a query is available to substitute for the positional operator
// setinel ('$'), the update fails with an error.  SERVER-6669 SERVER-4713

t = db.jstests_updatel;
t.drop();



// The collection is empty, forcing an upsert.  In this case the query has no array position match
// to substiture for the positional operator.  SERVER-4713
t.update( {}, { $set:{ 'a.$.b':1 } }, true );
assert( db.getLastError(), "An error is reported." );
assert.eq( 0, t.count(), "No upsert occurred." );



// Save a document to the collection so it is no longer empty.
t.save( { _id:0 } );

// Now, with an existing document, trigger an update rather than an upsert.  The query has no array
// position match to substiture for the positional operator.  SERVER-6669
t.update( {}, { $set:{ 'a.$.b':1 } } );
assert( db.getLastError(), "An error is reported." );
assert.eq( [ { _id:0 } ], t.find().toArray(), "No update occurred." );



// Now, try with an update by _id (without a query array match).
t.update( { _id:0 }, { $set:{ 'a.$.b':1 } } );
assert( db.getLastError(), "An error is reported." );
assert.eq( [ { _id:0 } ], t.find().toArray(), "No update occurred." );



// Seed the collection with a document suitable for the following check.
t.remove();
t.save( { _id:0, a:[ { b:{ c:1 } } ] } );

// Now, attempt to apply an update with two nested positional operators.  There is a positional
// query match for the first positional operator but not the second.  Note that dollar sign
// substitution for multiple positional opertors is not implemented (SERVER-831).
t.update( { 'a.b.c':1 }, { $set:{ 'a.$.b.$.c':2 } } );
assert( db.getLastError(), "An error is reported" );
assert.eq( [ { _id:0, a:[ { b:{ c:1 } } ] } ], t.find().toArray(), "No update occurred." );
