Quick reference to CMake
========================

    if(<constant>)
    
True if the constant is `1`, `ON`, `YES`, `TRUE`, `Y`, or a non-zero number. 
False if the constant is `0`, `OFF`, `NO`, `FALSE`, `N`, `IGNORE`, `NOTFOUND`, 
the empty string, or ends in the suffix `-NOTFOUND`. Named boolean constants 
are case-insensitive. If the argument is not one of these constants, it is 
treated as a variable.

    if(<variable>)

True if the variable is defined to a value that is not a false constant. False otherwise. 
(Note macro arguments are not variables.)

    if(DEFINED <variable>)

True if the given variable is defined. It does not matter if the variable is 
true or false just if it has been set. (Note macro arguments are not variables.)
