#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([lkl-net], [0.7], [dumitru.sorin87@gmail.com])
AM_INIT_AUTOMAKE([lkl-net foreign], [0.6])
AC_CONFIG_FILES(Makefile)

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LEX
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostbyname inet_ntoa memset socket strchr strdup strerror strtoul])

AC_OUTPUT
