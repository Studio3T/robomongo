// Simple covered index query test

// Include helpers for analyzing explain output.
load("jstests/libs/analyze_plan.js");

var coll = db.getCollection("covered_simple_1")
coll.drop()
for (i=0;i<10;i++) {
    coll.insert({foo:i})
}
for (i=0;i<10;i++) {
    coll.insert({foo:i})
}
for (i=0;i<5;i++) {
    coll.insert({bar:i})
}
coll.insert({foo:"string"})
coll.insert({foo:{bar:1}})
coll.insert({foo:null})
coll.ensureIndex({foo:1})

// Test equality with int value
var plan = coll.find({foo:1}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.1 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.1 - docs examined should be 0 for covered query")

// Test equality with string value
var plan = coll.find({foo:"string"}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.2 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.2 - docs examined should be 0 for covered query")

// Test equality with doc value
var plan = coll.find({foo:{bar:1}}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.3 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.3 - docs examined should be 0 for covered query")

// Test no query
var plan = coll.find({}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.4 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.4 - docs examined should be 0 for covered query")

// Test range query
var plan = coll.find({foo:{$gt:2,$lt:6}}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.5 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.5 - docs examined should be 0 for covered query")

// Test in query
var plan = coll.find({foo:{$in:[5,8]}}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.6 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.6 - docs examined should be 0 for covered query")

// Test no return
var plan = coll.find({foo:"2"}, {foo:1, _id:0}).hint({foo:1}).explain("executionStats");
assert(isIndexOnly(plan.queryPlanner.winningPlan),
       "simple.1.7 - indexOnly should be true on covered query")
assert.eq(0, plan.executionStats.totalDocsExamined,
          "simple.1.7 - nscannedObjects should be 0 for covered query")

print ('all tests pass')
