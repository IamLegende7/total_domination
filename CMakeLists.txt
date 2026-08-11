cmake_minimum_required(VERSION 4.0)
project(Total_Domination)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_BUILD_TYPE Debug)

### Source & Header files ###
file(GLOB_RECURSE SOURCES "src/*.cpp")


### Paths & Executable ###
# Create the executable target
add_executable(Total_Domination ${SOURCES})

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin/lib)
set(LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/lib)

#set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
#set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

### Compile options ###
# Set compile options
target_compile_options(Total_Domination PRIVATE -Wall -Wextra)

target_include_directories(Total_Domination PRIVATE 
    include 
        lib/rapidjson/include
        lib/BS_thread_pool/include
        lib/SDL_shadercross/include
)


### Settings ###
### Settings ###

set(SDLTTF_VENDORED ON)

### Librarys ###
set_target_properties(Total_Domination PROPERTIES
    BUILD_RPATH "$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib"
)

add_subdirectory(lib/SDL EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL_image EXCLUDE_FROM_ALL)
add_subdirectory(lib/SDL_ttf EXCLUDE_FROM_ALL)
add_subdirectory(lib/simpleini)

if(WIN32 AND MINGW)

    include(
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL_shadercross/build-scripts/download-prebuilt-DirectXShaderCompiler.cmake"
    )

    set(SDLSHADERCROSS_VENDORED OFF CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_DXC ON CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)

    set(SDL_CPU_X64 ON CACHE BOOL "" FORCE)

    set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
    set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)

    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL_shadercross/external/SPIRV-Cross"
        EXCLUDE_FROM_ALL
    )
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL_shadercross/external/SPIRV-Headers"
        EXCLUDE_FROM_ALL
    )
    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL_shadercross/external/SPIRV-Tools"
        EXCLUDE_FROM_ALL
    )

    if(TARGET spirv-cross-c AND NOT TARGET spirv_cross_c)
        add_library(spirv_cross_c ALIAS spirv-cross-c)
    endif()

    set(DXC_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL_shadercross/external/DirectXShaderCompiler-binaries/windows"
    )
    set(DirectXShaderCompiler_INCLUDE_PATH
        "${DXC_ROOT}/inc"
        CACHE PATH "" FORCE
    )
    set(DirectXShaderCompiler_dxcompiler_BINARY
        "${DXC_ROOT}/bin/x64/dxcompiler.dll"
        CACHE FILEPATH "" FORCE
    )
    set(DirectXShaderCompiler_dxcompiler_LIBRARY
        "${DXC_ROOT}/lib/x64/dxcompiler.lib"
        CACHE FILEPATH "" FORCE
    )
    set(DirectXShaderCompiler_dxil_BINARY
        "${DXC_ROOT}/bin/x64/dxil.dll"
        CACHE FILEPATH "" FORCE
    )

else()

    set(SDLSHADERCROSS_VENDORED ON CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_DXC ON CACHE BOOL "" FORCE)

endif()
add_subdirectory(lib/SDL_shadercross)

# Link Librarys
target_link_libraries(Total_Domination PRIVATE 
                      SDL3::SDL3
                      SDL3_image::SDL3_image
                      SDL3_ttf::SDL3_ttf
                      SimpleIni::SimpleIni
                      SDL3_shadercross::SDL3_shadercross
                    )

set_target_properties(Total_Domination
    PROPERTIES
        OUTPUT_NAME total_domination
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin"
)

message(STATUS "WIN32 = ${WIN32}")
message(STATUS "MINGW = ${MINGW}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_FIND_ROOT_PATH = ${CMAKE_FIND_ROOT_PATH}")
if(WIN32 AND MINGW)

    set(MINGW_RUNTIME_DIR
        "/usr/x86_64-w64-mingw32/bin"
    )

    set(DXC_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/SDL_shadercross/external/DirectXShaderCompiler-binaries/windows"
    )

    set(MINGW_RUNTIME_DLLS
        "${MINGW_RUNTIME_DIR}/libgcc_s_seh-1.dll"
        "${MINGW_RUNTIME_DIR}/libstdc++-6.dll"
        "${MINGW_RUNTIME_DIR}/libwinpthread-1.dll"
    )

    set(DXC_RUNTIME_DLLS
        "${DXC_ROOT}/bin/x64/dxcompiler.dll"
        "${DXC_ROOT}/bin/x64/dxil.dll"
    )

    foreach(DLL IN LISTS MINGW_RUNTIME_DLLS DXC_RUNTIME_DLLS)
        if(NOT EXISTS "${DLL}")
            message(FATAL_ERROR "Missing runtime DLL: ${DLL}")
        endif()

        message(STATUS "Runtime DLL: ${DLL}")
    endforeach()

    add_custom_command(TARGET Total_Domination POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E echo
            "Copying Windows runtime DLLs..."

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${MINGW_RUNTIME_DLLS}
            "$<TARGET_FILE_DIR:Total_Domination>"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DXC_RUNTIME_DLLS}
            "$<TARGET_FILE_DIR:Total_Domination>"

        COMMAND_EXPAND_LISTS
    )

endif()

if(UNIX AND NOT APPLE)

    add_custom_command(TARGET Total_Domination POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_BINARY_DIR}/lib/SDL_shadercross/external/DirectXShaderCompiler/lib/libdxcompiler.so"
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/lib/libdxcompiler.so"
    )

    set_target_properties(Total_Domination PROPERTIES
        BUILD_RPATH "$ORIGIN/lib"
        INSTALL_RPATH "$ORIGIN/lib"
    )

endif()