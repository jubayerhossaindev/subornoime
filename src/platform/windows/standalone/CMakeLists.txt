# Zero Bengali Language Pack Dependency — standalone typing host
# (SubornoStandalone.exe). WH_KEYBOARD_LL hook + SendInput Unicode injection
# on top of the SAME portable engine the TSF IME uses (suborno_core). No TSF
# registration, no language bar, no Win+Space, no Bengali language pack:
# install, run, type Bangla anywhere — the Smart/Classic usability model.
#
# The target is additive to the MSVC-WARNINGS-REPAIR-02 pipeline; turn the
# SUBORNO_BUILD_STANDALONE option off to build only the original pipeline.

set(SUBORNO_FILE_DESCRIPTION "Suborno Standalone Bangla Typing")
set(SUBORNO_INTERNAL_NAME "SubornoStandalone")
set(SUBORNO_ORIGINAL_FILENAME "SubornoStandalone.exe")
set(SUBORNO_FILE_TYPE VFT_APP)
configure_file(
    "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/suborno.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_standalone.rc"
    @ONLY
)

add_executable(suborno_standalone WIN32
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_standalone.rc"
    standalone_main.cpp
    keyboard_hook.cpp
    unicode_injector.cpp
    preview_window.cpp
    font_loader.cpp
    standalone_tray.cpp
    # The standalone host resolves the same user data paths (settings.ini,
    # dictionary) as the TSF IME and the desktop shell, so both shells stay
    # in sync with one engine configuration.
    "${CMAKE_CURRENT_SOURCE_DIR}/../app_paths.cpp"
)
set_target_properties(suborno_standalone PROPERTIES OUTPUT_NAME "SubornoStandalone")
target_include_directories(suborno_standalone PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(suborno_standalone PRIVATE
    suborno_core
    Advapi32.lib
    Comctl32.lib
    Gdi32.lib
    Ole32.lib
    Shell32.lib
    User32.lib
    Uuid.lib
)
suborno_configure_target(suborno_standalone)

# Ship the bundled OFL Bangla fonts next to the executable so the dev-run
# binary gets the same FR_PRIVATE brand face the installed layout uses
# (install_standalone.cmd copies assets\fonts -> %LOCALAPPDATA%\Suborno\fonts).
# Only for Windows targets; other generators never build this target.
add_custom_command(TARGET suborno_standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets/fonts" "$<TARGET_FILE_DIR:suborno_standalone>/fonts"
    COMMENT "Copying bundled Bangla fonts (assets/fonts) next to SubornoStandalone.exe"
    VERBATIM
)
