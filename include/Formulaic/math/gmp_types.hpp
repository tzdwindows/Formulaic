#pragma once

#include <Formulaic/core/export.hpp>

#if defined(FORMULAIC_HAS_NATIVE_GMP)
    #include <gmp.h>
    #define FORMULAIC_GMP_BACKEND_NAME "Native GMP (Assembly Accelerated)"
    #define FORMULAIC_HAS_GMP 1
#elif defined(FORMULAIC_HAS_MINI_GMP)
    #include "mini-gmp.h"
    #include "mini-mpq.h"
    #define FORMULAIC_GMP_BACKEND_NAME "GNU mini-gmp (Pure C Portable)"
    #define FORMULAIC_HAS_GMP 1
#else
    #define FORMULAIC_GMP_BACKEND_NAME "None"
#endif

namespace formulaic::math {

[[nodiscard]] FORMULAIC_API const char* get_gmp_backend_info() noexcept;
[[nodiscard]] FORMULAIC_API bool has_gmp_support() noexcept;

} // namespace formulaic::math
