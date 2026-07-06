#ifndef IVY_TYPES_H
#define IVY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #define MAX_PATH_LEN 260
#else
    #define MAX_PATH_LEN 4096
#endif

#ifndef IVY_INLINE
    #define IVY_INLINE static inline
#endif

#if defined(__GNUC__) || defined(__clang__)
    typedef __UINT8_TYPE__  u8;
    typedef __UINT16_TYPE__ u16;
    typedef __UINT32_TYPE__ u32;
    typedef __UINT64_TYPE__ u64;

    typedef __INT8_TYPE__   i8;
    typedef __INT16_TYPE__  i16;
    typedef __INT32_TYPE__  i32;
    typedef __INT64_TYPE__  i64;

    typedef __SIZE_TYPE__   usize;
#else
    #include <stdint.h>
    #include <stddef.h>

    typedef uint8_t   u8;
    typedef uint16_t  u16;
    typedef uint32_t  u32;
    typedef uint64_t  u64;

    typedef int8_t    i8;
    typedef int16_t   i16;
    typedef int32_t   i32;
    typedef int64_t   i64;

    typedef size_t    usize;
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define IVY_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define IVY_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define IVY_UNREACHABLE __builtin_unreachable()
#else
    #define IVY_LIKELY(x)   (x)
    #define IVY_UNLIKELY(x) (x)
    #define IVY_UNREACHABLE do {} while(0)
#endif

#define IVY_ASSERT_STATIC(condition, message) _Static_assert(condition, message)

#ifdef IVY_DEBUG
    #include <stdio.h>
    #include <stdlib.h>

    #define IVY_ASSERT(condition, ...)                                              \
        do {                                                                        \
            if (IVY_UNLIKELY(!(condition))) {                                       \
                fprintf(stderr, "\n[ASSERT FAILED] %s:%d\n", __FILE__, __LINE__);   \
                fprintf(stderr, "  " __VA_ARGS__);                                  \
                fprintf(stderr, "\n");                                              \
                abort();                                                            \
            }                                                                       \
        } while (0)

    #define IVY_CHECK(cond, fmt, ...)                                               \
        do {                                                                        \
            if (IVY_UNLIKELY(!(cond))) {                                            \
                fprintf(stderr, "[CHECK FAILED] " fmt "\n", ##__VA_ARGS__);         \
                abort();                                                            \
            }                                                                       \
        } while (0)

    #define IVY_ENSURE(cond) IVY_ASSERT(cond, "Critical condition failed!")

#else
    #define IVY_ASSERT(condition, ...) ((void)sizeof(condition))
    #define IVY_CHECK(cond, ...) ((void)sizeof(cond))
    #define IVY_ENSURE(cond) do { if (!(cond)) IVY_UNREACHABLE; } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif