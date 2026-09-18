cmake_minimum_required(VERSION 3.20)
project(Formulaic VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern folder structuring in Visual Studio IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Global compilation options
if(MSVC)
    add_compile_options(/W4 /WX- /wd4251 /permissive- /Zc:preprocessor /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build options
option(FORMULAIC_BUILD_STATIC "Build Formulaic as static library" ON)
option(FORMULAIC_BUILD_SHARED "Build Formulaic as shared library (DLL)" ON)
option(FORMULAIC_BUILD_TESTS "Build Formulaic test suite in test/" ON)
option(FORMULAIC_BUILD_EXAMPLES "Build Formulaic example applications in examples/" ON)

# Source files
set(FORMULAIC_PUBLIC_HEADERS
    include/Formulaic/core/export.hpp
    include/Formulaic/core/types.hpp
    include/Formulaic/core/error.hpp
    include/Formulaic/parser/token.hpp
    include/Formulaic/parser/ast.hpp
    include/Formulaic/parser/bytecode.hpp
    include/Formulaic/parser/expression.hpp
    include/Formulaic/render/color.hpp
    include/Formulaic/render/viewport.hpp
    include/Formulaic/render/framebuffer.hpp
    include/Formulaic/render/image_export.hpp
    include/Formulaic/render/frame_stream.hpp
    include/Formulaic/render/raster_engine.hpp
    include/Formulaic/render/window_renderer.hpp
    include/Formulaic/hooks/hooks.hpp
    include/Formulaic/hooks/pipeline_hooks.hpp
    include/Formulaic/math/calculus.hpp
    include/Formulaic/math/fft.hpp
    include/Formulaic/utils/timer.hpp
)

set(FORMULAIC_SOURCES
    src/parser/lexer.hpp
    src/parser/lexer.cpp
    src/parser/parser.hpp
    src/parser/parser.cpp
    src/parser/bytecode_compiler.hpp
    src/parser/bytecode_compiler.cpp
    src/parser/bytecode_vm.hpp
    src/parser/bytecode_vm.cpp
    src/parser/expression.cpp
    src/math/calculus.cpp
    src/math/fft.cpp
    src/render/color.cpp
    src/render/framebuffer.cpp
    src/render/image_export.cpp
    src/render/frame_stream.cpp
    src/render/raster_engine.cpp
    src/render/backends/win32_window.cpp
    src/render/backends/software_window.cpp
)

# -----------------------------------------------------------------------------
# Target 1: Static Library (.lib)
# -----------------------------------------------------------------------------
if(FORMULAIC_BUILD_STATIC)
    add_library(Formulaic_static STATIC ${FORMULAIC_PUBLIC_HEADERS} ${FORMULAIC_SOURCES})
    add_library(Formulaic::static ALIAS Formulaic_static)
    target_compile_definitions(Formulaic_static PUBLIC FORMULAIC_STATIC)
    target_include_directories(Formulaic_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_include_directories(Formulaic_static PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    if(WIN32)
        target_link_libraries(Formulaic_static PUBLIC gdi32 user32)
    endif()
    set_target_properties(Formulaic_static PROPERTIES
        OUTPUT_NAME "Formulaic_static"
        FOLDER "Libraries"
    )
endif()

# -----------------------------------------------------------------------------
# Target 2: Dynamic Library (.dll + .lib import stub)
# -----------------------------------------------------------------------------
if(FORMULAIC_BUILD_SHARED)
    add_library(Formulaic_shared SHARED ${FORMULAIC_PUBLIC_HEADERS} ${FORMULAIC_SOURCES})
    add_library(Formulaic::shared ALIAS Formulaic_shared)
    add_library(Formulaic::Formulaic ALIAS Formulaic_shared)
    target_compile_definitions(Formulaic_shared PRIVATE FORMULAIC_EXPORTS)
    target_include_directories(Formulaic_shared PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
    target_include_directories(Formulaic_shared PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    if(WIN32)
        target_link_libraries(Formulaic_shared PUBLIC gdi32 user32)
    endif()
    set_target_properties(Formulaic_shared PROPERTIES
        OUTPUT_NAME "Formulaic"
        FOLDER "Libraries"
    )
endif()

# -----------------------------------------------------------------------------
# Subdirectories: Tests & Examples
# -----------------------------------------------------------------------------
if(FORMULAIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(FORMULAIC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
