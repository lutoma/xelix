#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS() \
    do { \
        builtin_define_std("xelix"); \
        builtin_define_std("unix"); \
        builtin_define("__xelix__"); \
        builtin_define("__unix__"); \
        builtin_assert("system=xelix"); \
        builtin_assert("system=unix"); \
        builtin_assert("system=posix"); \
    } while (0);

/* Files that are linked before user code.
   The %s tells GCC to look for these files in the library directory. */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s crti.o%s crtbegin.o%s"

/* Files that are linked after user code. */
#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

/* Default arguments you want when running your
   i686-myos-gcc/x86_64-myos-gcc toolchain */
#undef LIB_SPEC
#define LIB_SPEC "-lc" /* link against C standard library */

/* Files that are linked before user code.
   The %s tells GCC to look for these files in the library directory. */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s crti.o%s crtbegin.o%s"

/* Files that are linked after user code. */
#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

/* Don't automatically add extern "C" { } around header files. */
#undef  NO_IMPLICIT_EXTERN_C
#define NO_IMPLICIT_EXTERN_C 1

#undef  OBJECT_FORMAT_ELF
#define OBJECT_FORMAT_ELF

/* This macro applies on top of OBJECT_FORMAT_ELF and indicates that
   we want to support both flat and ELF output.  */
#undef OBJECT_FORMAT_FLAT
#define OBJECT_FORMAT_FLAT
