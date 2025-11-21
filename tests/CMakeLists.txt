
find_package(Catch2)
add_executable(test_Formatter test_Formatter.cpp ../utils/FormatterSQL.cpp)
#target_include_directories(test_Formatter PRIVATE ${CMAKE_SOURCE_DIR}/include)
#target_link_libraries(test_Formatter Catch2::Catch2)


#add_library(test_sources test_Formatter.cpp )
#target_link_libraries(test_sources Catch2::Catch2)

#add_executable(tests test.cpp)
#target_link_libraries(tests -Wl,--whole-archive test_sources -Wl,--no-whole-archive)
#target_link_libraries(tests Catch2::Catch2)

#include(CTest)
include(Catch)
catch_discover_tests(test_Formatter)
