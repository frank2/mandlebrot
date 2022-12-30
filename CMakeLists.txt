cmake_minimum_required(VERSION 3.10)

project(libparfait)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(PARFAIT_BUILD_SHARED "Build Parfait as a shared library." OFF)
option(TEST_PARFAIT "Enable testing for Parfait." OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/intervaltree)

file(GLOB_RECURSE HEADER_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/include/*.h ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB_RECURSE SRC_FILES FOLLOW_SYMLINKS ${PROJECT_SOURCE_DIR}/src/*.c ${PROJECT_SOURCE_DIR}/src/*.cpp)

source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE "${PROJECT_SOURCE_DIR}" PREFIX "Source Files" FILES ${SRC_FILES})

if (PARFAIT_BUILD_SHARED)
  add_compile_definitions(PARFAIT_SHARED)
  add_library(libparfait SHARED ${HEADER_FILES} ${SRC_FILES})
else()
  add_compile_definitions(PARFAIT_STATIC)
  add_library(libparfait STATIC ${HEADER_FILES} ${SRC_FILES})
endif()
  
target_include_directories(libparfait PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(libparfait PUBLIC libintervaltree)

if (TEST_PARFAIT)
  enable_testing()
  add_executable(testparfait ${PROJECT_SOURCE_DIR}/test/main.cpp ${PROJECT_SOURCE_DIR}/test/framework.hpp)
  target_link_libraries(testparfait PUBLIC libparfait)
  target_include_directories(testparfait PUBLIC
    "${PROJECT_SOURCE_DIR}/test"
  )
  add_test(NAME testparfait COMMAND testparfait)
endif()
