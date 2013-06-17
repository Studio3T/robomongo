/*
  Run all the aggregation tests
*/

/* load the test documents */
load('jstests/aggregation/data/articles.js');

// make sure we're using the right db; this is the same as "use mydb;" in shell
db = db.getSiblingDB("aggdb");


// just passing through fields
var p1 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        tags : 1,
        pageViews : 1
    }}
]});

var p1result = [
    {
        "_id" : 1,
        "pageViews" : 5,
        "tags" : [
            "fun",
            "good",
            "fun"
        ]
    },
    {
        "_id" : 2,
        "pageViews" : 7,
        "tags" : [
            "fun",
            "nasty"
        ]
    },
    {
        "_id" : 3,
        "pageViews" : 6,
        "tags" : [
            "nasty",
            "filthy"
        ]
    }
];

assert.eq(p1.result, p1result, 'p1 failed');


// a simple array unwinding
var u1 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $unwind : "$tags" }
]});

var u1result = [
    {
        "_id" : 1,
        "title" : "this is my title",
        "author" : "bob",
        "posted" : ISODate("2004-03-21T18:59:54Z"),
        "pageViews" : 5,
        "tags" : "fun",
        "comments" : [
            {
                "author" : "joe",
                "text" : "this is cool"
            },
            {
                "author" : "sam",
                "text" : "this is bad"
            }
        ],
        "other" : {
            "foo" : 5
        }
    },
    {
        "_id" : 1,
        "title" : "this is my title",
        "author" : "bob",
        "posted" : ISODate("2004-03-21T18:59:54Z"),
        "pageViews" : 5,
        "tags" : "good",
        "comments" : [
            {
                "author" : "joe",
                "text" : "this is cool"
            },
            {
                "author" : "sam",
                "text" : "this is bad"
            }
        ],
        "other" : {
            "foo" : 5
        }
    },
    {
        "_id" : 1,
        "title" : "this is my title",
        "author" : "bob",
        "posted" : ISODate("2004-03-21T18:59:54Z"),
        "pageViews" : 5,
        "tags" : "fun",
        "comments" : [
            {
                "author" : "joe",
                "text" : "this is cool"
            },
            {
                "author" : "sam",
                "text" : "this is bad"
            }
        ],
        "other" : {
            "foo" : 5
        }
    },
    {
        "_id" : 2,
        "title" : "this is your title",
        "author" : "dave",
        "posted" : ISODate("2030-08-08T04:11:10Z"),
        "pageViews" : 7,
        "tags" : "fun",
        "comments" : [
            {
                "author" : "barbara",
                "text" : "this is interesting"
            },
            {
                "author" : "jenny",
                "text" : "i like to play pinball",
                "votes" : 10
            }
        ],
        "other" : {
            "bar" : 14
        }
    },
    {
        "_id" : 2,
        "title" : "this is your title",
        "author" : "dave",
        "posted" : ISODate("2030-08-08T04:11:10Z"),
        "pageViews" : 7,
        "tags" : "nasty",
        "comments" : [
            {
                "author" : "barbara",
                "text" : "this is interesting"
            },
            {
                "author" : "jenny",
                "text" : "i like to play pinball",
                "votes" : 10
            }
        ],
        "other" : {
            "bar" : 14
        }
    },
    {
        "_id" : 3,
        "title" : "this is some other title",
        "author" : "jane",
        "posted" : ISODate("2000-12-31T05:17:14Z"),
        "pageViews" : 6,
        "tags" : "nasty",
        "comments" : [
            {
                "author" : "will",
                "text" : "i don't like the color"
            },
            {
                "author" : "jenny",
                "text" : "can i get that in green?"
            }
        ],
        "other" : {
            "bar" : 14
        }
    },
    {
        "_id" : 3,
        "title" : "this is some other title",
        "author" : "jane",
        "posted" : ISODate("2000-12-31T05:17:14Z"),
        "pageViews" : 6,
        "tags" : "filthy",
        "comments" : [
            {
                "author" : "will",
                "text" : "i don't like the color"
            },
            {
                "author" : "jenny",
                "text" : "can i get that in green?"
            }
        ],
        "other" : {
            "bar" : 14
        }
    }
];

assert.eq(u1.result, u1result, 'u1 failed');

// unwind an array at the end of a dotted path
db.ut.drop();
db.ut.save({_id: 4, a:1, b:{e:7, f:[4, 3, 2, 1]}, c:12, d:17});
var u2 = db.runCommand(
{ aggregate : "ut", pipeline : [
    { $unwind : "$b.f" }
]});

var u2result = [
    {
        "_id" : 4,
        "a" : 1,
        "b" : {
            "e" : 7,
            "f" : 4
        },
        "c" : 12,
        "d" : 17
    },
    {
        "_id" : 4,
        "a" : 1,
        "b" : {
            "e" : 7,
            "f" : 3
        },
        "c" : 12,
        "d" : 17
    },
    {
        "_id" : 4,
        "a" : 1,
        "b" : {
            "e" : 7,
            "f" : 2
        },
        "c" : 12,
        "d" : 17
    },
    {
        "_id" : 4,
        "a" : 1,
        "b" : {
            "e" : 7,
            "f" : 1
        },
        "c" : 12,
        "d" : 17
    }
];

assert.eq(u2.result, u2result, 'u2 failed');


// combining a projection with unwinding an array
var p2 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
        pageViews : 1
    }},
    { $unwind : "$tags" }
]});

var p2result = [
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
        "tags" : "fun"
    },
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
        "tags" : "good"
    },
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
        "tags" : "fun"
    },
    {
        "_id" : 2,
        "author" : "dave",
        "pageViews" : 7,
        "tags" : "fun"
    },
    {
        "_id" : 2,
        "author" : "dave",
        "pageViews" : 7,
        "tags" : "nasty"
    },
    {
        "_id" : 3,
        "author" : "jane",
        "pageViews" : 6,
        "tags" : "nasty"
    },
    {
        "_id" : 3,
        "author" : "jane",
        "pageViews" : 6,
        "tags" : "filthy"
    }
];

assert.eq(p2.result, p2result, 'p2 failed');


// pulling values out of subdocuments
var p3 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        otherfoo : "$other.foo",
        otherbar : "$other.bar"
    }}
]});

var p3result = [
    {
        "_id" : 1,
        "otherfoo" : 5
    },
    {
        "_id" : 2,
        "otherbar" : 14
    },
    {
        "_id" : 3,
        "otherbar" : 14
    }
];

assert.eq(p3.result, p3result, 'p3 failed');


// projection includes a computed value
var p4 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        daveWroteIt : { $eq:["$author", "dave"] }
    }}
]});

var p4result = [
    {
        "_id" : 1,
        "author" : "bob",
        "daveWroteIt" : false
    },
    {
        "_id" : 2,
        "author" : "dave",
        "daveWroteIt" : true
    },
    {
        "_id" : 3,
        "author" : "jane",
        "daveWroteIt" : false
    }
];

assert.eq(p4.result, p4result, 'p4 failed');


// projection includes a virtual (fabricated) document
var p5 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        pageViews : 1,
        tags : 1
    }},
    { $unwind : "$tags" },
    { $project : {
        author : 1,
        subDocument : { foo : "$pageViews", bar : "$tags"  }
    }}
]});

var p5result = [
    {
        "_id" : 1,
        "author" : "bob",
        "subDocument" : {
            "foo" : 5,
            "bar" : "fun"
        }
    },
    {
        "_id" : 1,
        "author" : "bob",
        "subDocument" : {
            "foo" : 5,
            "bar" : "good"
        }
    },
    {
        "_id" : 1,
        "author" : "bob",
        "subDocument" : {
            "foo" : 5,
            "bar" : "fun"
        }
    },
    {
        "_id" : 2,
        "author" : "dave",
        "subDocument" : {
            "foo" : 7,
            "bar" : "fun"
        }
    },
    {
        "_id" : 2,
        "author" : "dave",
        "subDocument" : {
            "foo" : 7,
            "bar" : "nasty"
        }
    },
    {
        "_id" : 3,
        "author" : "jane",
        "subDocument" : {
            "foo" : 6,
            "bar" : "nasty"
        }
    },
    {
        "_id" : 3,
        "author" : "jane",
        "subDocument" : {
            "foo" : 6,
            "bar" : "filthy"
        }
    }
];

assert.eq(p5.result, p5result, 'p5 failed');


// multi-step aggregate
// nested expressions in computed fields
var p6 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
        pageViews : 1
    }},
    { $unwind : "$tags" },
    { $project : {
        author : 1,
        tag : "$tags",
        pageViews : 1,
        daveWroteIt : { $eq:["$author", "dave"] },
        weLikeIt : { $or:[ { $eq:["$author", "dave"] },
                           { $eq:["$tags", "good"] } ] }
    }}
]});

var p6result = [
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
        "tag" : "fun",
        "daveWroteIt" : false,
        "weLikeIt" : false
    },
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
        "tag" : "good",
        "daveWroteIt" : false,
        "weLikeIt" : true
    },
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
        "tag" : "fun",
        "daveWroteIt" : false,
        "weLikeIt" : false
    },
    {
        "_id" : 2,
        "author" : "dave",
        "pageViews" : 7,
        "tag" : "fun",
        "daveWroteIt" : true,
        "weLikeIt" : true
    },
    {
        "_id" : 2,
        "author" : "dave",
        "pageViews" : 7,
        "tag" : "nasty",
        "daveWroteIt" : true,
        "weLikeIt" : true
    },
    {
        "_id" : 3,
        "author" : "jane",
        "pageViews" : 6,
        "tag" : "nasty",
        "daveWroteIt" : false,
        "weLikeIt" : false
    },
    {
        "_id" : 3,
        "author" : "jane",
        "pageViews" : 6,
        "tag" : "filthy",
        "daveWroteIt" : false,
        "weLikeIt" : false
    }
];

assert.eq(p6.result, p6result, 'p6 failed');


// slightly more complex computed expression; $ifNull
var p7 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        theSum : { $add:["$pageViews",
                         { $ifNull:["$other.foo",
                                    "$other.bar"] } ] }
    }}
]});

var p7result = [
    {
        "_id" : 1,
        "theSum" : 10
    },
    {
        "_id" : 2,
        "theSum" : 21
    },
    {
        "_id" : 3,
        "theSum" : 20
    }
];

assert.eq(p7.result, p7result, 'p7 failed');


// dotted path inclusion; _id exclusion
var p8 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        _id : 0,
        author : 1,
        tags : 1,
        "comments.author" : 1
    }},
    { $unwind : "$tags" }
]});

var p8result = [
    {
        "author" : "bob",
        "tags" : "fun",
        "comments" : [
            {
                "author" : "joe"
            },
            {
                "author" : "sam"
            }
        ]
    },
    {
        "author" : "bob",
        "tags" : "good",
        "comments" : [
            {
                "author" : "joe"
            },
            {
                "author" : "sam"
            }
        ]
    },
    {
        "author" : "bob",
        "tags" : "fun",
        "comments" : [
            {
                "author" : "joe"
            },
            {
                "author" : "sam"
            }
        ]
    },
    {
        "author" : "dave",
        "tags" : "fun",
        "comments" : [
            {
                "author" : "barbara"
            },
            {
                "author" : "jenny"
            }
        ]
    },
    {
        "author" : "dave",
        "tags" : "nasty",
        "comments" : [
            {
                "author" : "barbara"
            },
            {
                "author" : "jenny"
            }
        ]
    },
    {
        "author" : "jane",
        "tags" : "nasty",
        "comments" : [
            {
                "author" : "will"
            },
            {
                "author" : "jenny"
            }
        ]
    },
    {
        "author" : "jane",
        "tags" : "filthy",
        "comments" : [
            {
                "author" : "will"
            },
            {
                "author" : "jenny"
            }
        ]
    }
];

assert.eq(p8.result, p8result, 'p8 failed');


// collapse a dotted path with an intervening array
var p9 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        _id : 0,
        author : 1,
        commentsAuthor : "$comments.author"
    }}
]});

var p9result = [
    {
        "author" : "bob",
        "commentsAuthor" : [
            "joe",
            "sam"
        ]
    },
    {
        "author" : "dave",
        "commentsAuthor" : [
            "barbara",
            "jenny"
        ]
    },
    {
        "author" : "jane",
        "commentsAuthor" : [
            "will",
            "jenny"
        ]
    }
];

assert.eq(p9.result, p9result, 'p9 failed');


// simple sort
var p10 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $sort : { title : 1 }
    }
]});

var p10result = [
    {
        "_id" : 1,
        "title" : "this is my title",
        "author" : "bob",
        "posted" : ISODate("2004-03-21T18:59:54Z"),
        "pageViews" : 5,
        "tags" : [
            "fun",
            "good",
            "fun"
        ],
        "comments" : [
            {
                "author" : "joe",
                "text" : "this is cool"
            },
            {
                "author" : "sam",
                "text" : "this is bad"
            }
        ],
        "other" : {
            "foo" : 5
        }
    },
    {
        "_id" : 3,
        "title" : "this is some other title",
        "author" : "jane",
        "posted" : ISODate("2000-12-31T05:17:14Z"),
        "pageViews" : 6,
        "tags" : [
            "nasty",
            "filthy"
        ],
        "comments" : [
            {
                "author" : "will",
                "text" : "i don't like the color"
            },
            {
                "author" : "jenny",
                "text" : "can i get that in green?"
            }
        ],
        "other" : {
            "bar" : 14
        }
    },
    {
        "_id" : 2,
        "title" : "this is your title",
        "author" : "dave",
        "posted" : ISODate("2030-08-08T04:11:10Z"),
        "pageViews" : 7,
        "tags" : [
            "fun",
            "nasty"
        ],
        "comments" : [
            {
                "author" : "barbara",
                "text" : "this is interesting"
            },
            {
                "author" : "jenny",
                "text" : "i like to play pinball",
                "votes" : 10
            }
        ],
        "other" : {
            "bar" : 14
        }
    }
];

assert.eq(p10.result, p10result, 'p10 failed');


// unwind on nested array
db.p11.drop();
db.p11.save( {
    _id : 5,
    name : 'MongoDB',
    items : {
        authors : ['jay', 'vivek', 'bjornar'],
        dbg : [17, 42]
    },
    favorites : ['pickles', 'ice cream', 'kettle chips']
});

var p11 = db.runCommand(
{ aggregate : "p11", pipeline : [
    { $unwind : "$items.authors" },
    { $project : {
        name : 1,
        author : "$items.authors"
    }},
]});

p11result = [
    {
        "_id" : 5,
        "name" : "MongoDB",
        "author" : "jay"
    },
    {
        "_id" : 5,
        "name" : "MongoDB",
        "author" : "vivek"
    },
    {
        "_id" : 5,
        "name" : "MongoDB",
        "author" : "bjornar"
    }
];

assert.eq(p11.result, p11result, 'p11 failed');


// multiply test
var p12 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        theProduct : { $multiply:["$pageViews",
                         { $ifNull:["$other.foo",
                                    "$other.bar"] } ] }
    }}
]});

var p12result = [
    {
        "_id" : 1,
        "theProduct" : 25
    },
    {
        "_id" : 2,
        "theProduct" : 98
    },
    {
        "_id" : 3,
        "theProduct" : 84
    }
];

assert.eq(p12.result, p12result, 'p12 failed');


// subtraction test
var p13 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        theDifference : { $subtract:["$pageViews",
                         { $ifNull:["$other.foo",
                                    "$other.bar"] } ] }
    }}
]});

var p13result = [
    {
        "_id" : 1,
        "theDifference" : 0
    },
    {
        "_id" : 2,
        "theDifference" : -7
    },
    {
        "_id" : 3,
        "theDifference" : -8
    }
];

assert.eq(p13.result, p13result, 'p13 failed');


// mod test
var p14 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        theRemainder : { $mod:[
            { $ifNull:["$other.foo",
                    "$other.bar"] },
                "$pageViews", ] }
    }}
]});

var p14result = [
    {
        "_id" : 1,
        "theRemainder" : 0
    },
    {
        "_id" : 2,
        "theRemainder" : 0
    },
    {
        "_id" : 3,
        "theRemainder" : 2
    }
];

assert.eq(p14.result, p14result, 'p14 failed');


// toUpper test
var p15 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : {$toUpper: "$author"},
        pageViews : 1
    }}
]});

var p15result = [
    {
        "_id" : 1,
        "author" : "BOB",
        "pageViews" : 5
    },
    {
        "_id" : 2,
        "author" : "DAVE",
        "pageViews" : 7
    },
    {
        "_id" : 3,
        "author" : "JANE",
        "pageViews" : 6
    }
];

assert.eq(p15.result, p15result, 'p15 failed');


// toLower test
var p16 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : {$toUpper: "$author"},
        pageViews : 1
    }},
    { $project : {
    author : {$toLower: "$author"},
    pageViews : 1
    }}
]});

var p16result = [
    {
        "_id" : 1,
        "author" : "bob",
        "pageViews" : 5,
    },
    {
        "_id" : 2,
        "author" : "dave",
        "pageViews" : 7,
    },
    {
        "_id" : 3,
        "author" : "jane",
        "pageViews" : 6,
    }
];

assert.eq(p16.result, p16result, 'p16 failed');


// substr test
var p17 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : {$substr: ["$author", 1, 2]},
    }}
]});

var p17result = [
    {
        "_id" : 1,
        "author" : "ob"
    },
    {
        "_id" : 2,
        "author" : "av"
    },
    {
        "_id" : 3,
        "author" : "an"
    }
];

assert.eq(p17.result, p17result, 'p17 failed');


// strcasecmp test
var p18 = db.runCommand(
{aggregate : "article", pipeline : [
    { $project : {
        tags : 1,
        thisisalametest : {$strcasecmp: ["foo","bar"]},
        thisisalamepass : {$strcasecmp: ["foo","foo"]}
    }}
]});

var p18result = [
    {
        "_id" : 1,
        "tags" : [
            "fun",
            "good",
            "fun"
        ],
        "thisisalametest" : 1,
        "thisisalamepass" : 0
    },
    {
        "_id" : 2,
        "tags" : [
            "fun",
            "nasty"
        ],
        "thisisalametest" : 1,
        "thisisalamepass" : 0
    },
    {
        "_id" : 3,
        "tags" : [
            "nasty",
            "filthy"
        ],
        "thisisalametest" : 1,
        "thisisalamepass" : 0
    }
];

assert.eq(p18.result, p18result, 'p18 failed');


// date tests
var p19 = db.runCommand({aggregate : "article", pipeline : [
    { $project : {
        authors: 1,
        posted: 1,
        seconds: {$second: "$posted"},
        minutes: {$minute: "$posted"},
        hour: {$hour: "$posted"},
        dayOfYear: {$dayOfYear: "$posted"},
        dayOfMonth: {$dayOfMonth: "$posted"},
        dayOfWeek: {$dayOfWeek: "$posted"},
        month: {$month: "$posted"},
        week: {$week: "$posted"},
        year: {$year: "$posted"}
    }}
]});

var p19result = [
    {
        "_id" : 1,
        "posted" : ISODate("2004-03-21T18:59:54Z"),
        "seconds" : 54,
        "minutes" : 59,
        "hour" : 18,
        "dayOfYear" : 81,
        "dayOfMonth" : 21,
        "dayOfWeek" : 1,
        "month" : 3,
        "week" : 12,
        "year" : 2004,
    },
    {
        "_id" : 2,
        "posted" : ISODate("2030-08-08T04:11:10Z"),
        "seconds" : 10,
        "minutes" : 11,
        "hour" : 4,
        "dayOfYear" : 220,
        "dayOfMonth" : 8,
        "dayOfWeek" : 5,
        "month" : 8,
        "week" : 31,
        "year" : 2030,
    },
    {
        "_id" : 3,
        "posted" : ISODate("2000-12-31T05:17:14Z"),
        "seconds" : 14,
        "minutes" : 17,
        "hour" : 5,
        "dayOfYear" : 366,
        "dayOfMonth" : 31,
        "dayOfWeek" : 1,
        "month" : 12,
        "week" : 53,
        "year" : 2000,
    }
];

assert.eq(p19.result, p19result, 'p19 failed');


db.vartype.drop();
db.vartype.save({ x : 17, y : "foo"});

// just passing through fields
var p20 = db.runCommand(
{ aggregate : "vartype", pipeline : [
    { $project : {
        all_numbers : { $add:[1, "$x", 2, "$x"] },
        string_fields : { $concat:[3, "$y", 4, "$y"] },
        number_fields : { $concat:["a", "$x", "b", "$x"] },
        all_strings : { $concat:["c", "$y", "d", "$y"] },
        potpourri_1 : { $concat:[5, "$y", "e", "$x"] },
        potpourri_2 : { $concat:[6, "$x", "f", "$y"] },
        potpourri_3 : { $concat:["g", "$y", 7, "$x"] },
        potpourri_4 : { $concat:["h", "$x", 8, "$y"] },
        _id: 0
    }}
]});

var p20result = [
    {
        "all_numbers" : 37,
        "string_fields" : "3foo4foo",
        "number_fields" : "a17b17",
        "all_strings" : "cfoodfoo",
        "potpourri_1" : "5fooe17",
        "potpourri_2" : "617ffoo",
        "potpourri_3" : "gfoo717",
        "potpourri_4" : "h178foo"
    }
];

assert.eq(p20.result, p20result, 'p20 failed');

// ternary conditional operator
var p21 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        _id : 0,
        author : 1,
        pageViews : { $cond : [ {$eq:["$author", "dave"]},
                                {$add:["$pageViews", 1000]}, "$pageViews" ]
        }
    }}
]});

var p21result = [
    {
        "author" : "bob",
        "pageViews" : 5
    },
    {
        "author" : "dave",
        "pageViews" : 1007
    },
    {
        "author" : "jane",
        "pageViews" : 6
    }
];

assert.eq(p21.result, p21result, 'p21 failed');


// simple matching
var m1 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $match : { author : "dave" } }
]});

var m1result = [
    {
        "_id" : 2,
        "title" : "this is your title",
        "author" : "dave",
        "posted" : ISODate("2030-08-08T04:11:10Z"),
        "pageViews" : 7,
        "tags" : [
            "fun",
            "nasty"
        ],
        "comments" : [
            {
                "author" : "barbara",
                "text" : "this is interesting"
            },
            {
                "author" : "jenny",
                "text" : "i like to play pinball",
                "votes" : 10
            }
        ],
        "other" : {
            "bar" : 14
        }
    }
];

assert.eq(m1.result, m1result, 'm1 failed');


// combining matching with a projection
var m2 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        title : 1,
        author : 1,
        pageViews : 1,
        tags : 1,
        comments : 1
    }},
    { $unwind : "$tags" },
    { $match : { tags : "nasty" } }
]});

var m2result = [
    {
        "_id" : 2,
        "title" : "this is your title",
        "author" : "dave",
        "pageViews" : 7,
        "tags" : "nasty",
        "comments" : [
            {
                "author" : "barbara",
                "text" : "this is interesting"
            },
            {
                "author" : "jenny",
                "text" : "i like to play pinball",
                "votes" : 10
            }
        ]
    },
    {
        "_id" : 3,
        "title" : "this is some other title",
        "author" : "jane",
        "pageViews" : 6,
        "tags" : "nasty",
        "comments" : [
            {
                "author" : "will",
                "text" : "i don't like the color"
            },
            {
                "author" : "jenny",
                "text" : "can i get that in green?"
            }
        ]
    }
];

assert.eq(m2.result, m2result, 'm2 failed');


// group by tag, _id is a field reference
var g1 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
        pageViews : 1
    }},
    { $unwind : "$tags" },
    { $group : {
        _id : "$tags",
        docsByTag : { $sum : 1 },
        viewsByTag : { $sum : "$pageViews" }
    }},
    {$sort: {'_id': 1}}
]});

var g1result = [
    {
        "_id" : "filthy",
        "docsByTag" : 1,
        "viewsByTag" : 6
    },
    {
        "_id" : "fun",
        "docsByTag" : 3,
        "viewsByTag" : 17
    },
    {
        "_id" : "good",
        "docsByTag" : 1,
        "viewsByTag" : 5
    },
    {
        "_id" : "nasty",
        "docsByTag" : 2,
        "viewsByTag" : 13
    },
];

assert.eq(g1.result, g1result, 'g1 failed');


// $max, and averaging in a final projection; _id is structured
var g2 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
        pageViews : 1
    }},
    { $unwind : "$tags" },
    { $group : {
        _id: { tags : "$tags" },
        docsByTag : { $sum : 1 },
        viewsByTag : { $sum : "$pageViews" },
        mostViewsByTag : { $max : "$pageViews" },
    }},
    { $project : {
        _id: false,
        tag : "$_id.tags",
        mostViewsByTag : 1,
        docsByTag : 1,
        viewsByTag : 1,
        avgByTag : { $divide:["$viewsByTag", "$docsByTag"] }
    }},
    {$sort: {'docsByTag': 1, 'viewsByTag': 1}}
]});

var g2result = [
    {
        "docsByTag" : 1,
        "viewsByTag" : 5,
        "mostViewsByTag" : 5,
        "tag" : "good",
        "avgByTag" : 5
    },
    {
        "docsByTag" : 1,
        "viewsByTag" : 6,
        "mostViewsByTag" : 6,
        "tag" : "filthy",
        "avgByTag" : 6
    },
    {
        "docsByTag" : 2,
        "viewsByTag" : 13,
        "mostViewsByTag" : 7,
        "tag" : "nasty",
        "avgByTag" : 6.5
    },
    {
        "docsByTag" : 3,
        "viewsByTag" : 17,
        "mostViewsByTag" : 7,
        "tag" : "fun",
        "avgByTag" : 5.666666666666667
    }
];

assert.eq(g2.result, g2result, 'g2 failed');


// $push as an accumulator; can pivot data
var g3 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
    }},
    { $unwind : "$tags" },
    { $group : {
        _id : { tags : "$tags" },
        authors : { $push : "$author" }
    }},
    {$sort: {'_id': 1}}
]});

var g3result = [
    {
        "_id" : {
            "tags" : "filthy"
        },
        "authors" : [
            "jane"
        ]
    },
    {
        "_id" : {
            "tags" : "fun"
        },
        "authors" : [
            "bob",
            "bob",
            "dave"
        ]
    },
    {
        "_id" : {
            "tags" : "good"
        },
        "authors" : [
            "bob"
        ]
    },
    {
        "_id" : {
            "tags" : "nasty"
        },
        "authors" : [
            "dave",
            "jane"
        ]
    }
];

assert.eq(g3.result, g3result, 'g3 failed');


// $avg, and averaging in a final projection
var g4 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
        pageViews : 1
    }},
    { $unwind : "$tags" },
    { $group : {
        _id: { tags : "$tags" },
        docsByTag : { $sum : 1 },
        viewsByTag : { $sum : "$pageViews" },
        avgByTag : { $avg : "$pageViews" },
    }},
    {$sort: {'_id': 1}}
]});

var g4result = [
    {
        "_id" : {
            "tags" : "filthy"
        },
        "docsByTag" : 1,
        "viewsByTag" : 6,
        "avgByTag" : 6
    },
    {
        "_id" : {
            "tags" : "fun"
        },
        "docsByTag" : 3,
        "viewsByTag" : 17,
        "avgByTag" : 5.666666666666667
    },
    {
        "_id" : {
            "tags" : "good"
        },
        "docsByTag" : 1,
        "viewsByTag" : 5,
        "avgByTag" : 5
    },
    {
        "_id" : {
            "tags" : "nasty"
        },
        "docsByTag" : 2,
        "viewsByTag" : 13,
        "avgByTag" : 6.5
    }
];

assert.eq(g4.result, g4result, 'g4 failed');


// $addToSet as an accumulator; can pivot data
var g5 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $project : {
        author : 1,
        tags : 1,
    }},
    { $unwind : "$tags" },
    { $group : {
        _id : { tags : "$tags" },
        authors : { $addToSet : "$author" }
    }},
    {$sort: {'_id': 1}}
]});

var g5result = [
    {
        "_id" : {
            "tags" : "filthy"
        },
        "authors" : [
            "jane"
        ]
    },
    {
        "_id" : {
            "tags" : "fun"
        },
        "authors" : [
            "dave",
            "bob"
        ]
    },
    {
        "_id" : {
            "tags" : "good"
        },
        "authors" : [
            "bob"
        ]
    },
    {
        "_id" : {
            "tags" : "nasty"
        },
        "authors" : [
            "jane",
            "dave"
        ]
    }
];

assert.eq(g5.result, g5result, 'g5 failed');


// $first and $last accumulators, constant _id
var g6 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $sort : { author : -1 } },
    { $group : {
        _id : "authors", /* constant string, *not* a field reference */
        firstAuthor : { $last : "$author" }, /* note reverse sort above */
        lastAuthor : { $first : "$author" }, /* note reverse sort above */
        count : { $sum : 1 }
    }}
]});

var g6result = [
    {
        "_id" : "authors",
        firstAuthor : "bob",
        lastAuthor : "jane",
        count : 3
    }
];

// Test unwind on an unused field
var g7 = db.runCommand(
{ aggregate : "article", pipeline : [
    { $unwind : '$tags' },
    { $group : {
        _id : "tag_count", /* constant string, *not* a field reference */
        count : { $sum : 1 }
    }}
]});
assert.eq(g7.result[0].count, 7);
