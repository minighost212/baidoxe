cmake_minimum_required(VERSION 3.16)
project(AEON_Parking_Simulation VERSION 3.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AEON_BUILD_APP "Build the OpenGL desktop application" ON)
include(CTest)

if(AEON_BUILD_APP)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glm REQUIRED)

    add_executable(aeon_parking
        parking_main.cpp
        camera.cpp
        guard_station.cpp
        aeon_layout.h
        car_model.h
        camera.h
        guard_station.h
        renderer.h
        parking_world.h
        scene_view.h
        obj_model.h
    )
    target_link_libraries(aeon_parking PRIVATE OpenGL::GL glfw GLEW::GLEW glm::glm)

    if(MSVC)
        target_compile_options(aeon_parking PRIVATE /W4 /permissive-)
    else()
        target_compile_options(aeon_parking PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(BUILD_TESTING)
    # The world-model tests need only vec2/vec3 operations. A tiny GLM-compatible
    # test stub keeps CTest runnable on machines without the OpenGL SDK. The
    # desktop application always links the real GLM package above.
    add_executable(parking_world_tests tests/parking_world_tests.cpp)
    target_include_directories(parking_world_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/stubs")
    if(MSVC)
        target_compile_options(parking_world_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(parking_world_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME parking_world_tests COMMAND parking_world_tests)

    add_executable(debug_full_state tests/debug_full_state.cpp)
    target_include_directories(debug_full_state PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/stubs")

    add_executable(debug_stuck tests/debug_stuck.cpp)
    target_include_directories(debug_stuck PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/stubs")
endif()

# vios.obj is retained as the supplied reference asset. The current renderer
# builds the procedural Toyota Vios mesh from car_model.h at runtime.
