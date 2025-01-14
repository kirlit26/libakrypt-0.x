/* ----------------------------------------------------------------------------------------------- */
/*  Copyright (c) 2014 - 2019 by Axel Kenzo, axelkenzo@mail.ru                                     */
/*                                                                                                 */
/*  Файл ak_tools.с                                                                                */
/*  - содержит реализацию служебных функций, не экспортируемых за пределы библиотеки               */
/* ----------------------------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------------------------- */
/* это объявление нужно для использования функции fdopen() */
#ifdef __linux__
 #ifndef _POSIX_C_SOURCE
   #define _POSIX_C_SOURCE 2
 #endif
#endif

/* ----------------------------------------------------------------------------------------------- */
 #include <ak_tools.h>

/* ----------------------------------------------------------------------------------------------- */
#ifdef LIBAKRYPT_HAVE_STDIO_H
 #include <stdio.h>
#else
 #error Library cannot be compiled without stdio.h header
#endif
#ifdef LIBAKRYPT_HAVE_STDLIB_H
 #include <stdlib.h>
#else
 #error Library cannot be compiled without stdlib.h header
#endif
#ifdef LIBAKRYPT_HAVE_STRING_H
 #include <string.h>
#else
 #error Library cannot be compiled without string.h header
#endif
#ifdef LIBAKRYPT_HAVE_ERRNO_H
 #include <errno.h>
#else
 #error Library cannot be compiled without errno.h header
#endif
#ifdef LIBAKRYPT_HAVE_STDARG_H
 #include <stdarg.h>
#else
 #error Library cannot be compiled without stdarg.h header
#endif

/* ----------------------------------------------------------------------------------------------- */
#ifdef LIBAKRYPT_HAVE_FCNTL_H
 #include <fcntl.h>
#endif
#ifdef LIBAKRYPT_HAVE_SYSSTAT_H
 #include <sys/stat.h>
#endif
#ifdef LIBAKRYPT_HAVE_TERMIOS_H
 #include <termios.h>
#endif
#ifdef LIBAKRYPT_HAVE_UNISTD_H
 #include <unistd.h>
#endif
#ifdef LIBAKRYPT_HAVE_SYSLOG_H
 #include <syslog.h>
#endif
#ifdef LIBAKRYPT_HAVE_UNISTD_H
 #include <unistd.h>
#endif
#ifdef LIBAKRYPT_HAVE_LIMITS_H
 #include <limits.h>
#endif

/* ----------------------------------------------------------------------------------------------- */
#ifdef LIBAKRYPT_HAVE_PTHREAD
 #include <pthread.h>
#endif

/* ----------------------------------------------------------------------------------------------- */
#ifdef _MSC_VER
 #include <share.h>
 #include <direct.h>
#endif

/* ----------------------------------------------------------------------------------------------- */
/*!  Переменная, содержащая в себе код последней ошибки                                            */
 static int ak_errno = ak_error_ok;

/* ----------------------------------------------------------------------------------------------- */
/*! Внутренний указатель на функцию аудита                                                         */
 static ak_function_log *ak_function_log_default = NULL;
#ifdef LIBAKRYPT_HAVE_PTHREAD
 static pthread_mutex_t ak_function_log_default_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Cтатическая переменная для вывода сообщений. */
 static char ak_ptr_to_hexstr_static_buffer[1024];

/*! \brief Cтатическая переменные для окрашивания кодов и выводимых сообщений. */
 static char *ak_error_code_start_string = "";
 static char *ak_error_code_end_string = "";
#ifndef _WIN32
 static char *ak_error_code_start_red_string = "\x1b[31m";
 static char *ak_error_code_end_red_string = "\x1b[0m";
#endif

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Тип данных для хранения одной опции библиотеки */
 typedef struct option {
  /*! \brief Человекочитаемое имя опции, используется для поиска и установки значения */
   char *name;
  /*! \brief Численное значение опции (31 значащий бит + знак) */
   ak_int64 value;
 } *ak_option;

/* ----------------------------------------------------------------------------------------------- */
/*! Константные значения опций (значения по-умолчанию) */
 static struct option options[] = {
     { "log_level", ak_log_standard },
     { "context_manager_size", 32 },
     { "context_manager_max_size", 4096 },
     { "pbkdf2_iteration_count", 2000 },
     { "hmac_key_count_resource", 65536 },

  /* значение константы задает максимальный объем зашифрованной информации на одном ключе в 4 Mб:
                                 524288 блока x 8 байт на блок = 4.194.304 байт = 4096 Кб = 4 Mб   */
     { "magma_cipher_resource", 524288 },

  /* значение константы задает максимальный объем зашифрованной информации на одном ключе в 32 Mб:
                             2097152 блока x 16 байт на блок = 33.554.432 байт = 32768 Кб = 32 Mб  */
     { "kuznechik_cipher_resource", 2097152 },
     { "acpkm_message_count", 4096 },
     { "acpkm_section_magma_block_count", 128 },
     { "acpkm_section_kuznechik_block_count", 512 },

  /* при значении равным единицы, формат шифрования данных соответствует варианту OpenSSL */
     { "openssl_compability", 0 },

     { NULL, 0 } /* завершающая константа, должна всегда принимать нулевые значения */
 };

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.

    \return Общее количество опций библиотеки.                                                     */
/* ----------------------------------------------------------------------------------------------- */
 size_t ak_libakrypt_options_count( void )
{
  return ( sizeof( options )/( sizeof( struct option ))-1 );
}

/* ----------------------------------------------------------------------------------------------- */
/*! \param name Имя опции
    \return Значение опции с заданным именем. Если имя указано неверно, то возвращается
    ошибка \ref ak_error_wrong_option.                                                             */
/* ----------------------------------------------------------------------------------------------- */
 ak_int64 ak_libakrypt_get_option( const char *name )
{
  size_t i = 0;
  ak_int64 result = ak_error_wrong_option;
  for( i = 0; i < ak_libakrypt_options_count(); i++ ) {
     if( strncmp( name, options[i].name, strlen( options[i].name )) == 0 ) result = options[i].value;
  }
 return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание! Функция не проверяет и не интерпретирует значение устанавливааемой опции.

    \param name Имя опции
    \param value Значение опции

    \return В случае удачного установления значения опции возввращается \ref ak_error_ok.
     Если имя опции указано неверно, то возвращается ошибка \ref ak_error_wrong_option.            */
/* ----------------------------------------------------------------------------------------------- */
 int ak_libakrypt_set_option( const char *name, const ak_int64 value )
{
  size_t i = 0;
  int result = ak_error_wrong_option;
  for( i = 0; i < ak_libakrypt_options_count(); i++ ) {
     if( strncmp( name, options[i].name, strlen( options[i].name )) == 0 ) {
       options[i].value = value;
       result = ak_error_ok;
     }
  }
 return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция возвращает указатель на строку символов, содержащую человекочитаемое имя опции
    библиотеки. Для строки выделяется память, которая должна быть позднее удалена пользователем
    самостоятельно.

    \b Внимание. Функция экспортируется.

    \param index Индекс опции, должен быть от нуля до значения,
    возвращаемого функцией ak_libakrypt_options_count().

    \return Строка симовлов, содержащая имя функции, в случае правильно определенного индекса.
    В противном случае, возвращается NULL.                                                         */
/* ----------------------------------------------------------------------------------------------- */
 char *ak_libakrypt_get_option_name( const size_t index )
{
 size_t len = 0;
 char *ptr = NULL;
 if( index >= ak_libakrypt_options_count() ) return ak_null_string;
  else {
    len = strlen( options[index].name ) + 1;
    if(( ptr = malloc( len )) == NULL ) {
      ak_error_message( ak_error_null_pointer, __func__, "incorrect memory allocation");
    } else memcpy( ptr, options[index].name, len );
  }
  return ptr;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.

    \param index Индекс опции, должен быть от нуля до значения,
    возвращаемого функцией ak_libakrypt_options_count().

    \return Целое неотрицательное число, содержащее значение опции с заданным индексом.
    В случае неправильно определенного индекса возвращается значение \ref ak_error_wrong_option.   */
/* ----------------------------------------------------------------------------------------------- */
 ak_int64 ak_libakrypt_get_option_value( const size_t index )
{
 if( index >= ak_libakrypt_options_count() ) return ak_error_wrong_option;
  else return options[index].value;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Если это возможно, то функция возвращает память, выравненную по границе 16 байт.
    @param size Размер выделяемой памяти в байтах.
    @return Указатель на выделенную память.                                                        */
/* ----------------------------------------------------------------------------------------------- */
 ak_pointer ak_libakrypt_aligned_malloc( size_t size )
{
 return
#ifdef LIBAKRYPT_HAVE_STDALIGN
  aligned_alloc( 16,
#else
  malloc(
#endif
  size );
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция заполняет заданную область памяти случайными данными, выработанными заданным
    генератором псевдослучайных чисел. Генератор должен быть предварительно корректно
    инициализирован с помощью функции вида `ak_random_context_create_...()`.

    @param ptr Область данных, которая заполняется случайным мусором.
    @param size Размер заполняемой области в байтах.
    @param generator Генератор псевдо-случайных чисел, используемый для генерации случайного мусора.
    @param readflag Булева переменная, отвечающая за обязательное чтение сгенерированных данных.
    В большинстве случаев должна принимать истинное значение.
    @return Функция возвращает \ref ak_error_ok (ноль) в случае успешного уничтожения данных.
    В противном случае возвращается код ошибки.                                                    */
/* ----------------------------------------------------------------------------------------------- */
 int ak_ptr_context_wipe( ak_pointer ptr, size_t size, ak_random rnd )
{
  size_t idx = 0;
  if( rnd == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                                "using null pointer to random generator context" );
  if( rnd->random == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                                  "using uninitialized random generator context" );
  if( size > (((size_t)-1) >> 1 )) return ak_error_message( ak_error_wrong_length, __func__,
                                                                   "using very large size value" );
  if(( ptr == NULL ) || ( size == 0 )) return ak_error_ok;

  if( ak_random_context_random( rnd, ptr, (ssize_t) size ) != ak_error_ok ) {
    memset( ptr, 0, size );
    return ak_error_message( ak_error_write_data, __func__, "incorrect memory wiping" );
  }
 /* запись в память при чтении => необходим вызов функции чтения данных из ptr */
  for( idx = 0; idx < size; idx++ ) ((ak_uint8 *)ptr)[idx] += ((ak_uint8 *)ptr)[size - 1 - idx];
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
#ifndef LIBAKRYPT_CONST_CRYPTO_PARAMS
/* ----------------------------------------------------------------------------------------------- */
/*! @param hpath Буффер в который будет помещено имя домашнего каталога пользователя.
    @param size Размер буффера в байтах.

    @return В случае возникновения ошибки возвращается ее код. В случае успеха
    возвращается \ref ak_error_ok.                                                                 */
/* ----------------------------------------------------------------------------------------------- */
 int ak_libakrypt_get_home_path( char *hpath, const size_t size )
{
 if( hpath == NULL ) return ak_error_message( ak_error_null_pointer, __func__,
                                                         "using null pointer to filename buffer" );
 if( !size ) return ak_error_message( ak_error_zero_length, __func__,
                                                               "using a buffer with zero length" );
 memset( hpath, 0, size );

 #ifdef _WIN32
  /* в начале определяем, находимся ли мы в консоли MSys */
   GetEnvironmentVariableA( "HOME", hpath, ( DWORD )size );
  /* если мы находимся не в консоли, то строка hpath должна быть пустой */
   if( strlen( hpath ) == 0 ) {
     GetEnvironmentVariableA( "USERPROFILE", hpath, ( DWORD )size );
   }
 #else
   ak_snprintf( hpath, size, "%s", getenv( "HOME" ));
 #endif

 if( strlen( hpath ) == 0 ) return ak_error_message( ak_error_undefined_value, __func__,
                                                                           "wrong user home path");
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*!
   @param filename Массив, куда помещается имя файла. Память под массив
          должна быть быделена заранее.
   @param size Размер выделенной памяти.
   @param lastname Собственно короткое имя файла, заданное в виде null-строки.
   @param where Указатель на то, в каком каталоге будет расположен файл с настройками.
          Значение 0 - домашний каталог, значение 1 - общесистемный каталог
   @return Функция возвращает код ошибки.                                                          */
/* ----------------------------------------------------------------------------------------------- */
 int ak_libakrypt_create_filename( char *filename,
                                              const size_t size, char *lastname, const int where  )
{
 int error = ak_error_ok;
 char hpath[FILENAME_MAX];

  memset( (void *)filename, 0, size );
  memset( (void *)hpath, 0, FILENAME_MAX );

  switch( where )
 {
   case 0  : /* имя файла помещается в домашний каталог пользователя */
             if(( error = ak_libakrypt_get_home_path( hpath, FILENAME_MAX )) != ak_error_ok )
               return ak_error_message_fmt( error, __func__, "wrong %s name creation", lastname );
             #ifdef _WIN32
              ak_snprintf( filename, size, "%s\\.config\\libakrypt\\%s", hpath, lastname );
             #else
              ak_snprintf( filename, size, "%s/.config/libakrypt/%s", hpath, lastname );
             #endif
             break;

   case 1  : { /* имя файла помещается в общесистемный каталог */
               size_t len = 0;
               if(( len = strlen( LIBAKRYPT_OPTIONS_PATH )) > FILENAME_MAX-16 ) {
                 return ak_error_message( ak_error_wrong_length, __func__ ,
                                                           "wrong length of predefined filepath" );
               }
               memcpy( hpath, LIBAKRYPT_OPTIONS_PATH, len );
             }
             #ifdef _WIN32
              ak_snprintf( filename, size, "%s\\%s", hpath, lastname );
             #else
              ak_snprintf( filename, size, "%s/%s", hpath, lastname );
             #endif
             break;
   default : return ak_error_message( ak_error_undefined_value, __func__,
                                                       "unexpected value of \"where\" parameter ");
 }
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
 static bool_t ak_libakrypt_load_one_option( const char *string, const char *field, ak_int64 *value )
{
  char *ptr = NULL, *endptr = NULL;
  if(( ptr = strstr( string, field )) != NULL ) {
    ak_int64 val = ( ak_int64 ) strtoll( ptr += strlen(field), &endptr, 10 ); // strtoll
    if(( endptr != NULL ) && ( ptr == endptr )) {
      ak_error_message_fmt( ak_error_undefined_value, __func__,
                                    "using an undefinded value for variable %s", field );
      return ak_false;
    }
    if(( errno == ERANGE && ( val >= INT_MAX || val <= INT_MIN )) || (errno != 0 && val == 0)) {
     #ifdef _MSC_VER
       char mbuf[256];
       strerror_s( mbuf, 256, errno );
       ak_error_message_fmt( ak_error_undefined_value, __func__, "%s for field %s", mbuf, field );
     #else
       ak_error_message_fmt( ak_error_undefined_value, __func__,
                                                    "%s for field %s", strerror( errno ), field );
     #endif
    } else {
             *value = val;
             return ak_true;
           }
  }
 return ak_false;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Функция считывает опции из открытого файла, дескриптор которого передается в
    качестве аргумента функции.

    @param fd Дескриптор файла. Должен быть предварительно открыт на чтение с помощью функции
    ak_file_is_exist().

    @return Функция возвращает код ошибки или \ref ak_error_ok.                                    */
/* ----------------------------------------------------------------------------------------------- */
 static int ak_libakrypt_load_options_from_file( ak_file fd )
{
 int off = 0;
 size_t idx = 0;
 char ch, localbuffer[1024];

 /* нарезаем входные на строки длиной не более чем 1022 символа */
  memset( localbuffer, 0, 1024 );
  for( idx = 0; idx < (size_t) fd->size; idx++ ) {
     if( ak_file_read( fd, &ch, 1 ) != 1 ) {
       ak_file_close(fd);
       return ak_error_message( ak_error_read_data, __func__ ,
                                                         "unexpected end of libakrypt.conf file" );
     }
     if( off > 1022 ) {
       ak_file_close( fd );
       return ak_error_message( ak_error_read_data, __func__ ,
                                         "libakrypt.conf has a line with more than 1022 symbols" );
     }
    if( ch == '\n' ) {
      if((strlen(localbuffer) != 0 ) && ( strchr( localbuffer, '#' ) == 0 )) {
        ak_int64 value = 0;

        /* устанавливаем уровень аудита */
        if( ak_libakrypt_load_one_option( localbuffer, "log_level = ", &value ))
          ak_libakrypt_set_option( "log_level", value );

        /* устанавливаем минимальный размер структуры управления контекстами */
        if( ak_libakrypt_load_one_option( localbuffer, "context_manager_size = ", &value )) {
          if( value < 32 ) value = 32;
          if( value > 65536 ) value = 65536;
          ak_libakrypt_set_option( "context_manager_size", value );
        }

       /* устанавливаем максимально возможный размер структуры управления контекстами */
        if( ak_libakrypt_load_one_option( localbuffer, "context_manager_max_size = ", &value )) {
          if( value < 4096 ) value = 4096;
          if( value > 2147483647 ) value = 2147483647;
          ak_libakrypt_set_option( "context_manager_max_size", value );
        }

       /* устанавливаем количество циклов в алгоритме pbkdf2 */
        if( ak_libakrypt_load_one_option( localbuffer, "pbkdf2_iteration_count = ", &value )) {
          if( value < 1000 ) value = 1000;
          if( value > 32768 ) value = 32768;
          ak_libakrypt_set_option( "pbkdf2_iteration_count", value );
        }

       /* устанавливаем ресурс ключа выработки имитовставки для алгоритма HMAC */
        if( ak_libakrypt_load_one_option( localbuffer, "hmac_key_counter_resource = ", &value )) {
          if( value < 1024 ) value = 1024;
          if( value > 2147483647 ) value = 2147483647;
          ak_libakrypt_set_option( "hmac_key_count_resource", value );
        }

       /* устанавливаем ресурс ключа алгоритма блочного шифрования Магма */
        if( ak_libakrypt_load_one_option( localbuffer, "magma_cipher_resource = ", &value )) {
          if( value < 1024 ) value = 1024;
          if( value > 2147483647 ) value = 2147483647;
          ak_libakrypt_set_option( "magma_cipher_resource", value );
        }
       /* устанавливаем ресурс ключа алгоритма блочного шифрования Кузнечик */
        if( ak_libakrypt_load_one_option( localbuffer, "kuznechik_cipher_resource = ", &value )) {
          if( value < 1024 ) value = 1024;
          if( value > 2147483647 ) value = 2147483647;
          ak_libakrypt_set_option( "kuznechik_cipher_resource", value );
        }
       /* учет совместимости с другими библиотеками */
        if( ak_libakrypt_load_one_option( localbuffer, "openssl_compability = ", &value )) {
          if(( value < 0 ) || ( value > 1 )) value = 0;
          ak_libakrypt_set_option( "openssl_compability", value );
        }

      } /* далее мы очищаем строку независимо от ее содержимого */
      off = 0;
      memset( localbuffer, 0, 1024 );
    } else localbuffer[off++] = ch;
  }

  /* закрываем */
  ak_file_close(fd);
  return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
 static int ak_libakrypt_write_options( void )
{
  size_t i;
  struct file fd;
  int error = ak_error_ok;
  char hpath[FILENAME_MAX], filename[FILENAME_MAX];

  memset( hpath, 0, FILENAME_MAX );
  memset( filename, 9, FILENAME_MAX );

 /* начинаем последовательно создавать подкаталоги */
  if(( error = ak_libakrypt_get_home_path( hpath, FILENAME_MAX )) != ak_error_ok )
    return ak_error_message( error, __func__, "wrong libakrypt.conf name creation" );

 /* создаем .config */
 #ifdef _WIN32
  ak_snprintf( filename, FILENAME_MAX, "%s\\.config", hpath );
  #ifdef _MSC_VER
   if( _mkdir( filename ) < 0 ) {
  #else
   if( mkdir( filename ) < 0 ) {
  #endif
 #else
  ak_snprintf( filename, FILENAME_MAX, "%s/.config", hpath );
  if( mkdir( filename, S_IRWXU ) < 0 ) {
 #endif
    if( errno != EEXIST ) {
     #ifdef _MSC_VER
       strerror_s( hpath, FILENAME_MAX, errno ); /* помещаем сообщение об ошибке в ненужный буффер */
       return ak_error_message_fmt( ak_error_access_file, __func__,
                                         "wrong creation of %s directory [%s]", filename, hpath );
     #else
      return ak_error_message_fmt( ak_error_access_file, __func__,
                              "wrong creation of %s directory [%s]", filename, strerror( errno ));
     #endif
    }
  }

 /* создаем libakrypt */
 #ifdef _WIN32
  ak_snprintf( hpath, FILENAME_MAX, "%s\\libakrypt", filename );
  #ifdef _MSC_VER
   if( _mkdir( hpath ) < 0 ) {
  #else
   if( mkdir( hpath ) < 0 ) {
  #endif
 #else
  ak_snprintf( hpath, FILENAME_MAX, "%s/libakrypt", filename );
  if( mkdir( hpath, S_IRWXU ) < 0 ) {
 #endif
    if( errno != EEXIST ) {
     #ifdef _MSC_VER
       strerror_s( hpath, FILENAME_MAX, errno ); /* помещаем сообщение об ошибке в ненужный буффер */
       return ak_error_message_fmt( ak_error_access_file, __func__,
                                        "wrong creation of %s directory [%s]", filename, hpath );
     #else
      return ak_error_message_fmt( ak_error_access_file, __func__,
                             "wrong creation of %s directory [%s]", filename, strerror( errno ));
     #endif
    }
  }

 /* теперь начинаем манипуляции с файлом */
 #ifdef _WIN32
  ak_snprintf( filename, FILENAME_MAX, "%s\\libakrypt.conf", hpath );
 #else
  ak_snprintf( filename, FILENAME_MAX, "%s/libakrypt.conf", hpath );
 #endif

  if(( error = ak_file_create_to_write( &fd, filename )) != ak_error_ok )
    return ak_error_message( error, __func__, "wrong creation of libakrypt.conf file");

  for( i = 0; i < ak_libakrypt_options_count(); i++ ) {
    memset( hpath, 0, ak_min( 1024, FILENAME_MAX ));
    ak_snprintf( hpath, FILENAME_MAX - 1, "%s = %d\n", options[i].name, options[i].value );
    if( ak_file_write( &fd, hpath, strlen( hpath )) < 1 ) {
     #ifdef _MSC_VER
      strerror_s( hpath, FILENAME_MAX, errno ); /* помещаем сообщение об ошибке в ненужный буффер */
      ak_error_message_fmt( error = ak_error_write_data, __func__,
                                 "option %s stored with error [%s]", options[i].name, hpath );
     #else
      ak_error_message_fmt( error = ak_error_write_data, __func__,
                      "option %s stored with error [%s]", options[i].name, strerror( errno ));
     #endif
    }
  }
  ak_file_close( &fd );
  if( error == ak_error_ok )
    ak_error_message_fmt( ak_error_ok, __func__, "all options stored in %s file", filename );
 return error;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция последовательно ищет файл `libakrypt.conf` сначала в домашнем каталоге пользователя,
    потом в каталоге, указанном при сборке библиотеки с помощью флага `LIBAKRYPT_CONF`. В случае,
    если ни в одном из указанных мест файл не найден, то функция создает файл `libakrypt.conf`
    в домашнем каталоге пользователя со значениями по-умолчанию.                                   */
/* ----------------------------------------------------------------------------------------------- */
 bool_t ak_libakrypt_load_options( void )
{
 struct file fd;
 int error = ak_error_ok;
 char name[FILENAME_MAX];

/* создаем имя файла, расположенного в домашнем каталоге */
 if(( error = ak_libakrypt_create_filename( name, FILENAME_MAX,
                                                          "libakrypt.conf", 0 )) != ak_error_ok ) {
   ak_error_message( error, __func__, "incorrect name generation for options file");
   return ak_false;
 }
/* пытаемся считать данные из указанного файла */
 if( ak_file_open_to_read( &fd, name ) == ak_error_ok ) {
   if(( error = ak_libakrypt_load_options_from_file( &fd )) == ak_error_ok ) {
     if( ak_libakrypt_get_option( "log_level" ) > ak_log_standard ) {
       ak_error_message_fmt( ak_error_ok, __func__,
                                                 "all options was read from %s file", name );
     }
     return ak_true;
   } else {
         ak_error_message_fmt( error, __func__, "file %s exists, but contains invalid data", name );
       return ak_false;
     }
 }

/* создаем имя файла, расположенного в системном каталоге */
 if(( error = ak_libakrypt_create_filename( name, FILENAME_MAX,
                                                          "libakrypt.conf", 1 )) != ak_error_ok ) {
   ak_error_message( error, __func__, "incorrect name generation for options file");
   return ak_false;
 }
/* пытаемся считать данные из указанного файла */
 if( ak_file_open_to_read( &fd, name ) == ak_error_ok ) {
   if(( error = ak_libakrypt_load_options_from_file( &fd )) == ak_error_ok ) {
     if( ak_libakrypt_get_option( "log_level" ) > ak_log_standard ) {
       ak_error_message_fmt( ak_error_ok, __func__,
                                                 "all options was read from %s file", name );
     }
     return ak_true;
   } else {
       ak_error_message_fmt( error, __func__, "wrong options reading from %s file", name );
       return ak_false;
     }
 } else ak_error_message( ak_error_access_file, __func__,
                         "file libakrypt.conf not found either in home or system directory");

 /* формируем дерево подкаталогов и записываем файл с настройками */
  if(( error = ak_libakrypt_write_options( )) != ak_error_ok ) {
    ak_error_message_fmt( error, __func__, "wrong creation a libakrypt.conf file" );
    return ak_false;
  }

 return ak_true;
}
#endif

/* ----------------------------------------------------------------------------------------------- */
 void ak_libakrypt_log_options( void )
{
 /* выводим сообщение об установленных параметрах библиотеки */
  if( ak_libakrypt_get_option( "log_level" ) >= ak_log_maximum ) {
    size_t i = 0;
    ak_error_message_fmt( ak_error_ok, __func__, "libakrypt version: %s", ak_libakrypt_version( ));
   /* далее мы пропускаем вывод информации об архитектуре,
    поскольку она будет далее тестироваться отдельно     */
    for( i = 1; i < ak_libakrypt_options_count(); i++ ) {
       ak_error_message_fmt( ak_error_ok, __func__,
                                            "option %s is %d", options[i].name, options[i].value );
    }
   }
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_file_open_to_read( ak_file file, const char *filename )
{
#ifdef _WIN32
  struct _stat st;
  if( _stat( filename, &st ) < 0 ) {
#else
  struct stat st;
  if( stat( filename, &st ) < 0 ) {
#endif
    switch( errno ) {
      case EACCES:
        if( ak_log_get_level() >= ak_log_maximum )
          ak_error_message_fmt( ak_error_access_file, __func__,
                                 "incorrect access to file %s [%s]", filename, strerror( errno ));
        return ak_error_access_file;
      default:
        if( ak_log_get_level() >= ak_log_maximum )
          ak_error_message_fmt( ak_error_open_file, __func__ ,
                                     "wrong opening a file %s [%s]", filename, strerror( errno ));
        return ak_error_open_file;
    }
  }

 /* заполняем данные */
  file->size = ( ak_int64 )st.st_size;
 #ifdef LIBAKRYPT_HAVE_WINDOWS_H
  if(( file->hFile = CreateFile( filename,   /* name of the write */
                     GENERIC_READ,           /* open for reading */
                     0,                      /* do not share */
                     NULL,                   /* default security */
                     OPEN_EXISTING,          /* open only existing file */
                     FILE_ATTRIBUTE_NORMAL,  /* normal file */
                     NULL )                  /* no attr. template */
      ) == INVALID_HANDLE_VALUE )
       return ak_error_message_fmt( ak_error_open_file, __func__,
                                     "wrong opening a file %s [%s]", filename, strerror( errno ));
  file->blksize = 4096;
 #else
  if(( file->fd = open( filename, O_SYNC|O_RDONLY )) < 0 )
    return ak_error_message_fmt( ak_error_open_file, __func__ ,
                                     "wrong opening a file %s [%s]", filename, strerror( errno ));
  file->blksize = ( ak_int64 )st.st_blksize;
 #endif

 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_file_create_to_write( ak_file file, const char *filename )
{
 #ifndef LIBAKRYPT_HAVE_WINDOWS_H
  struct stat st;
 #endif

 /* необходимые проверки */
  if(( file == NULL ) || ( filename == NULL ))
    return ak_error_message( ak_error_null_pointer, __func__, "using null pointer" );

  file->size = 0;
 #ifdef LIBAKRYPT_HAVE_WINDOWS_H
  if(( file->hFile = CreateFile( filename,   /* name of the write */
                     GENERIC_WRITE,          /* open for writing */
                     0,                      /* do not share */
                     NULL,                   /* default security */
                     CREATE_ALWAYS,          /* create new file only */
                     FILE_ATTRIBUTE_NORMAL,  /* normal file */
                     NULL )                  /* no attr. template */
     ) == INVALID_HANDLE_VALUE )
      return ak_error_message_fmt( ak_error_create_file, __func__,
                                    "wrong creation a file %s [%s]", filename, strerror( errno ));
   file->blksize = 4096;

 #else  /* мы устанавливаем минимальные права: чтение и запись только для владельца */
  if(( file->fd = creat( filename, S_IRUSR | S_IWUSR )) < 0 )
    return ak_error_message_fmt( ak_error_create_file, __func__,
                                   "wrong creation a file %s [%s]", filename, strerror( errno ));
  if( fstat( file->fd, &st )) {
    close( file->fd );
    return ak_error_message_fmt( ak_error_access_file,  __func__,
                                "incorrect access to file %s [%s]", filename, strerror( errno ));
  } else file->blksize = ( ak_int64 )st.st_blksize;
 #endif

 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_file_close( ak_file file )
{
   file->size = 0;
   file->blksize = 0;
  #ifdef LIBAKRYPT_HAVE_WINDOWS_H
   CloseHandle( file->hFile);
  #else
   if( close( file->fd ) != 0 ) return ak_error_message_fmt( ak_error_close_file, __func__ ,
                                                 "wrong closing a file [%s]", strerror( errno ));
  #endif
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
 ssize_t ak_file_read( ak_file file, ak_pointer buffer, size_t size )
{
 #ifdef LIBAKRYPT_HAVE_WINDOWS_H
  DWORD dwBytesReaden = 0;
  BOOL bErrorFlag = ReadFile( file->hFile, buffer, ( DWORD )size,  &dwBytesReaden, NULL );
  if( bErrorFlag == FALSE ) {
    ak_error_message( ak_error_read_data, __func__, "unable to read from file");
    return 0;
  } else return ( ssize_t ) dwBytesReaden;
 #else
  return read( file->fd, buffer, size );
 #endif
}

/* ----------------------------------------------------------------------------------------------- */
 ssize_t ak_file_write( ak_file file, ak_const_pointer buffer, size_t size )
{
 #ifdef LIBAKRYPT_HAVE_WINDOWS_H
  DWORD dwBytesWritten = 0;
  BOOL bErrorFlag = WriteFile( file->hFile, buffer, ( DWORD )size,  &dwBytesWritten, NULL );
  if( bErrorFlag == FALSE ) {
    ak_error_message( ak_error_write_data, __func__, "unable to write to file");
    return 0;
  } else return ( ssize_t ) dwBytesWritten;
 #else
  return write( file->fd, buffer, size );
 #endif
}

/* ----------------------------------------------------------------------------------------------- */
/*! \hidecallgraph
    \hidecallergraph                                                                               */
/* ----------------------------------------------------------------------------------------------- */
 int ak_log_get_level( void ) { return (int)ak_libakrypt_get_option("log_level"); }

/* ----------------------------------------------------------------------------------------------- */
/*! Все сообщения библиотеки могут быть разделены на три уровня.

    \li Первый уровень аудита определяется константой \ref ak_log_none. На этом уровне выводятся
    сообщения об ошибках, а также минимальный набор сообщений, включающий в себя факт
    успешного тестирования работоспособности криптографических механизмов.

    \li Второй уровень аудита определяется константой \ref ak_log_standard. На этом уровене
    выводятся все сообщения из первого уровня, а также сообщения о фактах использования
    ключевой информации.

    \li Третий (максимальный) уровень аудита определяется константой \ref ak_log_maximum.
    На этом уровне выводятся все сообщения, доступные на первых двух уровнях, а также
    сообщения отладочного характера, позхволяющие прослдедить логику работы функций библиотеки.

    Функция сделана экспортируемой, однако она не позволяет понизить уровень аудита
    по-сравнению с тем значением, которое указано в конфигурационном файле.
    Экспорт функции разрешен с целью отладки приложений, использующих библиотеку.

    \param level Уровень аудита, может принимать значения \ref ak_log_none,
    \ref ak_log_standard и \ref ak_log_maximum

    \note Допускается передавать в функцию любое целое число, не превосходящее 16.
    Однако для всех значений от \ref ak_log_maximum  до 16 поведение функции аудита
    будет одинаковым для. Дополнительный диапазон предназначен для приложений библиотеки.

    \return Функция всегда возвращает \ref ak_error_ok (ноль).                                     */
/* ----------------------------------------------------------------------------------------------- */
 int ak_log_set_level( int level )
{
 int value = ak_max( level, ak_log_get_level( ));

   if( value < 0 ) return ak_libakrypt_set_option("log_level", ak_log_none );
   if( value > 16 ) value = 16;
 return ak_libakrypt_set_option("log_level", value );
}

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.
    \param value Код ошибки, который будет установлен. В случае, если значение value положительно,
    то код ошибки полагается равным величине \ref ak_error_ok (ноль).
    \return Функция возвращает устанавливаемое значение.                                           */
/* ----------------------------------------------------------------------------------------------- */
 int ak_error_set_value( const int value )
{
  return ( ak_errno = value );
}

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.
    \return Функция возвращает текущее значение кода ошибки. Данное значение не является
    защищенным от возможности изменения различными потоками выполнения программы.                  */
/* ----------------------------------------------------------------------------------------------- */
 int ak_error_get_value( void )
{
  return ak_errno;
}

#ifdef LIBAKRYPT_HAVE_SYSLOG_H
/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.
    \param message Выводимое сообщение.
    \return В случае успеха, возвращается ak_error_ok (ноль). В случае возникновения ошибки,
    возвращается ее код.                                                                           */
/* ----------------------------------------------------------------------------------------------- */
 int ak_function_log_syslog( const char *message )
{
 #ifdef __linux__
   int priority = LOG_AUTHPRIV | LOG_NOTICE;
 #else
   int priority = LOG_USER;
 #endif
  if( message != NULL ) syslog( priority, "%s", message );
 return ak_error_ok;
}
#endif

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.
    \param message Выводимое сообщение
    \return В случае успеха, возвращается ak_error_ok (ноль). В случае возникновения ошибки,
    возвращается ее код.                                                                           */
/* ----------------------------------------------------------------------------------------------- */
 int ak_function_log_stderr( const char *message )
{
  if( message != NULL ) fprintf( stderr, "%s\n", message );
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция устанавливает в качестве основного обработчика
    вывода сообщений функцию, задаваемую указателем function. Если аргумент function равен NULL,
    то используется функция по-умолчанию.
    Выбор того, какая именно функция будет установлена по-умолчанию, не фискирован.
    В текущей версии библиотеки он зависит от используемой операционной системы, например,
    под ОС Linux это вывод с использованием демона syslogd.

    \b Внимание. Функция экспортируется.

    \param function Указатель на функцию вывода сообщений.
    \return Функция всегда возвращает ak_error_ok (ноль).                                          */
/* ----------------------------------------------------------------------------------------------- */
 int ak_log_set_function( ak_function_log *function )
{
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_lock( &ak_function_log_default_mutex );
#endif
  if( function != NULL ) {
    ak_function_log_default = function;
    if( function == ak_function_log_stderr ) { /* раскрашиваем вывод кодов ошибок */
      #ifndef _WIN32
        ak_error_code_start_string = ak_error_code_start_red_string;
        ak_error_code_end_string = ak_error_code_end_red_string;
      #endif
    }
  }
   else {
    #ifdef LIBAKRYPT_HAVE_SYSLOG_H
      ak_function_log_default = ak_function_log_syslog;
    #else
      ak_function_log_default = ak_function_log_stderr;
    #endif
   }
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_unlock( &ak_function_log_default_mutex );
#endif
  return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция использует установленную ранее функцию-обработчик сообщений. Если сообщение,
    или обработчик не определены (равны NULL) возвращается код ошибки.

    \b Внимание. Функция экспортируется.

    \param message выводимое сообщение
    \return в случае успеха, возвращается ak_error_ok (ноль). В случае возникновения ошибки,
    возвращается ее код.                                                                           */
/* ----------------------------------------------------------------------------------------------- */
 int ak_log_set_message( const char *message )
{
  int result = ak_error_ok;
  if( ak_function_log_default == NULL ) return ak_error_set_value( ak_error_undefined_function );
  if( message == NULL ) {
    return ak_error_message( ak_error_null_pointer, __func__ , "using a NULL string for message" );
  } else {
          #ifdef LIBAKRYPT_HAVE_PTHREAD
           pthread_mutex_lock( &ak_function_log_default_mutex );
          #endif
           result = ak_function_log_default( message );
          #ifdef LIBAKRYPT_HAVE_PTHREAD
           pthread_mutex_unlock( &ak_function_log_default_mutex );
          #endif
      return result;
    }
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \param str Строка, в которую помещается результат (сообщение)
    \param size Максимальный размер помещаемого в строку str сообщения
    \param format Форматная строка, в соответствии с которой формируется сообщение

    \return Функция возвращает значение, которое вернул вызов системной (библиотечной) функции
    форматирования строки.                                                                         */
/* ----------------------------------------------------------------------------------------------- */
 int ak_snprintf( char *str, size_t size, const char *format, ... )
{
  int result = 0;
  va_list args;
  va_start( args, format );

 #ifdef _MSC_VER
  #if _MSC_VER > 1310
    result = _vsnprintf_s( str, size, size, format, args );
  #else
    result = _vsnprintf( str, size, format, args );
  #endif
 #else
  result = vsnprintf( str, size, format, args );
 #endif
  va_end( args );
 return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция устанавливает текущее значение кода ошибки, формирует строку специального вида и
    выводит сформированную строку в логгер с помомощью функции ak_log_set_message().

    \b Внимание. Функция экспортируется.

    \param code Код ошибки
    \param message Читаемое (понятное для пользователя) сообщение
    \param function Имя функции, вызвавшей ошибку

    \hidecallgraph
    \hidecallergraph
    \return Функция возвращает установленный код ошибки.                                           */
/* ----------------------------------------------------------------------------------------------- */
 int ak_error_message( const int code, const char *function, const char *message )
{
 /* здесь мы выводим в логгер строку вида [pid] function: message (code: n)                        */
  char error_event_string[1024];
  const char *br0 = "", *br1 = "():", *br = NULL;

  memset( error_event_string, 0, 1024 );
  if(( function == NULL ) || strcmp( function, "" ) == 0 ) br = br0;
    else br = br1;

#ifdef LIBAKRYPT_HAVE_UNISTD_H
  if( code < 0 ) ak_snprintf( error_event_string, 1023, "[%d] %s%s %s (%scode: %d%s)",
                              getpid(), function, br, message,
                                    ak_error_code_start_string, code, ak_error_code_end_string );
   else ak_snprintf( error_event_string, 1023, "[%d] %s%s %s", getpid(), function, br, message );
#else
 #ifdef _MSC_VER
  if( code < 0 ) ak_snprintf( error_event_string, 1023, "[%d] %s%s %s (code: %d)",
                                             GetCurrentProcessId(), function, br, message, code );
   else ak_snprintf( error_event_string, 1023, "[%d] %s%s %s",
                                                   GetCurrentProcessId(), function, br, message );
 #else
   #error Unsupported path to compile, sorry ...
 #endif
#endif
  ak_log_set_message( error_event_string );
 return ak_error_set_value( code );
}

/* ----------------------------------------------------------------------------------------------- */
/*! \b Внимание. Функция экспортируется.
    \param code Код ошибки
    \param function Имя функции, вызвавшей ошибку
    \param format Форматная строка, в соответствии с которой формируется сообщение
    \hidecallgraph
    \hidecallergraph
    \return Функция возвращает установленный код ошибки.                                           */
/* ----------------------------------------------------------------------------------------------- */
 int ak_error_message_fmt( const int code, const char *function, const char *format, ... )
{
  char message[256];
  va_list args;
  va_start( args, format );
  memset( message, 0, 256 );

 #ifdef _MSC_VER
  #if _MSC_VER > 1310
    _vsnprintf_s( message, 256, 256, format, args );
  #else
    _vsnprintf( message, 256, format, args );
  #endif
 #else
   vsnprintf( message, 256, format, args );
 #endif
   va_end( args );
 return ak_error_message( code, function, message );
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция рассматривает область памяти, на которую указывает указатель ptr, как массив
    последовательно записанных байт фиксированной длины, и
    последовательно выводит в статический буффер значения, хранящиеся в заданной области памяти.
    Значения выводятся в шестнадцатеричной системе счисления.

    Пример использования.
  \code
    ak_uint8 data[5] = { 1, 2, 3, 4, 5 };
    ak_uint8 *str = ak_ptr_to_hexstr( data, 5, ak_false );
    if( str != NULL ) printf("%s\n", str );
  \endcode

    @param ptr Указатель на область памяти
    @param ptr_size Размер области памяти (в байтах)
    @param reverse Последовательность вывода байт в строку. Если reverse равно \ref ak_false,
    то байты выводятся начиная с младшего к старшему.  Если reverse равно \ref ak_true, то байты
    выводятся начиная от старшего к младшему (такой способ вывода принят при стандартном выводе
    чисел: сначала старшие разряды, потом младшие).

    @return Функция возвращает указатель на статическую строку. В случае ошибки конвертации,
    либо в случае нехватки статической памяти, возвращается NULL.
    Код ошибки может быть получен с помощью вызова функции ak_error_get_value().                   */
/* ----------------------------------------------------------------------------------------------- */
 char *ak_ptr_to_hexstr( ak_const_pointer ptr, const size_t ptr_size, const bool_t reverse )
{
  size_t len = 1 + (ptr_size << 1);
  ak_uint8 *data = ( ak_uint8 * ) ptr;
  size_t idx = 0, js = 0, start = 0, offset = 2;

  if( ptr == NULL ) {
    ak_error_message( ak_error_null_pointer, __func__ , "using null pointer to data" );
    return NULL;
  }
  if( ptr_size <= 0 ) {
    ak_error_message( ak_error_zero_length, __func__ , "using data with zero or negative length" );
    return NULL;
  }
 /* если возвращаемое значение функции обрабатывается, то вывод предупреждения об ошибке излишен */
  if( sizeof( ak_ptr_to_hexstr_static_buffer ) < len ) return NULL;

  memset( ak_ptr_to_hexstr_static_buffer, 0, sizeof( ak_ptr_to_hexstr_static_buffer ));
  if( reverse ) { // движение в обратную сторону - от старшего байта к младшему
    start = len-3; offset = -2;
  }
  for( idx = 0, js = start; idx < ptr_size; idx++, js += offset ) {
     char str[4];
     ak_snprintf( str, 3, "%02x", data[idx] );
     memcpy( ak_ptr_to_hexstr_static_buffer+js, str, 2 );
  }

 return ak_ptr_to_hexstr_static_buffer;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция рассматривает область памяти, на которую указывает указатель ptr, как массив
    последовательно записанных байт фиксированной длины. Функция выделяет в оперативной памяти
    массив необходимого размера и последовательно выводит в него значения,
    хранящиеся в заданной области памяти. Значения выводятся в шестнадцатеричной системе счисления.

    Выделенная область памяти должна быть позднее удалена с помощью вызова функции free().

    Пример использования.
  \code
    ak_uint8 data[1000] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    ak_uint8 *str = ak_ptr_to_hexstr_alloc( data, sizeof( data ), ak_false );
    if( str != NULL ) printf("%s\n", str );
    free( str );
  \endcode

    @param ptr Указатель на область памяти
    @param ptr_size Размер области памяти (в байтах)
    @param reverse Последовательность вывода байт в строку. Если reverse равно \ref ak_false,
    то байты выводятся начиная с младшего к старшему.  Если reverse равно \ref ak_true, то байты
    выводятся начиная от старшего к младшему (такой способ вывода принят при стандартном выводе
    чисел: сначала старшие разряды, потом младшие).

    @return Функция возвращает указатель на статическую строку. В случае ошибки конвертации,
    либо в случае нехватки статической памяти, возвращается NULL.
    Код ошибки может быть получен с помощью вызова функции ak_error_get_value().                   */
/* ----------------------------------------------------------------------------------------------- */
 char *ak_ptr_to_hexstr_alloc( ak_const_pointer ptr, const size_t ptr_size, const bool_t reverse )
{
  char *result = NULL;
  size_t len = 1 + (ptr_size << 1);
  ak_uint8 *data = ( ak_uint8 * ) ptr;
  size_t idx = 0, js = 0, start = 0, offset = 2;

  if( ptr == NULL ) {
    ak_error_message( ak_error_null_pointer, __func__ , "using null pointer to data" );
    return NULL;
  }
  if( ptr_size <= 0 ) {
    ak_error_message( ak_error_zero_length, __func__ , "using data with zero or negative length" );
    return NULL;
  }
  if(( result = malloc( len )) == NULL ) {
    ak_error_message( ak_error_out_of_memory, __func__ , "incorrect memory allocation" );
    return NULL;
  }

  memset( result, 0, len );
  if( reverse ) { // движение в обратную сторону - от старшего байта к младшему
    start = len-3; offset = -2;
  }
  for( idx = 0, js = start; idx < ptr_size; idx++, js += offset ) {
     char str[4];
     ak_snprintf( str, 3, "%02x", data[idx] );
     memcpy( result+js, str, 2 );
  }

 return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Конвертация символа в целочисленное значение                                            */
/* ----------------------------------------------------------------------------------------------- */
 inline static ak_uint32 ak_xconvert( const char c )
{
    switch( c )
   {
      case 'a' :
      case 'A' : return 10;
      case 'b' :
      case 'B' : return 11;
      case 'c' :
      case 'C' : return 12;
      case 'd' :
      case 'D' : return 13;
      case 'e' :
      case 'E' : return 14;
      case 'f' :
      case 'F' : return 15;
      case '0' : return 0;
      case '1' : return 1;
      case '2' : return 2;
      case '3' : return 3;
      case '4' : return 4;
      case '5' : return 5;
      case '6' : return 6;
      case '7' : return 7;
      case '8' : return 8;
      case '9' : return 9;
      default : ak_error_set_value( ak_error_undefined_value ); return 0;
 }
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция преобразует строку символов, содержащую последовательность шестнадцатеричных цифр,
    в массив данных. Строка символов должна быть строкой, оканчивающейся нулем (NULL string).

    @param hexstr Строка символов.
    @param ptr Указатель на область памяти (массив), в которую будут размещаться данные.
    @param size Максимальный размер памяти (в байтах), которая может быть помещена в массив.
    Если исходная строка требует больший размер, то возбуждается ошибка.
    @param reverse Последовательность считывания байт в память. Если reverse равно \ref ak_false
    то первые байты строки (с младшими индексами) помещаются в младшие адреса, а старшие байты -
    в старшие адреса памяти. Если reverse равно \ref ak_true, то производится разворот,
    то есть обратное преобразование при котором элементы строки со старшиси номерами помещаются
    в младшие разряды памяти (такое представление используется при считывании больших целых чисел).

    @return В случае успеха возвращается ноль. В противном случае, в частности,
                      когда длина строки превышает размер массива, возвращается код ошибки.        */
/* ----------------------------------------------------------------------------------------------- */
 int ak_hexstr_to_ptr( const char *hexstr, ak_pointer ptr, const size_t size, const bool_t reverse )
{
  int i = 0;
  ak_uint8 *bdata = ptr;
  size_t len = 0;

  if( hexstr == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                                             "using null pointer to a hex string" );
  if( ptr == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                                                 "using null pointer to a buffer" );
  if( size == 0 ) return ak_error_message( ak_error_zero_length, __func__,
                                                          "using zero value for length of buffer" );
  len = strlen( hexstr );
  if( len&1 ) len++;
  len >>= 1;
  if( size < len ) return ak_error_message( ak_error_wrong_length, __func__ ,
                                                               "using a buffer with small length" );

  memset( ptr, 0, size ); // перед конвертацией мы обнуляем исходные данные
  ak_error_set_value( ak_error_ok );
  if( reverse ) {
    for( i = strlen( hexstr )-2, len = 0; i >= 0 ; i -= 2, len++ ) {
       bdata[len] = (ak_xconvert( hexstr[i] ) << 4) + ak_xconvert( hexstr[i+1] );
    }
    if( i == -1 ) bdata[len] = ak_xconvert( hexstr[0] );
  } else {
        for( i = 0, len = 0; i < (int) strlen( hexstr ); i += 2, len++ ) {
           bdata[len] = (ak_xconvert( hexstr[i] ) << 4);
           if( i < (int) strlen( hexstr )-1 ) bdata[len] += ak_xconvert( hexstr[i+1] );
        }
    }
 return ak_error_get_value();
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция используется для определения размера буффера, который может сохранить данные,
 *  записанные в виде шестнадцатеричной строки. Может использоваться совместно с функцией
 *  ak_hexstr_to_ptr().
 *  Строка символов должна быть строкой, оканчивающейся нулем (NULL string). При этом проверка того,
 *  что строка действительно содержит только шестнадцатеричные символы, не проводится.

    @param hexstr Строка символов.
    @return В случае успеха возвращается длина буффера. В противном случае,
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 ssize_t ak_hexstr_size( const char *hexstr )
{
  ssize_t len = 0;
  if( hexstr == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                                             "using null pointer to a hex string" ); 
  len = ( ssize_t ) strlen( hexstr );
  if( len&1 ) len++;
  len >>= 1;

 return len;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция сравнивает две области памяти одного размера, на которые указывают аргументы функции.

    Пример использования функции (результат выполнения функции должен быть \ref ak_false).
  \code
    ak_uint8 data_left[5] = { 1, 2, 3, 4, 5 };
    ak_uint8 data_right[5] = { 1, 2, 3, 4, 6 };

    if( ak_ptr_is_equal( data_left, data_right, 5 )) printf("Is equal");
     else printf("Not equal");
  \endcode

    @param left Указатель на область памяти, участвующей в сравнении слева.
    @param right Указатель на область пямяти, участвующей в сравнении справа.
    @param size Размер области, для которой производяится сравнение.
    @return Если данные идентичны, то возвращается \ref ak_true.
    В противном случае, а также в случае возникновения ошибки, возвращается \ref ak_false.
    Код ошибки может быть получен с помощью выщова функции ak_error_get_value().                   */
/* ----------------------------------------------------------------------------------------------- */
 bool_t ak_ptr_is_equal( ak_const_pointer left, ak_const_pointer right, const size_t size )
{
  size_t i = 0;
  bool_t result = ak_true;
  const ak_uint8 *lp = left, *rp = right;

  if(( left == NULL ) || ( right == NULL )) {
    ak_error_message( ak_error_null_pointer, __func__, "using a null pointer" );
    return ak_false;
  }

  for( i = 0; i < size; i++ )
     if( lp[i] != rp[i] ) result = ak_false;

  return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция сравнивает две области памяти одного размера, на которые указывают аргументы функции.
 *
    @param left Указатель на область памяти, участвующей в сравнении слева.
    Это данные, как правило, которые вычислены в ходе выполнения программы.
    @param right Указатель на область пямяти, участвующей в сравнении справа.
    Это константные данные, с котороыми производится сравнение.
    @param size Размер области, для которой производяится сравнение.
    @return Если данные идентичны, то возвращается \ref ak_true.
    В противном случае, а также в случае возникновения ошибки, возвращается \ref ak_false.
    Код ошибки может быть получен с помощью выщова функции ak_error_get_value().                   */
/* ----------------------------------------------------------------------------------------------- */
 bool_t ak_ptr_is_equal_with_log( ak_const_pointer left, ak_const_pointer right, const size_t size )
{
  size_t i = 0;
  bool_t result = ak_true;
  const ak_uint8 *lp = left, *rp = right;
  char buffer[1024];

  if(( left == NULL ) || ( right == NULL )) {
    ak_error_message( ak_error_null_pointer, __func__, "using a null pointer" );
    return ak_false;
  }

  for( i = 0; i < size; i++ ) {
     if( lp[i] != rp[i] ) {
       result = ak_false;
       if( i < ( sizeof( buffer ) >> 1 )) { buffer[2*i] = buffer[2*i+1] = '^'; }
      } else {
         if( i < ( sizeof( buffer ) >> 1 )) { buffer[2*i] = buffer[2*i+1] = ' '; }
        }
  }
  buffer[ ak_min( size << 1, 1022 ) ] = 0;

  if( result == ak_false ) {
    ak_error_message( ak_error_ok, "", "" ); /* пустая строка */
    ak_error_message_fmt( ak_error_ok, "", "%s (calculated data)",
                                                         ak_ptr_to_hexstr( left, size, ak_false ));
    ak_error_message_fmt( ak_error_ok, "", "%s (const value)",
                                                        ak_ptr_to_hexstr( right, size, ak_false ));
    ak_error_message_fmt( ak_error_ok, "", "%s", buffer );
  }

 return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! @param pass Строка, в которую будет помещен пароль. Память под данную строку должна быть
    выделена заранее. Если в данной памяти хранились какие-либо данные, то они будут полностью
    уничтожены.
    @param psize Максимально возможная длина пароля. При этом величина psize-1 задает
    максимально возможную длиину пароля, поскольку пароль всегда завершается нулевым символом.
    Таким образом длина пароля, после его чтения, может быть получена с помощью функции strlen().

    \b Внимание. В случае ввода пароля нулевой длины функция возвращает ошибку с кодом
    \ref ak_error_terminal

    @return В случае успеха функция возвращает значение \ref ak_error_ok. В случае возникновения
    ошибки возвращается ее код.                                                                    */
/* ----------------------------------------------------------------------------------------------- */
 int ak_password_read( char *pass, const size_t psize )
{
   size_t len = 0;
   int error = ak_error_ok;

 #ifndef LIBAKRYPT_HAVE_TERMIOS_H
  #ifdef _WIN32
   char c = 0;
   DWORD mode, count;
   HANDLE ih = GetStdHandle( STD_INPUT_HANDLE  );
   if( !GetConsoleMode( ih, &mode ))
     return ak_error_message( ak_error_terminal, __func__, "not connected to a console" );
   SetConsoleMode( ih, mode & ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT ));

   memset( pass, 0, psize );
   while( ReadConsoleA( ih, &c, 1, &count, NULL) && (c != '\r') && (c != '\n') && (len < psize-1) ) {
     pass[len]=c;
     len++;
   }
   pass[len]=0;

   /* восстанавливаем настройки консоли */
   SetConsoleMode( ih, mode );
   if(( len = strlen( pass )) < 1 )
     return ak_error_message( ak_error_zero_length, __func__ , "input a very short password");
   return error;

  #endif
   return ak_error_undefined_function;

 #else
  /* обрабатываем терминал */
   struct termios ts, ots;

   tcgetattr( STDIN_FILENO, &ts);   /* получаем настройки терминала */
   ots = ts;
   ts.c_cc[ VTIME ] = 0;
   ts.c_cc[ VMIN  ] = 1;
   ts.c_iflag &= ~( BRKINT | INLCR | ISTRIP | IXOFF ); // ICRNL | IUTF8
   ts.c_iflag |=    IGNBRK;
   ts.c_oflag &= ~( OPOST );
   ts.c_cflag &= ~( CSIZE | PARENB);
   ts.c_cflag |=    CS8;
   ts.c_lflag &= ~( ECHO | ICANON | IEXTEN | ISIG );
   tcsetattr( STDIN_FILENO, TCSAFLUSH, &ts );
   tcgetattr( STDIN_FILENO, &ts ); /* проверяем, что все установилось */
   if( ts.c_lflag & ECHO ) {
        ak_error_message( error = ak_error_terminal, __func__, "failed to turn off echo" );
        goto lab_exit;
   }

   memset( pass, 0, psize );
   fgets( pass, psize, stdin );
   if(( len = strlen( pass )) < 2 )
     ak_error_message( error = ak_error_zero_length, __func__ , "input a very short password");
   if( len > 0 ) pass[len-1] = 0;
    else pass[0] = 0;

  /* убираем за собой и восстанавливаем настройки */
   lab_exit: tcsetattr( STDIN_FILENO, TCSANOW, &ots );
   return error;
 #endif

 /* некорректный путь компиляции исходного текста функции */
 return ak_error_undefined_function;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \details Используемся модифицированный алгоритм,
    заменяющий обычное модульное сложение на операцию порязрядного сложения по модулю 2.
    Такая замена не только не изменяет статистические свойства алгоритма, но и позволяет
    вычислять контрольную сумму от ключевой информации в не зависимотси от значения используемой маски.

    \param data Указатель на область пямяти, для которой вычисляется контрольная сумма.
    \param size Размер области (в октетах).
    \param out Область памяти куда помещается результат.
    Память (32 бита) должна быть выделена заранее.
    \return Функция возвращает \ref ak_error_ok (ноль) в случае успешного вычисления результата.
    В противном случае возвращается код ошибки.                                                    */
/* ----------------------------------------------------------------------------------------------- */
 int ak_ptr_fletcher32_xor( ak_const_pointer data, const size_t size, ak_uint32 *out )
{
  ak_uint32 sB = 0;
  size_t idx = 0, cnt = size ^( size&0x1 );
  const ak_uint8 *ptr = data;

  if( data == NULL ) return ak_error_message( ak_error_null_pointer, __func__,
                                                              "using null pointer to input data" );
  if( size == 0 ) return ak_error_message( ak_error_zero_length, __func__,
                                                                        "using zero length data" );
  if( out == NULL )  return ak_error_message( ak_error_null_pointer, __func__,
                                                           "using null pointer to output buffer" );
 /* основной цикл по четному числу байт  */
  *out = 0;
  while( idx < cnt ) {
    *out ^= ( ptr[idx] | (ak_uint32)(ptr[idx+1] << 8));
    sB = (( sB ^= *out )&0x8000) ? (sB << 1)^0x8BB7 : (sB << 1);
    idx+= 2;
  }

 /* дополняем последний (нечетный) байт */
  if( idx != size ) {
    *out ^= ptr[idx];
    sB = (( sB ^= *out )&0x8000) ? (sB << 1)^0x8BB7 : (sB << 1);
  }
  *out ^= ( sB << 16 );
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция реализует алгоритм Флетчера с измененным модулем простого числа
    подробное описание см. [здесь]( https://en.wikipedia.org/wiki/Fletcher%27s_checksum#Fletcher-32).

    \param data Указатель на область пямяти, для которой вычисляется контрольная сумма.
    \param size Размер области (в октетах).
    \param out Область памяти куда помещается результат.
    Память (32 бита) должна быть выделена заранее.
    \return Функция возвращает \ref ak_error_ok (ноль) в случае успешного вычисления результата.
    В противном случае возвращается код ошибки.                                                    */
/* ----------------------------------------------------------------------------------------------- */
 int ak_ptr_fletcher32( ak_const_pointer data, const size_t size, ak_uint32 *out )
{
 ak_uint32 c0 = 0, c1 = 0;
 const ak_uint32 *ptr = ( const ak_uint32 *) data;
 size_t i, len = size >> 2, tail = size - ( len << 2 );

  if( data == NULL ) return ak_error_message( ak_error_null_pointer, __func__,
                                                              "using null pointer to input data" );
  if( size == 0 ) return ak_error_message( ak_error_zero_length, __func__,
                                                                        "using zero length data" );
  if( out == NULL )  return ak_error_message( ak_error_null_pointer, __func__,
                                                           "using null pointer to output buffer" );
 /* основной цикл обработки 32-х битных слов */
  for( i = 0; i < len; i++ ) {
    c0 += ptr[i];
    c1 += c0;
  }

 /* обрабатываем хвост */
  if( tail ) {
    ak_uint32 idx = 0, c2 = 0;
    while( tail-- ) {
        c2 <<= 8;
        c2 += (( const ak_uint8 * )data)[(len << 2)+(idx++)];
    }
    c0 += c2;
    c1 += c0;
  }

 *out = ( c1&0xffff ) << 16 | ( c0&0xffff );
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! эта реализация востребована только при сборке mingw и gcc под Windows                          */
/* ----------------------------------------------------------------------------------------------- */
#ifdef _WIN32
 #ifndef _MSC_VER
 unsigned long long __cdecl _byteswap_uint64( unsigned long long _Int64 )
{
#if defined(_AMD64_) || defined(__x86_64__)
  unsigned long long retval;
  __asm__ __volatile__ ("bswapq %[retval]" : [retval] "=rm" (retval) : "[retval]" (_Int64));
  return retval;
#elif defined(_X86_) || defined(__i386__)
  union {
    long long int64part;
    struct {
      unsigned long lowpart;
      unsigned long hipart;
    } parts;
  } retval;
  retval.int64part = _Int64;
  __asm__ __volatile__ ("bswapl %[lowpart]\n"
    "bswapl %[hipart]\n"
    : [lowpart] "=rm" (retval.parts.hipart), [hipart] "=rm" (retval.parts.lowpart)  : "[lowpart]" (retval.parts.lowpart), "[hipart]" (retval.parts.hipart));
  return retval.int64part;
#else
  unsigned char *b = (unsigned char *)&_Int64;
  unsigned char tmp;
  tmp = b[0];
  b[0] = b[7];
  b[7] = tmp;
  tmp = b[1];
  b[1] = b[6];
  b[6] = tmp;
  tmp = b[2];
  b[2] = b[5];
  b[5] = tmp;
  tmp = b[3];
  b[3] = b[4];
  b[4] = tmp;
  return _Int64;
#endif
}
 #endif
#endif

/* ----------------------------------------------------------------------------------------------- */
/*! \example example-hello.c                                                                       */
/*! \example example-log.c                                                                         */
/* ----------------------------------------------------------------------------------------------- */
/*                                                                                     ak_tools.c  */
/* ----------------------------------------------------------------------------------------------- */
