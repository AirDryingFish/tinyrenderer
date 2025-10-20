cmake_minimum_required(VERSION 3.12...3.26)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release default")
endif()

project(tinyrenderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

option(iwyu "Run include-what-you-use")
if(iwyu)
  find_program(IWYU_EXE NAMES include-what-you-use REQUIRED)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXE})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|Intel")
  add_compile_options(-Wall)
endif()

find_package(OpenMP COMPONENTS CXX)

# set(SOURCES main.cpp model.cpp tgaimage.cpp GL.cpp)
# file(GLOB SRC_FILES "*.cpp")
# add_executable(tinyrenderer ${SRC_FILES})
# add_executable(${PROJECT_NAME} ${SOURCES})

# file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/*.cpp")
# add_executable(tinyrenderer ${SRC_FILES})

file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/*.cpp"
)

# 过滤掉 build 目录
list(FILTER SRC_FILES EXCLUDE REGEX ".*/build/.*")

add_executable(tinyrenderer ${SRC_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>)

file(GENERATE OUTPUT .gitignore CONTENT "*")
