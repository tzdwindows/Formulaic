cmake_minimum_required(VERSION 3.20)
project(Formulaic VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable modern folder structuring in Visual Studio IDE
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Global compilation options
if(MSVC)
    add_compile_options(/W4 /WX- /wd4251 /wd4146 /wd4244 /wd4267 /permissive- /Zc:preprocessor /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build options
option(FORMULAIC_BUILD_STATIC "Build Formulaic as static library" ON)
option(FORMULAIC_BUILD_SHARED "Build Formulaic as shared library (DLL)" ON)
option(FORMULAIC_BUILD_TESTS "Build Formulaic test suite in test/" ON)
option(FORMULAIC_BUILD_EXAMPLES "Build Formulaic example applications in examples/" ON)
option(FORMULAIC_ENABLE_GMP "Enable GNU Multiple Precision Arithmetic Library (GMP)" ON)

# -----------------------------------------------------------------------------
# Dependency: GMP (Native with assembly acceleration or cross-platform mini-gmp fallback)
# -----------------------------------------------------------------------------
if(FORMULAIC_ENABLE_GMP)
    include(SelectLibraryConfigurations)
    find_path(GMP_INCLUDE_DIR NAMES gmp.h
        HINTS
        $ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/include
        E:/vcpkg/installed/x64-windows-static/include
        E:/vcpkg/installed/x64-windows/include
        /usr/include
        /usr/local/include
        /opt/homebrew/include
    )
    find_library(GMP_LIBRARY_RELEASE NAMES gmp gmp-10 libgmp
        PATHS
        $ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/lib
        E:/vcpkg/installed/x64-windows-static/lib
        E:/vcpkg/installed/x64-windows/lib
        /usr/lib
        /usr/local/lib
        /opt/homebrew/lib
        NO_DEFAULT_PATH
    )
    find_library(GMP_LIBRARY_DEBUG NAMES gmp gmp-10 libgmp
        PATHS
        $ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/debug/lib
        E:/vcpkg/installed/x64-windows-static/debug/lib
        E:/vcpkg/installed/x64-windows/debug/lib
        NO_DEFAULT_PATH
    )
    select_library_configurations(GMP)

    if(GMP_INCLUDE_DIR AND (GMP_LIBRARY_RELEASE OR GMP_LIBRARY_DEBUG))
        message(STATUS "Found Native GMP: ${GMP_LIBRARIES} (headers: ${GMP_INCLUDE_DIR})")
        set(FORMULAIC_GMP_DEF "FORMULAIC_HAS_NATIVE_GMP")
        set(FORMULAIC_GMP_LIBS ${GMP_LIBRARIES})
        set(FORMULAIC_GMP_INCLUDES ${GMP_INCLUDE_DIR})
        set(FORMULAIC_GMP_EXTRA_SOURCES "")
    else()
        message(STATUS "Native GMP not found. Using bundled cross-platform mini-gmp.")
        set(FORMULAIC_GMP_DEF "FORMULAIC_HAS_MINI_GMP")
        set(FORMULAIC_GMP_LIBS "")
        set(FORMULAIC_GMP_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mini-gmp)
        set(FORMULAIC_GMP_EXTRA_SOURCES
            3rdparty/mini-gmp/mini-gmp.h
            3rdparty/mini-gmp/mini-gmp.c
            3rdparty/mini-gmp/mini-mpq.h
            3rdparty/mini-gmp/mini-mpq.c
        )
        if(MSVC)
            set_source_files_properties(
                ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mini-gmp/mini-gmp.c
                ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mini-gmp/mini-mpq.c
                PROPERTIES COMPILE_FLAGS "/wd4146 /wd4244 /wd4267 /wd4018"
            )
        endif()
    endif()
endif()

# Source files
set(FORMULAIC_PUBLIC_HEADERS
    include/Formulaic/core/export.hpp
    include/Formulaic/core/types.hpp
    include/Formulaic/core/error.hpp
    include/Formulaic/parser/token.hpp
    include/Formulaic/parser/ast.hpp
    include/Formulaic/parser/bytecode.hpp
    include/Formulaic/parser/expression.hpp
    include/Formulaic/parser/latex_converter.hpp
    include/Formulaic/render/color.hpp
    include/Formulaic/render/viewport.hpp
    include/Formulaic/render/framebuffer.hpp
    include/Formulaic/render/image_export.hpp
    include/Formulaic/render/frame_stream.hpp
    include/Formulaic/render/raster_engine.hpp
    include/Formulaic/render/window_renderer.hpp
    include/Formulaic/render/latex_render_module.hpp
    include/Formulaic/hooks/hooks.hpp
    include/Formulaic/hooks/pipeline_hooks.hpp
    include/Formulaic/math/calculus.hpp
    include/Formulaic/math/fft.hpp
    include/Formulaic/math/gmp_types.hpp
    include/Formulaic/math/bigint.hpp
    include/Formulaic/math/rational.hpp
    include/Formulaic/math/gmp_evaluator.hpp
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
    src/parser/latex_converter.cpp
    src/math/calculus.cpp
    src/math/fft.cpp
    src/math/bigint.cpp
    src/math/rational.cpp
    src/math/gmp_evaluator.cpp
    ${FORMULAIC_GMP_EXTRA_SOURCES}
    src/render/color.cpp
    src/render/framebuffer.cpp
    src/render/image_export.cpp
    src/render/frame_stream.cpp
    src/render/raster_engine.cpp
    src/render/latex_render_module.cpp
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
    if(FORMULAIC_ENABLE_GMP)
        target_compile_definitions(Formulaic_static PUBLIC ${FORMULAIC_GMP_DEF})
        target_include_directories(Formulaic_static PUBLIC ${FORMULAIC_GMP_INCLUDES})
        if(FORMULAIC_GMP_LIBS)
            target_link_libraries(Formulaic_static PUBLIC ${FORMULAIC_GMP_LIBS})
        endif()
    endif()
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
    if(FORMULAIC_ENABLE_GMP)
        target_compile_definitions(Formulaic_shared PUBLIC ${FORMULAIC_GMP_DEF})
        target_include_directories(Formulaic_shared PUBLIC ${FORMULAIC_GMP_INCLUDES})
        if(FORMULAIC_GMP_LIBS)
            target_link_libraries(Formulaic_shared PUBLIC ${FORMULAIC_GMP_LIBS})
        endif()
    endif()
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
