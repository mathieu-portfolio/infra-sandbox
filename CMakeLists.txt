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

add_library(infra_simulation
    src/simulation/InfrastructureGraph.cpp
    src/simulation/Geography.cpp
    src/simulation/Link.cpp
    src/simulation/Mechanics.cpp
    src/simulation/Metrics.cpp
    src/simulation/Node.cpp
    src/simulation/NodeDefinition.cpp
    src/simulation/PressureAnalysis.cpp
    src/simulation/RuntimeSystems.cpp
    src/simulation/SimulationConfig.cpp
    src/simulation/SimulationLayer.cpp
    src/simulation/Simulation.cpp
    src/simulation/SimulationTime.cpp
    src/simulation/TopologyMutation.cpp
    src/gameplay/Event.cpp
    src/gameplay/Scenario.cpp
    src/gameplay/ScenarioManager.cpp
)

target_include_directories(infra_simulation PUBLIC src)

if(INFRA_BUILD_APP)
    add_executable(infra_sandbox
        src/main.cpp
        src/app/Application.cpp
        src/control/InterventionController.cpp
        src/control/OverlayController.cpp
        src/control/SelectionController.cpp
        src/control/SimulationController.cpp
        src/control/UiController.cpp
        src/input/InputManager.cpp
        src/rendering/CameraController.cpp
        src/rendering/GeoLayoutSystem.cpp
        src/rendering/MapRenderer.cpp
        src/rendering/Renderer.cpp
        src/rendering/RenderPrimitives.cpp
        src/ui/ActionPanelModel.cpp
        src/ui/DebugPanel.cpp
        src/ui/HudPanel.cpp
        src/ui/InterventionPanel.cpp
        src/ui/MetricsPanel.cpp
        src/ui/OverlayController.cpp
        src/ui/SelectionPanel.cpp
        src/ui/TimelinePanel.cpp
        src/ui/UiManager.cpp
        src/ui/UiTypes.cpp
    )

    target_include_directories(infra_sandbox PRIVATE src)
    target_link_libraries(infra_sandbox PRIVATE infra_simulation raylib)
endif()

if(MSVC)
    target_compile_options(infra_simulation PRIVATE /W4)
    if(INFRA_BUILD_APP)
        target_compile_options(infra_sandbox PRIVATE /W4)
    endif()
else()
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

    add_executable(infra_sandbox_tests
        tests/simulation/SimulationTests.cpp
    )
    target_link_libraries(infra_sandbox_tests PRIVATE infra_simulation GTest::gtest_main)

    if(MSVC)
        target_compile_options(infra_sandbox_tests PRIVATE /W4)
    else()
        target_compile_options(infra_sandbox_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    include(GoogleTest)
    gtest_discover_tests(infra_sandbox_tests)
endif()
