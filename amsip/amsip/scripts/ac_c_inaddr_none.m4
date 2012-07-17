dnl
dnl CHECK_INADDR_NONE
dnl
dnl checks for missing INADDR_NONE macro
dnl
AC_DEFUN(CHECK_INADDR_NONE,[
  AC_TRY_COMPILE([
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
],[
unsigned long foo = INADDR_NONE;
],[
    HAVE_INADDR_NONE=yes
],[
    HAVE_INADDR_NONE=no
    AC_DEFINE(INADDR_NONE, ((unsigned int) 0xffffffff), [ ])
])
  AC_MSG_CHECKING(whether system defines INADDR_NONE)
  AC_MSG_RESULT($HAVE_INADDR_NONE)
])
