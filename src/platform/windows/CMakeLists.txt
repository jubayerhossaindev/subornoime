# Windows production targets: the TSF text-service DLL and the settings tray
# application. Requires the MSVC v143 toolset.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR NOT MSVC_TOOLSET_VERSION STREQUAL "143")
    message(FATAL_ERROR
        "The Windows production target requires the MSVC v143 toolset; "
        "MinGW, GCC, clang-cl, and other toolsets are unsupported.")
endif()

find_package(wil CONFIG REQUIRED)
enable_language(RC)

# The InnoSetup script reads the icon from the top-level build directory
# (SetupIconFile={#MyBuildDir}\suborno.ico) — keep that contract intact.
set(SUBORNO_ICON_FILE "${CMAKE_BINARY_DIR}/suborno.ico")
configure_file(
    "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/suborno.ico"
    "${SUBORNO_ICON_FILE}"
    COPYONLY
)
configure_file(
    "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_version.hpp"
    @ONLY
)
# The installer script includes this generated define file; keep it at the
# top-level build directory — .github/workflows/release.yml reads it from
# build/<preset>/suborno_version.iss.inc.
file(WRITE "${CMAKE_BINARY_DIR}/suborno_version.iss.inc"
    "#define MyAppVersion \"${PROJECT_VERSION}\"\n")

set(SUBORNO_FILE_DESCRIPTION "Suborno Bengali Input Method")
set(SUBORNO_INTERNAL_NAME "Suborno")
set(SUBORNO_ORIGINAL_FILENAME "Suborno.exe")
set(SUBORNO_FILE_TYPE VFT_APP)
configure_file(
    "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/suborno.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_app.rc"
    @ONLY
)
set(SUBORNO_FILE_TYPE VFT_DLL)
set(SUBORNO_ORIGINAL_FILENAME "suborno_windows_tsf.dll")
configure_file(
    "${PROJECT_SOURCE_DIR}/src/platform/windows/resources/suborno.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_tsf.rc"
    @ONLY
)

add_library(suborno_windows_tsf SHARED
    app_paths.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_tsf.rc"
    tsf/suborno_windows_tsf.def
    tsf/module_smoke.cpp
    tsf/guids.cpp
    tsf/compartment.cpp
    tsf/display_attribute_info.cpp
    tsf/text_edit_session.cpp
    tsf/candidate_list_ui_element.cpp
    tsf/text_service.cpp
    tsf/class_factory.cpp
    tsf/dll_main.cpp
    registration/com_registration.cpp
    registration/tsf_profiles.cpp
)
target_include_directories(suborno_windows_tsf PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_compile_definitions(suborno_windows_tsf PRIVATE SUBORNO_WINDOWS_TSF_EXPORTS)
target_link_libraries(suborno_windows_tsf PRIVATE
    suborno_core
    WIL::WIL
    Advapi32.lib
    Ole32.lib
    OleAut32.lib
    Shell32.lib
    Shlwapi.lib
    User32.lib
    Uuid.lib
)
suborno_configure_target(suborno_windows_tsf)
# The five Dll* exports stay non-PRIVATE so the in-process lifecycle smoke
# test (suborno_windows_tests) can import them directly from the import
# library; MSVC flags each of them with LNK4104 unless marked PRIVATE in the
# .def file, so silence that one linker warning for this target.
if(MSVC)
    target_link_options(suborno_windows_tsf PRIVATE "/IGNORE:4104")
endif()
if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
    target_compile_options(suborno_windows_tsf PRIVATE /analyze:external-)
    suborno_mark_dependency_headers_external(suborno_windows_tsf WIL::WIL)
endif()

add_executable(suborno_app WIN32
    "${CMAKE_CURRENT_BINARY_DIR}/suborno_app.rc"
    app_paths.cpp
    app/application.cpp
    app/main.cpp
    app/menu_popup.cpp
    app/osk_window.cpp
    app/toast_window.cpp
    app/topbar.cpp
    app/ui_accessibility.cpp
    tsf/guids.cpp
)
set_target_properties(suborno_app PROPERTIES OUTPUT_NAME "Suborno")
target_include_directories(suborno_app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(suborno_app PRIVATE
    Advapi32.lib
    # The topbar's native tooltips use InitCommonControlsEx (common controls).
    Comctl32.lib
    Gdi32.lib
    Ole32.lib
    Shell32.lib
    suborno_core
    User32.lib
    Uuid.lib
)
suborno_configure_target(suborno_app)

# Optional zero-dependency standalone typing host (SubornoStandalone.exe):
# WH_KEYBOARD_LL hook + SendInput Unicode injection driven by the same
# suborno_core engine — no Bengali language pack, no language-bar switching,
# no Win+Space. Turn OFF to build only the TSF pipeline.
option(SUBORNO_BUILD_STANDALONE "Build the standalone zero-dependency typing host" ON)
if(SUBORNO_BUILD_STANDALONE)
    add_subdirectory(standalone)
endif()
