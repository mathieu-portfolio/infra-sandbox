cmake_minimum_required(VERSION 3.20)

project(infra_sandbox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INFRA_BUILD_APP "Build the raylib visualization app" ON)
option(INFRA_BUILD_TESTS "Build unit tests" ON)
option(INFRA_FETCH_RAYLIB "Fetch raylib with CMake if it is not installed" ON)

if(INFRA_BUILD_APP)
    find_package(raylib CONFIG QUIET)
endif()

if(INFRA_BUILD_APP AND NOT raylib_FOUND AND INFRA_FETCH_RAYLIB)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
    )
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

file(GLOB_RECURSE INFRA_CORE_SOURCES CONFIGURE_DEPENDS
    src/core/*.cpp
)

add_library(infra_core
    ${INFRA_CORE_SOURCES}
)

target_include_directories(infra_core PUBLIC src)

file(GLOB_RECURSE INFRA_SIMULATION_SOURCES CONFIGURE_DEPENDS
    src/content/*.cpp
    src/gameplay/*.cpp
    src/simulation/*.cpp
)

add_library(infra_simulation
    ${INFRA_SIMULATION_SOURCES}
)

target_include_directories(infra_simulation PUBLIC src)
target_compile_definitions(infra_simulation PUBLIC INFRA_CONTENT_DIR="${CMAKE_SOURCE_DIR}/content")
target_link_libraries(infra_simulation PUBLIC infra_core)

file(GLOB_RECURSE INFRA_APP_SOURCES CONFIGURE_DEPENDS
    src/app/*.cpp
    src/control/*.cpp
    src/input/*.cpp
    src/rendering/*.cpp
    src/ui/*.cpp
)

if(INFRA_BUILD_APP)
    add_executable(infra_sandbox
        src/main.cpp
        ${INFRA_APP_SOURCES}
    )

    target_include_directories(infra_sandbox PRIVATE src)
    target_link_libraries(infra_sandbox PRIVATE infra_simulation raylib)
endif()

if(MSVC)
    target_compile_options(infra_core PRIVATE /W4 /FS)
    target_compile_options(infra_simulation PRIVATE /W4 /FS)
    if(INFRA_BUILD_APP)
        target_compile_options(infra_sandbox PRIVATE /W4 /FS)
    endif()
else()
    target_compile_options(infra_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(infra_simulation PRIVATE -Wall -Wextra -Wpedantic)
    if(INFRA_BUILD_APP)
        target_compile_options(infra_sandbox PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(INFRA_BUILD_TESTS)
    include(CTest)
    include(FetchContent)

    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    file(GLOB_RECURSE INFRA_TEST_SOURCES CONFIGURE_DEPENDS
        tests/*.cpp
    )

    add_executable(infra_sandbox_tests
        ${INFRA_TEST_SOURCES}
    )

    target_link_libraries(infra_sandbox_tests PRIVATE infra_simulation GTest::gtest_main)

    if(MSVC)
        target_compile_options(infra_sandbox_tests PRIVATE /W4 /FS)
    else()
        target_compile_options(infra_sandbox_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(GoogleTest)
    gtest_discover_tests(infra_sandbox_tests)
endif()
