cmake_minimum_required(VERSION 3.1.0)
project(Robomongo)

include(cmake/utils.cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


set(PROJECT_NAME "Robomongo" CACHE STRING "Project Name")
set(PROJECT_NAME_TITLE ${PROJECT_NAME})
set(PROJECT_DOMAIN "www.robomongo.org" CACHE STRING "Internet domain name for ${PROJECT_NAME}")
set(PROJECT_COMPANYNAME "Paralect" CACHE STRING "Company name for ${PROJECT_NAME}")
set(PROJECT_COPYRIGHT "Copyright (C) 2013-2015 ${PROJECT_COMPANYNAME} All Rights Reserved.")
set(PROJECT_COMPANYNAME_DOMAIN "www.paralect.com" CACHE STRING "Internet domain name for ${PROJECT_NAME}")
set(PROJECT_GITHUB_FORK "www.github.com/paralect/robomongo" CACHE STRING "Git fork of ${PROJECT_NAME}")
set(PROJECT_GITHUB_ISSUES "www.github.com/paralect/robomongo/issues" CACHE STRING "Git issue link for ${PROJECT_NAME}")

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
set(MAJOR "0")
set(MINOR "8")
set(PATCH "6")
set(BUILD "dev")
set(SHORT_VERSION ${MAJOR}.${MINOR}.${PATCH} CACHE STRING "${PROJECT_NAME} short version")
set(PROJECT_VERSION "${SHORT_VERSION}${BUILD}" CACHE STRING "${PROJECT_NAME} version")

# Automatically generate code for Qt moc, uic and rcc files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5Widgets REQUIRED)

add_subdirectory(src)
