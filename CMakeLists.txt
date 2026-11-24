cmake_minimum_required(VERSION 3.21)

project(XuanquanDu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json CONFIG REQUIRED)

add_library(han_narrative STATIC
    src/narrative/narrative_state.cpp
    src/narrative/condition.cpp
    src/narrative/effects.cpp
    src/narrative/quest_system.cpp
    src/narrative/dialogue.cpp
    src/narrative/session.cpp
    src/narrative/validator.cpp
)

target_include_directories(han_narrative PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(han_narrative PUBLIC nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(han_narrative PRIVATE /W4 /utf-8 /permissive-)
else()
    target_compile_options(han_narrative PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(HAN_BUILD_TESTS "Build headless narrative tests" ON)
option(HAN_ENABLE_SFML "Build the SFML client layer (milestone M2+)" ON)
set(HAN_SFML_ROOT "" CACHE PATH "SFML 2.6 SDK root (extracted official zip); empty uses find_package")
set(HAN_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/data/narrative"
    CACHE PATH "Chapter data directory compiled into han_client")
set(HAN_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game-assets"
    CACHE PATH "Art asset root compiled into han_client")

if(HAN_BUILD_TESTS)
    enable_testing()

    add_executable(han_narrative_tests
        tests/test_main.cpp
        tests/narrative_tests.cpp
    )
    target_link_libraries(han_narrative_tests PRIVATE han_narrative)

    if(MSVC)
        target_compile_options(han_narrative_tests PRIVATE /utf-8)
    endif()

    target_compile_definitions(han_narrative_tests PRIVATE
        HAN_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/data/narrative"
    )

    add_test(NAME narrative COMMAND han_narrative_tests)
endif()

if(HAN_ENABLE_SFML)
    if(NOT HAN_SFML_ROOT AND DEFINED ENV{HAN_SFML_ROOT})
        set(HAN_SFML_ROOT "$ENV{HAN_SFML_ROOT}" CACHE PATH "SFML 2.6 SDK root (extracted official zip); empty uses find_package" FORCE)
    endif()
    if(HAN_SFML_ROOT)
        set(SFML_DIR "${HAN_SFML_ROOT}/lib/cmake/SFML")
    endif()

    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

    add_executable(han_client
        src/app/main.cpp
        src/ui/dialogue_screen.cpp
        src/ui/title_screen.cpp
    )
    target_link_libraries(han_client PRIVATE han_narrative sfml-graphics sfml-window sfml-system)

    # nlohmann_json from vcpkg contributes the whole vcpkg include dir (which
    # may hold a different SFML); keep the selected SDK's headers first.
    if(HAN_SFML_ROOT)
        target_include_directories(han_client BEFORE PRIVATE "${HAN_SFML_ROOT}/include")
    endif()

    if(MSVC)
        target_compile_options(han_client PRIVATE /W4 /utf-8 /permissive-)
    endif()

    target_compile_definitions(han_client PRIVATE
        HAN_DATA_DIR="${HAN_DATA_DIR}"
        HAN_ASSET_DIR="${HAN_ASSET_DIR}"
    )

    if(HAN_SFML_ROOT)
        add_custom_command(TARGET han_client POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${HAN_SFML_ROOT}/bin/sfml-graphics-d-2.dll,${HAN_SFML_ROOT}/bin/sfml-graphics-2.dll>"
                "$<IF:$<CONFIG:Debug>,${HAN_SFML_ROOT}/bin/sfml-window-d-2.dll,${HAN_SFML_ROOT}/bin/sfml-window-2.dll>"
                "$<IF:$<CONFIG:Debug>,${HAN_SFML_ROOT}/bin/sfml-system-d-2.dll,${HAN_SFML_ROOT}/bin/sfml-system-2.dll>"
                "$<TARGET_FILE_DIR:han_client>"
            VERBATIM)
    endif()
endif()
