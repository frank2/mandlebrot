cmake_minimum_required(VERSION 3.10)

project(libmandlebrot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(MANDLEBROT_BUILD_SHARED "Build Mandlebrot as a shared library." OFF)
option(TEST_MANDLEBROT "Enable testing for Mandlebrot." OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/intervaltree)

file(GLOB_RECURSE HEADER_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

if (MANDLEBROT_BUILD_SHARED)
  add_compile_definitions(MANDLEBROT_SHARED)
  add_library(libmandlebrot SHARED ${HEADER_FILES} ${SRC_FILES})
else()
  add_compile_definitions(MANDLEBROT_STATIC)
  add_library(libmandlebrot STATIC ${HEADER_FILES} ${SRC_FILES})
endif()
  
target_include_directories(libmandlebrot PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(libmandlebrot PUBLIC libintervaltree)

if (TEST_MANDLEBROT)
  enable_testing()
  add_executable(testmandlebrot ${PROJECT_SOURCE_DIR}/test/main.cpp ${PROJECT_SOURCE_DIR}/test/framework.hpp)
  target_link_libraries(testmandlebrot PUBLIC libmandlebrot)
  target_include_directories(testmandlebrot PUBLIC
    "${PROJECT_SOURCE_DIR}/test"
  )
  add_test(NAME testmandlebrot COMMAND testmandlebrot)
endif()
