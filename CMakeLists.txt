cmake_minimum_required(VERSION 3.23.0)
#set(CMAKE_CXX_SYSROOT_FLAG_CODE "list(APPEND CMAKE_CXX_SOURCE_FILE_EXTENSIONS cc)")
project(pgAdmin3 VERSION 1.26.0)
include_directories(. include)
set(PRECOMP_FILE precomp.h)
if(NOT CROSS_COMPILE)
        include_directories(include/libssh2)
        find_package(CURL)
	   if(NOT CURL_FOUND)
	    add_compile_definitions(NO_WXJSON_GIT)
	   else()
	    include_directories(${CURL_INCLUDE_DIR})
	    link_libraries(${CURL_LIBRARIES})
        endif()
	find_package(LibXslt REQUIRED)
	if(NOT LIBXSLT_FOUND)
	    message(SEND_ERROR "Failed to find LibXslt")
	    return()
	else()
	    include_directories(${LIBXSLT_INCLUDE_DIR})
	    link_libraries(${LIBXSLT_LIBRARIES})
            message(STATUS "path libxslt = ${LIBXSLT_LIBRARIES}")
	endif()
	find_package(LibXml2 REQUIRED)
	if(NOT LIBXML2_FOUND)
	    message(SEND_ERROR "Failed to find LibXml2")
	    return()
	else()
	    include_directories(${LIBXML2_INCLUDE_DIR})
	    link_libraries(${LIBXML2_LIBRARIES})
            message(STATUS "path libxml = ${LIBXML2_LIBRARIES}")
	endif()
	find_package(wxWidgets 3.2 REQUIRED stc aui base net core xrc html xml)
	if(NOT wxWidgets_FOUND)
	    message(SEND_ERROR "Failed to find wxWidgets ")
	    return()
	else()
	    include(${wxWidgets_USE_FILE})
	    include_directories(${wxWidgets_INCLUDE_DIRS})
	    link_libraries(${wxWidgets_LIBRARIES})
            message(STATUS "path wxwidgets = ${wxWidgets_LIBRARIES} inclure = ${wxWidgets_INCLUDE_DIRS}")
	endif()

	find_package(PostgreSQL REQUIRED)
	if(NOT PostgreSQL_FOUND)
	    message(SEND_ERROR "Failed to find PostgreSql")
	    return()
	else()
	    include_directories(${PostgreSQL_INCLUDE_DIRS})
	    include_directories(${PostgreSQL_INCLUDE_DIRS}/server)
	    link_libraries(${PostgreSQL_LIBRARIES})
            message(STATUS "path postgresql= ${PostgreSQL_LIBRARIES}")
	endif()
     add_compile_definitions(WX_PRECOMP)
else()
#windows

endif()


add_compile_definitions(wxUSE_UNICODE)
add_compile_definitions(EMBED_XRC)
add_compile_options(-Wno-narrowing)
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

add_executable(${PROJECT_NAME} )
target_sources(${PROJECT_NAME} PUBLIC pgAdmin3.cpp ${SOURCES})
if(CROSS_COMPILE STREQUAL "Windows")
  ENABLE_LANGUAGE(RC)
  set(CMAKE_RC_COMPILER_INIT windres)
  set(RC_FILE pgAdmin3.rc)
  SET(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> -v -O coff -I ${WXWIN}/include <DEFINES> -i <SOURCE> -o <OBJECT>")
  target_sources(${PROJECT_NAME} PRIVATE ${RC_FILE})
  message(STATUS "Use RC file ${RC_FILE}")
endif()

if (PRECOMP_FILE)
   target_precompile_headers(${PROJECT_NAME} PUBLIC include/${PRECOMP_FILE})
   message(STATUS "Precompile header include/${PRECOMP_FILE}")
endif()
if(CROSS_COMPILE STREQUAL "Windows")
     #add_compile_definitions(_CRT_SECURE_NO_DEPRECATE=1 NDEBUG WIN32 _WINDOWS __WINDOWS__ __WIN95__ __WIN32__ WINVER=0x0500 STRICT __WXMSW__ WXUSINGDLL wxUSE_UNICODE=1 PG_SSL)
        add_compile_definitions(NDEBUG WIN32 WXPRECOMP WINVER=0x0600 STRICT wxUSE_UNICODE=1 PG_SSL)
	set(WXWIN /home/sergey/k/wxMSW-3.2.2 )
	set(PGDIR /home/sergey/k/pg10 )
	set(PGBUILD /home/sergey/k )
	#set(/home/sergey/k/openssl )
	#set(PROJECTDIR /home/sergey/k/openssl )
	include_directories(${WXWIN}/include ${PGDIR}/include ${PGDIR}/include/server )
	include_directories(${PGBUILD}/libxml2/include ${PGBUILD}/libxslt/include ${PGBUILD}/iconv/include  )
#	include_directories(${PGBUILD}/openssl/include 
#	include_directories( ./include/libssh2 ./include/libssh2/Win32 )
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

