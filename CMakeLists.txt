cmake_minimum_required(VERSION 3.0.0)
#set(CMAKE_CXX_SYSROOT_FLAG_CODE "list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS cc)")
project(pgAdmin3 VERSION 1.26.0)
include_directories(. include include/libssh2
)
add_compile_definitions(WX_PRECOMP)
add_compile_definitions(WX_PRECOMP)
add_compile_definitions(wxUSE_UNICODE)
add_compile_definitions(EMBED_XRC)
#add_compile_definitions(EMBED_XRC)
add_compile_options(-Wno-narrowing
)

#set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS c)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES
    agent/*.c*
    ctl/*.c*
    db/*.c*
    dd/*.c* debugger/*.c* dlg/*.c* frm/*.c*
    gqb/*.c*
     hotdraw/*.c* ogl/*.c* pgscript/*.c*
     pro_scheduler/*.c* schema/*.c* slony/*.c* ui/xrcDialogs.cpp utils/*.c*
)

##########################################################
find_package(LibXslt REQUIRED)
if(NOT LIBXSLT_FOUND)
    message(SEND_ERROR "Failed to find LibXslt")
    return()
else()
    include_directories(${LIBXSLT_INCLUDE_DIR})
    link_libraries(${LIBXSLT_LIBRARIES})
endif()
find_package(LibXml2 REQUIRED)
if(NOT LIBXML2_FOUND)
    message(SEND_ERROR "Failed to find LibXml2")
    return()
else()
    include_directories(${LIBXML2_INCLUDE_DIR})
    link_libraries(${LIBXML2_LIBRARIES})
endif()
find_package(wxWidgets REQUIRED stc aui base net core xrc html xml)
    if(NOT wxWidgets_FOUND)
        message(SEND_ERROR "Failed to find wxWidgets ")
        return()
    else()
        include(${wxWidgets_USE_FILE})
        include_directories(${wxWidgets_INCLUDE_DIRS})
        link_libraries(${wxWidgets_LIBRARIES})
    endif()

find_package(PostgreSQL REQUIRED)
if(NOT PostgreSQL_FOUND)
    message(SEND_ERROR "Failed to find PostgreSql")
    return()
else()
    include_directories(${PostgreSQL_INCLUDE_DIRS})
    include_directories(${PostgreSQL_INCLUDE_DIRS}/server)
    link_libraries(${PostgreSQL_LIBRARIES})
endif()


add_executable(pgAdmin3 pgAdmin3.cpp ${SOURCES})
target_precompile_headers(pgAdmin3 PUBLIC include/precomp.h)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

