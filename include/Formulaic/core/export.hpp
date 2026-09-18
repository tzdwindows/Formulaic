#pragma once

// ==============================================================================
// Formulaic - High Performance Math Expression & Rendering Engine
// Symbol visibility & export macro definitions
// ==============================================================================

#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(FORMULAIC_STATIC)
        // Building or consuming static library
        #define FORMULAIC_API
    #elif defined(FORMULAIC_EXPORTS)
        // Building dynamic library (DLL)
        #define FORMULAIC_API __declspec(dllexport)
    #else
        // Consuming dynamic library (DLL)
        #define FORMULAIC_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define FORMULAIC_API __attribute__((visibility("default")))
    #else
        #define FORMULAIC_API
    #endif
#endif
