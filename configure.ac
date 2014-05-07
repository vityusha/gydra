#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gydra], [0.1], [vit@trs.sut.ru])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile man/Makefile])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([portaudio], [Pa_Initialize], [], [
         echo "Portaudio library is required for this program"
         exit -1])
AC_CHECK_LIB([asound], [snd_ctl_open], [], [
         echo "ALSA asound library is required for this program"
         exit -1])
AC_CHECK_LIB([m], [sin], [], [
         echo "Math library is required for this program"
         exit -1])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h memory.h netinet/in.h stdint.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset select socket])

AC_OUTPUT
