cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

cmake_policy(SET CMP0091 NEW) # CMAKE_MSVC_RUNTIME_LIBRARY

project(
    xlive-steamworks-games
    VERSION 0.1.0
    DESCRIPTION "Per-title builds of the xlive-steamworks SDK"
    LANGUAGES C CXX
)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Use a separate build directory.")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Everything here ships next to a game exe, so nothing may need a Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# --- The SDK ----------------------------------------------------------------------------------------
set(XLS_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk" CACHE PATH "xlive-steamworks SDK checkout (the submodule by default)")
if(NOT EXISTS "${XLS_SDK_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "xlive-steamworks SDK not found at '${XLS_SDK_DIR}'. Run 'git submodule update --init' or pass -DXLS_SDK_DIR.")
endif()
add_subdirectory("${XLS_SDK_DIR}" sdk)

# The SDK exports STEAMWORKS_SDK_DIR to the cache, the 1.65 redistributable and headers come from it.
set(XLS_STEAM_API_DLL "${STEAMWORKS_SDK_DIR}/redistributable_bin/steam_api.dll")
set(XLS_STEAM_INCLUDE "${STEAMWORKS_SDK_DIR}/public")

set(XLS_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/release" CACHE PATH "Where release folders are assembled")

# xls_add_release(<name> FILES <file>... [DEPENDS <target>...])
# Assembles release/<name>/ from the listed files after the targets they come from are built.
function(xls_add_release name)
    cmake_parse_arguments(REL "" "" "FILES;DEPENDS" ${ARGN})
    set(out "${XLS_RELEASE_DIR}/${name}")
    add_custom_target(release_${name} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${out}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${REL_FILES} "${out}"
        DEPENDS ${REL_DEPENDS}
        COMMENT "Assembling release/${name}"
        VERBATIM
    )
    set_target_properties(release_${name} PROPERTIES FOLDER "release")
endfunction()

add_subdirectory(games/sfxt)
