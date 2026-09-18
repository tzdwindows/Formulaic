# ==============================================================================
# Formulaic Examples CMake Configuration
# Demonstrates consumption of both Static (.lib) and Shared (.dll) targets
# ==============================================================================

# Example 1: Static Library Linkage (Offline image generation)
if(TARGET Formulaic_static)
    add_executable(example_static example_static.cpp)
    target_link_libraries(example_static PRIVATE Formulaic_static)
    set_target_properties(example_static PROPERTIES FOLDER "Examples")
endif()

# Example 2: Shared Library Linkage (Real-time Interactive HWND Window)
if(TARGET Formulaic_shared)
    add_executable(example_interactive_window example_interactive_window.cpp)
    target_link_libraries(example_interactive_window PRIVATE Formulaic_shared)
    set_target_properties(example_interactive_window PROPERTIES FOLDER "Examples")

    # On Windows, copy Formulaic.dll next to the executable for runtime execution
    if(WIN32)
        add_custom_command(TARGET example_interactive_window POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Formulaic_shared>
                $<TARGET_FILE_DIR:example_interactive_window>
        )
    endif()
endif()
