include_guard(GLOBAL)

# Shared build helpers. Every target defined by a per-directory build file
# calls suborno_configure_target; test targets call suborno_configure_test_target.

function(suborno_apply_build_options target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "suborno_apply_build_options: '${target}' is not a target")
    endif()

    if(MSVC)
        # Compiler security & build hardening (docs/engineering/
        # SECURITY_STANDARDS.md, docs/engineering/SANITIZER_CONFIGURATION.md):
        #   /W4 /utf-8      full project warning coverage, UTF-8 source charset
        #   /permissive-    strict two-phase C++ conformance (C++20 policy)
        #   /sdl            additional Security Development Lifecycle checks
        #   /GS             stack buffer overrun detection
        #   /guard:cf       control-flow guard at compile time; the matching
        #                   /GUARD:CF link option is added below
        target_compile_options("${target}" PRIVATE /W4 /utf-8 /permissive- /sdl /GS /guard:cf)
        target_link_options("${target}" PRIVATE /GUARD:CF)
        # The codebase calls the wide-character Windows API explicitly, but SDK
        # resource macros (IDC_ARROW, MAKEINTRESOURCE, ...) follow the UNICODE
        # macro: without it they expand to ANSI types and LoadCursorW-style
        # calls fail with C2664. Define it for every target so all
        # translation units agree. NOMINMAX is required for the same reason:
        # the SDK's min/max macros would otherwise rewrite every std::min /
        # std::max call (C2589 "illegal token on right side of '::'"), and no
        # source relies on the macro forms.
        target_compile_definitions("${target}" PRIVATE UNICODE _UNICODE NOMINMAX)
        if(SUBORNO_ENABLE_MSVC_ANALYSIS)
            target_compile_options("${target}" PRIVATE /analyze)
            # Third-party header analysis policy (C28182 disposition, task
            # MSVC-WARNINGS-REPAIR-02). MSVC Code Analysis scans third-party
            # headers by default (the documented default of /analyze is to
            # analyze external headers), and WIL 1.0.260126.7 from the vcpkg
            # manifest emits three C28182 "Dereferencing NULL pointer"
            # diagnostics from wil/resource.h's event-watcher state allocation
            # (create_take_hevent_ownership). Investigated against the
            # SHA512/tree-verified installed copy of that exact version: the
            # allocation result IS null-checked (RETURN_IF_NULL_ALLOC) before
            # every flagged member dereference, SUBORNO never calls a WIL
            # nothrow-allocating API from any call site (the project's complete
            # WIL surface is com_ptr / unique_hmodule / unique_bstr, and every
            # project-owned new(std::nothrow) site checks for null), and
            # microsoft/wil#610 (closed 2026-01-20) confirms the analyzer
            # mishandles the new(std::nothrow) braced-init pattern — the object
            # is never constructed when the allocation fails, so the guarded
            # dereference cannot receive that null. Disposition: verified
            # third-party analysis false positive, not a project usage defect
            # and not a genuine WIL bug. Rather than editing dependency sources
            # or bumping dependency versions, the build distinguishes
            # third-party headers from project-owned headers, scoped to exactly
            # the vcpkg-installed include root:
            #   /external:W4         keeps every regular compiler warning from
            #                        those headers at the current /W4 level
            #                        (only static analysis changes; MSVC also
            #                        ignores the other /external options with
            #                        command-line warning D9007 unless an
            #                        /external:Wn is present),
            #   /external:I<root>    designates that one directory tree (the
            #                        vcpkg dependencies, e.g.
            #                        wil/resource.h) as external headers,
            #   /analyze:external-   skips analysis for external headers only.
            # Project-owned sources and headers (src/, include/suborno/) keep
            # full /W4 + /analyze coverage: every diagnostic whose reported
            # location is a project file is still produced, and no rule is
            # disabled for project code. Evidence: docs/reports/
            # MSVC_WARNING_ELIMINATION_01_REPORT.md.
            if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET
               AND EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")
                target_compile_options("${target}" PRIVATE
                    /external:W4
                    "/external:I${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include"
                    /analyze:external-)
            endif()
        endif()
        if(SUBORNO_ENABLE_SANITIZERS)
            target_compile_options("${target}" PRIVATE /fsanitize=address)
            target_link_options("${target}" PRIVATE /INCREMENTAL:NO)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options("${target}" PRIVATE -Wall -Wextra -Wpedantic)
        if(SUBORNO_ENABLE_SANITIZERS)
            target_compile_options("${target}" PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer)
            target_link_options("${target}" PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

function(suborno_configure_target target_name)
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    suborno_apply_build_options(${target_name})
endfunction()

# Code analysis stays ON for test targets, exactly like production code. The
# single exception is C6326, forced-included (cl /FI) into MSVC test
# translation units only: googletest 1.18's own assertion helper
# AssertionResultExpectation::operator bool() compares
# `converted == expected_result` (bool == bool), and the MSVC analyzer
# attributes a constant-vs-constant C6326 to every EXPECT_*/ASSERT_*
# expansion line — ~1000 false positives per build, a class Microsoft
# tracks as "False positive with static code analysis and GoogleTest"
# (Developer Community #10978073, Oct 2025). Every other analysis rule and
# every compiler warning remains fully active on test code.
function(suborno_configure_test_target target_name)
    suborno_configure_target(${target_name})
    if(MSVC AND SUBORNO_ENABLE_MSVC_ANALYSIS)
        target_compile_options("${target_name}" PRIVATE
            /analyze:external-
            "/FI${PROJECT_SOURCE_DIR}/cmake/msvc_test_c6326_suppress.h")
    endif()
endfunction()

function(suborno_mark_dependency_headers_external target_name dependency_target)
    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR
            "suborno_mark_dependency_headers_external: '${target_name}' is not a target")
    endif()
    if(NOT TARGET "${dependency_target}")
        message(FATAL_ERROR
            "suborno_mark_dependency_headers_external: dependency target "
            "'${dependency_target}' does not exist in this directory scope. "
            "IMPORTED targets created by find_package() are visible only in "
            "the directory where they were found and its subdirectories: run "
            "find_package() for '${dependency_target}' in this directory (or "
            "a parent) before marking its headers SYSTEM.")
    endif()
    get_target_property(dependency_include_directories
        "${dependency_target}" INTERFACE_INCLUDE_DIRECTORIES)
    if(dependency_include_directories)
        target_include_directories("${target_name}" SYSTEM PRIVATE
            ${dependency_include_directories})
    endif()
endfunction()
