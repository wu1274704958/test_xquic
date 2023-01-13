#pragma once
#ifdef MQAS_NO_EXPORT
# define MQAS_EXTERN /* nothing */
#else

#ifdef _WIN32
/* Windows - set up dll import/export decorators. */
# if defined(BUILDING_MQAS_SHARED)
    /* Building shared library. */
#   define MQAS_EXTERN __declspec(dllexport)
# elif 1
    /* Using shared library. */
#   define MQAS_EXTERN __declspec(dllimport)
# else
    /* Building static library. */
#   define MQAS_EXTERN /* nothing */
# endif
#elif __GNUC__ >= 4
# define MQAS_EXTERN __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550) /* Sun Studio >= 8 */
# define MQAS_EXTERN __global
#else
# define MQAS_EXTERN /* nothing */
#endif

#endif