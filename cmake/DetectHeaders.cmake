# -------------------------------------------------------------------------------------------------- #
include(CheckCSourceCompiles)

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <stdio.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_STDIO_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <stdlib.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_STDLIB_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <string.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_STRING_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <strings.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_STRINGS_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <ctype.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_CTYPE_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <endian.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_ENDIAN_H )

if( NOT LIBAKRYPT_HAVE_ENDIAN_H )
  check_c_source_compiles("
     #include <sys/endian.h>
     int main( void ) {
        return 0;
     }" LIBAKRYPT_HAVE_SYSENDIAN_H )
endif()

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <time.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_TIME_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <syslog.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SYSLOG_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <unistd.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_UNISTD_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <fcntl.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_FCNTL_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <limits.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_LIMITS_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <sys/mman.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SYSMMAN_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <sys/stat.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SYSSTAT_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <sys/types.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SYSTYPES_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <sys/socket.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SYSSOCKET_H )

if( LIBAKRYPT_HAVE_SYSSOCKET_H )
    check_c_source_compiles("
      #include <sys/un.h>
      #include <sys/socket.h>
      int main( void ) {
         struct sockaddr_un sock;
        return 0;
      }" LIBAKRYPT_HAVE_SYSUN_H )
endif()

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <sys/select.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SYSSELECT_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <errno.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_ERRNO_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <termios.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_TERMIOS_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <dirent.h>
  int main( void ) {
     struct dirent st;
     st.d_type = 4;
     return 0;
  }" LIBAKRYPT_HAVE_DIRENT_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <fnmatch.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_FNMATCH_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <stdalign.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_STDALIGN_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <stdarg.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_STDARG_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <windows.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_WINDOWS_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <getopt.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_GETOPT_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <locale.h>
  int main( void ) {
     setlocale( LC_ALL, \"\" );
     return 0;
  }" LIBAKRYPT_HAVE_LOCALE_H )

# -------------------------------------------------------------------------------------------------- #
check_c_source_compiles("
  #include <signal.h>
  int main( void ) {
     return 0;
  }" LIBAKRYPT_HAVE_SIGNAL_H )

# -------------------------------------------------------------------------------------------------- #
