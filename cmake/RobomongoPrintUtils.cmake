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

###
# Prints included directories
#
function(print_include_dirs)
    get_property(dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
    foreach(dir ${dirs})
        message("${dir}")
    endforeach()
endfunction()

###
# Prints link directories
#
function(print_link_dirs)
    get_property(dirs DIRECTORY PROPERTY LINK_DIRECTORIES)
    foreach(dir ${dirs})
        message("${dir}")
    endforeach()
endfunction()

###
# Prints compile definitions
#
function(print_compile_definitions)
    get_property(defs DIRECTORY PROPERTY COMPILE_DEFINITIONS)
    foreach(def ${defs})
        message("${def}")
    endforeach()
endfunction()


###
# Prints target include directories
#
function(print_target_include_dirs target)
    get_property(dirs TARGET ${target} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    foreach(dir ${dirs})
        message("${dir}")
    endforeach()
endfunction()

###
# Prints target compile definitions
#
function(print_target_compile_definitions target)
    get_property(defs TARGET ${target} PROPERTY INTERFACE_COMPILE_DEFINITIONS)
    foreach(def ${defs})
        message("${def}")
    endforeach()
endfunction()

###
# Prints target link libraries
#
function(print_target_link_libraries target)
    get_property(defs TARGET ${target} PROPERTY INTERFACE_LINK_LIBRARIES)
    foreach(def ${defs})
        message("${def}")
    endforeach()
endfunction()

###
# Prints Qt5 module plugins
#
function(print_qt_module_plugins module)
    foreach(plugin ${Qt5${module}_PLUGINS})
        get_target_property(loc ${plugin} LOCATION)
        message("Plugin ${plugin} is at location ${loc}")
    endforeach()
endfunction()
