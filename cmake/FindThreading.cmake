# This wrapper is created in order to support CMake 3.0
# Only starting from CMake 3.1 "Threads" module exports
# "Threads::Threads" target.

find_package(Threads REQUIRED)

# Support for CMake 3.0
# Export "Threads::Threads" imported target if it wasn't exported.
# This lines are taken from the "FindThreads.cmake" of CMake 3.1:
# https://github.com/Kitware/CMake/blob/v3.1.0/Modules/FindThreads.cmake
if(THREADS_FOUND AND NOT TARGET Threads::Threads)
    add_library(Threads::Threads INTERFACE IMPORTED)

    if(THREADS_HAVE_PTHREAD_ARG)
        set_property(TARGET Threads::Threads PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
    endif()

    if(CMAKE_THREAD_LIBS_INIT)
        set_property(TARGET Threads::Threads PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}")
    endif()
endif()
