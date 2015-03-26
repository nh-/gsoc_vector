
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(gsoc_vector_test main.cpp)
if(${WIN32})
	target_link_libraries(gsoc_vector_test ${GTEST_LIBRARIES})
else()
	target_link_libraries(gsoc_vector_test ${GTEST_LIBRARIES} pthread)
endif()
add_test(gsoc_vector_tests gsoc_vector_test)

