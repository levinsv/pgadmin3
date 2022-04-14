cmake_minimum_required(VERSION 3.0.0)
project(pgAdmin3 VERSION 1.24.0)
include_directories(. include)
add_compile_definitions(WX_PRECOMP)
add_compile_definitions(WX_PRECOMP)
add_compile_definitions(wxUSE_UNICODE)
add_compile_definitions(EMBED_XRC)
#add_compile_definitions(EMBED_XRC)

SET(GCC_COVERAGE_COMPILE_FLAGS "-Wdeprecated-declarations")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SOURCES
    src/*.c*
    agent/*.c*
    ctl/*.c*
    db/*.c*
    dd/*.c* debugger/*.c* dlg/*.c* frm/*.c*
    gqb/*.c*
     hotdraw/*.c* ogl/*.c* pgscript/*.c*
     pro_scheduler/*.c* schema/*.c* slony/*.c* ui/*.c* utils/*.c*
)

##########################################################
find_package(LibXml2 REQUIRED)
if(NOT LIBXML2_FOUND)
    message(SEND_ERROR "Failed to find LibXml2")
    return()
else()
    include_directories(${LIBXML2_INCLUDE_DIR})
endif()
find_package(wxWidgets REQUIRED)
if(NOT wxWidgets_FOUND)
    message(SEND_ERROR "Failed to find wxWidgets")
    return()
else()
    message(STATUS "wxWidgets_INCLUDE_DIRS=${wxWidgets_INCLUDE_DIRS}" )
    include(${wxWidgets_USE_FILE})
    include_directories(${wxWidgets_INCLUDE_DIRS})
    link_libraries(${wxWidgets_LIBRARIES})
endif()
find_package(PostgreSQL REQUIRED)
if(NOT wxWidgets_FOUND)
    message(SEND_ERROR "Failed to find PostgreSql")
    return()
else()
    include(${wxWidgets_USE_FILE})
    include_directories(${PostgreSQL_INCLUDE_DIRS})
    include_directories(${PostgreSQL_INCLUDE_DIRS}/server)
    link_libraries(${PostgreSQL_LIBRARIES})
endif()

#include(CTest)
#enable_testing()

add_executable(pgAdmin3 pgAdmin3.cpp ${SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
#include(CPack)
