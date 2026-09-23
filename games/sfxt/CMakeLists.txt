# Street Fighter X Tekken: the GFWL build takes the SDK dll as is, the Steam build also needs a
# steam_api.dll that speaks SDK 1.19 to the exe and the SDK version to xlive.dll.

add_library(sfxt_steam_api SHARED src/steam_api_shim.cpp src/steam_api_shim.def)
set_target_properties(sfxt_steam_api PROPERTIES
    OUTPUT_NAME "steam_api"
    PREFIX ""
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    FOLDER "games/sfxt"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<0:>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<0:>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<0:>"
    PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin/$<0:>"
)
target_include_directories(sfxt_steam_api PRIVATE "${XLS_STEAM_INCLUDE}")
target_compile_definitions(sfxt_steam_api PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(sfxt_steam_api PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/wd4201>
    $<$<CXX_COMPILER_ID:MSVC>:/Zi>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2>
)
target_link_options(sfxt_steam_api PRIVATE /DEBUG $<$<CONFIG:Release>:/OPT:REF> $<$<CONFIG:Release>:/OPT:ICF>)

# The forwarders in the .def name steam_api_real.dll, which is the SDK redistributable renamed.
set(sfxt_steam_api_real "$<TARGET_FILE_DIR:sfxt_steam_api>/steam_api_real.dll")
add_custom_command(TARGET sfxt_steam_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${XLS_STEAM_API_DLL}" "${sfxt_steam_api_real}"
    COMMENT "Placing the SDK steam_api.dll as steam_api_real.dll"
    VERBATIM
)

xls_add_standard_release(sfxt)

xls_add_release(sfxt-steam
    FILES
        "$<TARGET_FILE:xlive>"
        "$<TARGET_FILE:sfxt_steam_api>"
        "${sfxt_steam_api_real}"
        "${CMAKE_CURRENT_SOURCE_DIR}/xlive_steamworks.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/import-save.ps1"
    DEPENDS xlive sfxt_steam_api
)
