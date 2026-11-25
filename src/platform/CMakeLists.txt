# Platform adapters: only the directory matching the host OS is built.
if(UNIX AND NOT APPLE)
    add_subdirectory(linux)
elseif(APPLE)
    add_subdirectory(macos)
endif()

if(WIN32)
    add_subdirectory(windows)
endif()
