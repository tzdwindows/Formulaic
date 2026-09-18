# ==============================================================================
# Formulaic Test Suite CMake Configuration
# All tests strictly isolated within test/ directory
# ==============================================================================

macro(add_formulaic_test TEST_NAME SOURCE_FILE LINK_TARGET)
    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE ${LINK_TARGET})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    set_target_properties(${TEST_NAME} PROPERTIES FOLDER "Tests")
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

# Determine link target (prefer static for unit tests to avoid DLL search issues in CTest)
if(TARGET Formulaic_static)
    set(TEST_LIB_TARGET Formulaic_static)
else()
    set(TEST_LIB_TARGET Formulaic_shared)
endif()

# 1. Expression Parser & Bytecode Evaluation Correctness Test
add_formulaic_test(test_parser test_parser.cpp ${TEST_LIB_TARGET})

# 2. Offline Rasterization, Marching Squares & Image Export Test
add_formulaic_test(test_raster test_raster.cpp ${TEST_LIB_TARGET})

# 3. Native Window Binding & Pipeline Hooks Integration Test
add_formulaic_test(test_window_hook test_window_hook.cpp ${TEST_LIB_TARGET})

# 4. Custom User Input Scripting, Calculus/FFT & Image Render Test
add_formulaic_test(test_custom_render test_custom_render.cpp ${TEST_LIB_TARGET})

