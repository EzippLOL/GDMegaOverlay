include(FetchContent)

FetchContent_Declare(
    discord-rpc
    GIT_REPOSITORY https://github.com/discord/discord-rpc.git
    GIT_TAG        963aa9f3e5ce81a4682c6ca3d136cddda614db33
    GIT_PROGRESS TRUE
)
# discord-rpc runs clang-format on its own files during the build, and recent clang versions
# reject its .clang-format (duplicated key), so turn both the formatting and the examples off
set(CLANG_FORMAT_CMD "" CACHE STRING "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

message("Fetching discord-rpc")
FetchContent_MakeAvailable(discord-rpc)
