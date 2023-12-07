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

#define DYNAMIC_LINKER "/usr/lib/ld-xelix.so"

#undef LIB_SPEC
#define LIB_SPEC "-lc"

#undef REAL_LIBGCC_SPEC
#define REAL_LIBGCC_SPEC "%{static: -lgcc%s}%{!static: -lgcc_s%s}"

#undef  LINK_SPEC
#define LINK_SPEC  "%{shared:-shared} %{!shared: %{static:-static} %{!static: -dynamic-linker " DYNAMIC_LINKER "}}"

/* Files that are linked before user code.
   The %s tells GCC to look for these files in the library directory. */
#undef STARTFILE_SPEC
#define STARTFILE_SPEC "%{!shared: crt0.o%s} crti.o%s %{!shared: crtbegin.o%s} %{shared: crtbeginS.o%s}"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC "%{!shared: crtend.o%s} %{shared: crtendS.o%s} crtn.o%s"

/* Use --as-needed -lgcc_s for eh support.  */
#ifdef HAVE_LD_AS_NEEDED
#define USE_LD_AS_NEEDED 1
#endif

#undef  OBJECT_FORMAT_ELF
#define OBJECT_FORMAT_ELF
