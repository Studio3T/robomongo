###
# Prints CMake variables.
#
# Print all variables in the current scope:
#
# print_vars()
# print_vars("")           # the same result, as previous line
#
# Print all variables with names, matching regex, case insensitive:
#
# print_vars("SOMEVAR")
# print_vars("someVar")    # the same result, as previous line
# print_vars("^CMAKE")       # print all variables, that starts from "CMAKE"
# print_vars("VAR$")       # ends by "VAR"
# print_vars("^VAR$")      # exact match
#
function(print_vars)
    if(ARGV0)
        set(regex ${ARGV0})
    endif()
    get_cmake_property(names VARIABLES)
    foreach(name ${names})
        # Make lowercase variants for case-insensitive comparing
        string(TOLOWER "${name}" lc_name)
        string(TOLOWER "${regex}" lc_regex)

        if(lc_name MATCHES "${lc_regex}")
            message("${name}=${${name}}")
        endif()
    endforeach()
endfunction()


###
# Print content of variable.
#
# print_var(PROJECT_NAME)
# print_var("PROJECT_NAME")   # the same result as previous line
#
function(print_var name)

    # Show nice output for not defined or empty variables
    if(NOT DEFINED ${name})
        set(value " is not defined.")
    elseif("${${name}}" STREQUAL "")
        set(value " is defined but empty.")
    else()
        set(value "=${${name}}")
    endif()

    message("${name}${value}")
endfunction()
