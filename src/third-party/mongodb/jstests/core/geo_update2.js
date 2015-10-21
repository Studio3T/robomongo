
t = db.geo_update2
t.drop()

for(var x = 0; x < 10; x++ ) { 
    for(var y = 0; y < 10; y++ ) { 
        t.insert({"loc": [x, y] , x : x , y : y }); 
    } 
} 

t.ensureIndex( { loc : "2d" } ) 

function p(){
    print( "--------------" );
    for ( var y=0; y<10; y++ ){
        var c = t.find( { y : y } ).sort( { x : 1 } )
        var s = "";
        while ( c.hasNext() )
            s += c.next().z + " ";
        print( s )
    }
    print( "--------------" );
}

p()


assert.writeOK(t.update({"loc" : {"$within" : {"$center" : [[5,5], 2]}}},
                        {'$inc' : { 'z' : 1}}, false, true));
p()

assert.writeOK(t.update({}, {'$inc' : { 'z' : 1}}, false, true));
p()


assert.writeOK(t.update({"loc" : {"$within" : {"$center" : [[5,5], 2]}}},
                        {'$inc' : { 'z' : 1}}, false, true));
p()

