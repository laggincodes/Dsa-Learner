cmake_minimum_required(VERSION 3.16)
project(dsa_manager_lite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(CURL REQUIRED)

set(DSA_CORE_SOURCES
    src/Algorithms.cpp
    src/DataStructures.cpp
    src/HistoryLogger.cpp
    src/BinarySearchTree.cpp
    src/Graph.cpp
    src/QuizEngine.cpp
    src/ProgressManager.cpp
    src/LearningHub.cpp
    src/IntelligenceHub.cpp
    src/Json.cpp
    src/CodeRegistry.cpp
)

add_library(dsa_core STATIC ${DSA_CORE_SOURCES})
target_include_directories(dsa_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(dsa_core PUBLIC CURL::libcurl)

add_executable(dsa_server
    src/main.cpp
    src/HttpServer.cpp
)
target_link_libraries(dsa_server PRIVATE dsa_core)
target_compile_definitions(dsa_server PRIVATE DSA_DEFAULT_WEB_DIR="${CMAKE_CURRENT_SOURCE_DIR}/web")

enable_testing()
add_executable(dsa_tests tests/test_dsa.cpp)
target_link_libraries(dsa_tests PRIVATE dsa_core)
add_test(NAME dsa_unit_tests COMMAND dsa_tests)
