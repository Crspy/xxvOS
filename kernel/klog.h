
#include <stdarg.h>

/**
 * @brief Log level
 *
 */
typedef enum {
    KLOG_LEVEL_NONE,       /*!< No log output */
    KLOG_LEVEL_ERROR,      /*!< Critical errors, software module can not recover on its own */
    KLOG_LEVEL_WARN,       /*!< Error conditions from which recovery measures have been taken */
    KLOG_LEVEL_INFO,       /*!< Information messages which describe normal flow of events */
    KLOG_LEVEL_DEBUG,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    KLOG_LEVEL_VERBOSE     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} klog_level_t;
typedef int (*vprintf_like_t)(const char*, va_list);



/**
 * @brief Function which returns system timestamp to be used in log output
 *
 * This function is used in expansion of KLOGx macros to print
 * the system time as "HH:MM:SS.sss".
 *
 * @return timestamp, in "HH:MM:SS.sss"
 */
char* klog_system_timestamp(void);

/**
 * @brief Write message into the log
 *
 * This function is not intended to be used directly. Instead, use one of
 * KLOGE, KLOGW, KLOGI, KLOGD, KLOGV macros.
 *
 * This function or these macros should not be used from an interrupt.
 */
void klog_write(klog_level_t level, const char* tag, const char* format, ...) __attribute__((format(printf, 3, 4)));

/**
 * @brief Write message into the log, va_list variant
 * @see klog_write()
 *
 * This function is provided to ease integration toward other logging framework,
 * so that klog can be used as a log sink.
 */
void klog_writev(klog_level_t level, const char* tag, const char* format, va_list args);

vprintf_like_t klog_set_vprintf(vprintf_like_t func);


#define CONFIG_LOG_COLORS 1

#if CONFIG_LOG_COLORS
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D
#define LOG_COLOR_V
#else //CONFIG_LOG_COLORS
#define LOG_COLOR_E
#define LOG_COLOR_W
#define LOG_COLOR_I
#define LOG_COLOR_D
#define LOG_COLOR_V
#define LOG_RESET_COLOR
#endif //CONFIG_LOG_COLORS

#define LOG_SYSTEM_TIME_FORMAT(letter, format)  LOG_COLOR_ ## letter #letter " (%s) %s: " format LOG_RESET_COLOR "\n"



#define KLOG_LEVEL(level, tag, format, ...) do {                     \
        if (level== KLOG_LEVEL_ERROR )          { klog_write(KLOG_LEVEL_ERROR,      tag, LOG_SYSTEM_TIME_FORMAT(E, format), klog_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level== KLOG_LEVEL_WARN )      { klog_write(KLOG_LEVEL_WARN,       tag, LOG_SYSTEM_TIME_FORMAT(W, format), klog_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level== KLOG_LEVEL_DEBUG )     { klog_write(KLOG_LEVEL_DEBUG,      tag, LOG_SYSTEM_TIME_FORMAT(D, format), klog_system_timestamp(), tag, ##__VA_ARGS__); } \
        else if (level== KLOG_LEVEL_VERBOSE )   { klog_write(KLOG_LEVEL_VERBOSE,    tag, LOG_SYSTEM_TIME_FORMAT(V, format), klog_system_timestamp(), tag, ##__VA_ARGS__); } \
        else                                { klog_write(KLOG_LEVEL_INFO,       tag, LOG_SYSTEM_TIME_FORMAT(I, format), klog_system_timestamp(), tag, ##__VA_ARGS__); } \
    } while(0)



#define KLOG_LOCAL_LEVEL KLOG_LEVEL_VERBOSE // should be in an option in KConfig (if we ever made one)

/** runtime macro to output logs at a specified level. Also check the level with ``KLOG_LOCAL_LEVEL``.
 *
 * @see ``printf``, ``KLOG_LEVEL``
 */
#define KLOG_LEVEL_LOCAL(level, tag, format, ...) do {               \
        if ( KLOG_LOCAL_LEVEL >= level ) KLOG_LEVEL(level, tag, format, ##__VA_ARGS__); \
    } while(0)


#define KLOGE( tag, format, ... ) KLOG_LEVEL_LOCAL(KLOG_LEVEL_ERROR,   tag, format, ##__VA_ARGS__)
#define KLOGW( tag, format, ... ) KLOG_LEVEL_LOCAL(KLOG_LEVEL_WARN,    tag, format, ##__VA_ARGS__)
#define KLOGI( tag, format, ... ) KLOG_LEVEL_LOCAL(KLOG_LEVEL_INFO,    tag, format, ##__VA_ARGS__)
#define KLOGD( tag, format, ... ) KLOG_LEVEL_LOCAL(KLOG_LEVEL_DEBUG,   tag, format, ##__VA_ARGS__)
#define KLOGV( tag, format, ... ) KLOG_LEVEL_LOCAL(KLOG_LEVEL_VERBOSE, tag, format, ##__VA_ARGS__)
