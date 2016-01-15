#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-omit-frame-pointer -fPIC -fno-strict-aliasing -pthread -O0 -fstack-protector -fno-builtin-memcmp -rdynamic -fuse-ld=gold")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(PROJECT_NAME "Robomongo")
set(PROJECT_NAME_TITLE ${PROJECT_NAME})
set(PROJECT_DOMAIN "www.robomongo.org")
set(PROJECT_COMPANYNAME "Paralect")
set(PROJECT_COPYRIGHT "Copyright (C) 2013-2015 ${PROJECT_COMPANYNAME} All Rights Reserved.")
set(PROJECT_COMPANYNAME_DOMAIN "www.paralect.com")
set(PROJECT_GITHUB_FORK "www.github.com/paralect/robomongo")
set(PROJECT_GITHUB_ISSUES "www.github.com/paralect/robomongo/issues")

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
