cmake_minimum_required(VERSION 3.25)
project(MW3GamepadFix VERSION 0.8.8 LANGUAGES C CXX RC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This prototype targets Windows x64 with MSVC.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(/W4 /permissive- /utf-8 /EHsc)
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

set(GAMEINPUT_SDK "${CMAKE_CURRENT_SOURCE_DIR}/.deps/Microsoft.GameInput.3.5.283" CACHE PATH "Pinned GameInput SDK")
if(NOT EXISTS "${GAMEINPUT_SDK}/native/lib/x64/GameInput.lib")
    message(FATAL_ERROR "Run python scripts/restore_gameinput.py to restore Microsoft.GameInput 3.5.283.")
endif()
add_library(Microsoft::GameInput STATIC IMPORTED)
set_target_properties(Microsoft::GameInput PROPERTIES
    IMPORTED_LOCATION "${GAMEINPUT_SDK}/native/lib/x64/GameInput.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${GAMEINPUT_SDK}/native/include"
    INTERFACE_LINK_LIBRARIES "advapi32;version")
add_library(Microsoft::XInput INTERFACE IMPORTED)
set_target_properties(Microsoft::XInput PROPERTIES INTERFACE_LINK_LIBRARIES "xinput")

add_library(mw3gf_core STATIC
    src/core/input_monitor.cpp
    src/core/status_text.cpp
    src/core/menu_adapter.cpp
    src/core/cinematic_skip.cpp
    src/core/menu_footer.cpp
    src/core/prompt_layout.cpp
    src/core/gameplay_adapter.cpp
    src/core/gameplay_prompts.cpp
    src/core/stick_camera.cpp
    src/core/use_policy.cpp
    src/core/prone_prompt.cpp
    src/core/mortar_prompt.cpp
)
target_include_directories(mw3gf_core PUBLIC include)
add_library(mw3gf_input STATIC
    src/input/xinput_backend.cpp
    src/input/gameinput_backend.cpp
    src/input/backend_factory.cpp
)
target_link_libraries(mw3gf_input PUBLIC mw3gf_core PRIVATE Microsoft::XInput Microsoft::GameInput)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps/minhook-1.3.4/CMakeLists.txt")
    message(FATAL_ERROR "Run python scripts/restore_minhook.py to restore MinHook 1.3.4.")
endif()
add_subdirectory(.deps/minhook-1.3.4 EXCLUDE_FROM_ALL)
add_library(mw3gf_platform STATIC
    src/platform/windows.cpp
)
target_link_libraries(mw3gf_platform PUBLIC mw3gf_core PRIVATE bcrypt)
set(PROMPT_ASSET_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/button-icons.bin")
set(PS3_PROMPT_ASSET_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/button-icons-ps3.bin")
configure_file(src/plugin/prompts.rc.in "${CMAKE_CURRENT_BINARY_DIR}/prompts.rc" @ONLY)
set(MW3GF_GAME_SOURCES
    src/game/hook_set.cpp
    src/game/runtime.cpp
    src/game/native_hooks.cpp
    src/game/button_icons.cpp
    src/game/settings.cpp
    src/game/engine_api.cpp
    src/game/native_memory.cpp
    src/game/runtime_log.cpp
    src/game/input_session.cpp
    src/game/runtime_context.cpp
    src/game/prompt_controller.cpp
    src/game/hud_prompt.cpp
    src/game/gameplay_controller.cpp
    src/game/remote_control.cpp
    src/game/use_hold.cpp
    src/game/menu_controller.cpp
    src/game/menu_actions.cpp
)
add_library(mw3gf_game OBJECT ${MW3GF_GAME_SOURCES})
target_link_libraries(mw3gf_game PRIVATE mw3gf_input mw3gf_platform minhook user32 d3d9)
add_library(MW3GamepadFixASI SHARED
    src/plugin/entry.cpp
    $<TARGET_OBJECTS:mw3gf_game>
    "${CMAKE_CURRENT_BINARY_DIR}/prompts.rc")
target_link_libraries(MW3GamepadFixASI PRIVATE mw3gf_input mw3gf_platform minhook user32 d3d9)
target_compile_definitions(MW3GamepadFixASI PRIVATE MW3GF_ASI=1)
set_target_properties(MW3GamepadFixASI PROPERTIES OUTPUT_NAME MW3GamepadFix ARCHIVE_OUTPUT_NAME MW3GamepadFixASI SUFFIX .asi)
add_custom_command(TARGET MW3GamepadFixASI POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/MW3GamepadFix.ini" "$<TARGET_FILE_DIR:MW3GamepadFixASI>/MW3GamepadFix.ini")

include(CTest)
if(BUILD_TESTING)
    function(mw3gf_add_test target name)
        cmake_parse_arguments(TEST "" "" "SOURCES;LIBRARIES;ARGS" ${ARGN})
        add_executable(${target} ${TEST_SOURCES})
        target_include_directories(${target} PRIVATE include)
        target_compile_options(${target} PRIVATE /WX)
        if(TEST_LIBRARIES)
            target_link_libraries(${target} PRIVATE ${TEST_LIBRARIES})
        endif()
        add_test(NAME ${name} COMMAND ${target} ${TEST_ARGS})
    endfunction()

    mw3gf_add_test(mw3gf_settings_tests settings
        SOURCES tests/settings_tests.cpp src/game/settings.cpp)
    mw3gf_add_test(mw3gf_menu_actions_tests menu_actions
        SOURCES tests/menu_actions_tests.cpp src/game/menu_actions.cpp)
    mw3gf_add_test(mw3gf_hook_lifecycle_tests hook_lifecycle
        SOURCES tests/hook_lifecycle_tests.cpp src/game/hook_set.cpp
        LIBRARIES minhook)
    mw3gf_add_test(mw3gf_native_memory_tests native_memory
        SOURCES tests/native_memory_tests.cpp src/game/native_memory.cpp)
    mw3gf_add_test(mw3gf_core_tests input_monitor
        SOURCES tests/input_monitor_tests.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_menu_tests menu_adapter
        SOURCES tests/menu_adapter_tests.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_cinematic_tests cinematic_skip
        SOURCES tests/cinematic_skip_tests.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_layout_tests prompt_layout
        SOURCES tests/prompt_layout_tests.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_asi_tests asi_bootstrap
        SOURCES tests/asi_bootstrap_tests.cpp
        ARGS $<TARGET_FILE:MW3GamepadFixASI>)
    mw3gf_add_test(mw3gf_gameplay_tests gameplay_adapter
        SOURCES tests/gameplay_adapter_tests.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_hud_prompt_tests hud_prompt
        SOURCES tests/hud_prompt_tests.cpp src/game/hud_prompt.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_camera_tests stick_camera
        SOURCES tests/stick_camera_tests.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_remote_tests remote_control
        SOURCES tests/remote_control_tests.cpp src/game/remote_control.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_use_hold_tests use_hold
        SOURCES tests/use_hold_tests.cpp src/game/use_hold.cpp src/game/native_memory.cpp
        LIBRARIES mw3gf_core)
    mw3gf_add_test(mw3gf_camera_hook_tests camera_hooks
        SOURCES tests/camera_hook_tests.cpp
        LIBRARIES minhook)
    mw3gf_add_test(mw3gf_independent_prompt_tests independent_prompts_x360
        SOURCES tests/independent_prompt_tests.cpp src/game/button_icons.cpp src/game/native_memory.cpp
        LIBRARIES mw3gf_core d3d9 user32
        ARGS $<TARGET_FILE:MW3GamepadFixASI> x360)
    add_test(NAME independent_prompts_ps3
        COMMAND mw3gf_independent_prompt_tests $<TARGET_FILE:MW3GamepadFixASI> ps3)

endif()
foreach(target mw3gf_game mw3gf_core mw3gf_input mw3gf_platform MW3GamepadFixASI)
    target_compile_options(${target} PRIVATE /WX)
endforeach()
install(FILES MW3GamepadFix.ini DESTINATION .)
install(FILES README.md THIRD_PARTY_NOTICES.md DESTINATION .)
install(FILES assets/button-icons.json assets/button-icons-ps3.json DESTINATION assets)
install(DIRECTORY licenses DESTINATION .)
install(TARGETS MW3GamepadFixASI RUNTIME DESTINATION . COMPONENT asi)
