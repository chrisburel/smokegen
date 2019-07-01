string(REPLACE "-Wl,--no-undefined" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_SHARED_MODULE_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
add_subdirectory(smoke)
add_subdirectory(dump)
