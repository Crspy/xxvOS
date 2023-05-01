#include <kernel/version.h>

#define __STRINGIFY(x) #x
#define STRINGIFY(x) __STRINGIFY(x) /* expand */


/* Kernel name */
const char * __kernel_name = "xxvOSS";

/* format: major.minor.patch-suffix
 * This really shouldn't change, and if it does,
 * always ensure it still has the correct arguments
 * when used as a *printf() format. */
const char * __kernel_version_format = "%d.%d.%d-%s";

/* version numbers */
int    __kernel_version_major = 0;
int    __kernel_version_minor = 0;
int    __kernel_version_patch = 0;

/* Kernel build suffix, which doesn't necessarily
 * mean anything, but could be used in the future for attaching git tags */
const char * __kernel_version_suffix   = "r";

/* The release codename. */
const char * __kernel_version_codename = "\"First Step\"";

/* Build architecture */
const char * __kernel_arch = STRINGIFY(x86_64);

/* Rebuild from clean to reset these. */
const char * __kernel_build_date = __DATE__;
const char * __kernel_build_time = __TIME__;

#if (defined(__GNUC__) || defined(__GNUG__)) && !(defined(__clang__) || defined(__INTEL_COMPILER))
# define COMPILER_VERSION "gcc " __VERSION__
const char * __kernel_compiler_version = "gcc " __VERSION__;
#elif (defined(__clang__))
# define COMPILER_VERSION "clang " __clang_version__
const char * __kernel_compiler_version = "clang " __clang_version__;
#else
const char * __kernel_compiler_version = "unknown compiler!";
#endif
