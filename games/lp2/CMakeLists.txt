# Lost Planet 2: a plain GFWL title. It ships no steam_api.dll of its own, so the release is the SDK
# xlive.dll, the Steamworks redistributable steam_api.dll, the config and the README.

xls_add_release(lp2-gfwl
    FILES
        "$<TARGET_FILE:xlive>"
        "${XLS_STEAM_API_DLL}"
        "${CMAKE_CURRENT_SOURCE_DIR}/xlive_steamworks.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DEPENDS xlive
)
