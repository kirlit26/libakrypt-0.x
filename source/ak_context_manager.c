/* ----------------------------------------------------------------------------------------------- */
/*  Copyright (c) 2017 - 2019 by Axel Kenzo, axelkenzo@mail.ru                                     */
/*                                                                                                 */
/*  Файл ak_context_manager.c                                                                      */
/*  - содержит реализацию функций для управления контекстами.                                      */
/* ----------------------------------------------------------------------------------------------- */
 #include <ak_hmac.h>
 #include <ak_bckey.h>
 #include <ak_tools.h>
 #include <ak_context_manager.h>

/* ----------------------------------------------------------------------------------------------- */
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
#ifdef LIBAKRYPT_HAVE_PTHREAD
 #include <pthread.h>
#endif

/* ----------------------------------------------------------------------------------------------- */
/*                                   класс ak_context_node                                         */
/* ----------------------------------------------------------------------------------------------- */
/*! \brief Функция возвращает oid из указателя на объект (контекст), тип которого задается
    параметром engine.
    @param ctx Контекст объетка.
    @param engine Тип контекста: блочный шифр, функия хеширования, массив с данными и т.п.
    @return Функция возвращает указатель на OID объекта.
    В случае возникновения ошибки возвращается NULL. Код ошибки может быть получен с помощью
    вызова функции ak_error_get_value().                                                           */
/* ----------------------------------------------------------------------------------------------- */
 static ak_oid ak_context_node_get_context_oid( const ak_pointer ctx, const oid_engines_t engine )
{
  ak_oid oid = NULL;
  if( ctx == NULL ) {
    ak_error_message( ak_error_null_pointer, __func__, "using null pointer to context" );
    return NULL;
  }

 /* вытаскиваем oid, присвоенный при создании контекста */
  switch( engine ) {
    case random_generator:
                  oid = (( ak_random ) ctx )->oid;
                  break;
    case hash_function:
                  oid = (( ak_hash ) ctx )->oid;
                  break;
    case hmac_function:
                  oid = (( ak_hmac ) ctx )->key.oid;
                  break;
    case block_cipher:
                  oid = (( ak_bckey ) ctx )->key.oid;
                  break;
    default: oid = NULL;
  }

 /* проверка найденного */
  if( oid == NULL ) {
    ak_error_message( ak_error_oid_engine, __func__, "using unsupported oid engine" );
   } else {
       /* проверка существования данного адреса */
        if( !ak_oid_context_check( oid )) {
          oid = NULL;
          ak_error_message( ak_error_wrong_oid, __func__, "wrong pointer to context" );
        }
     }

 return oid;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция создает новый элемент структуры управления контекстами, заполняя его поля значениями,
    передаваемыми в качестве аргументов функции.

    @param ctx Контекст, который будет храниться в структуре управления контекстами.
    К моменту вызова функции контекст должен быть отличен от NULL, создан и, при необходимости,
    инициализирован.
    @param handle Дескриптор контекста, величина по которой пользовать может получить доступ
    к функциям, реализующим действия с контекстом.
    @param engine тип контекста: блочный шифр, функия хеширования, массив с данными и т.п.
    @param description пользовательское описание контекста, может быть NULL
    @return Функция возвращает указатель на созданный элемент структуры управления контекстами.
    В случае возникновения ошибки возвращается NULL. Код ошибки может быть получен с помощью
    вызова функции ak_error_get_value().                                                           */
/* ----------------------------------------------------------------------------------------------- */
 ak_context_node ak_context_node_new( const ak_pointer ctx, const ak_handle handle,
                                               const oid_engines_t engine, const char *description )
{
  size_t len = 0;
  ak_oid oid = NULL;
  int error = ak_error_ok;
  ak_context_node node = NULL;

 /* выполняем необходимые проверки */
  if(( oid = ak_context_node_get_context_oid( ctx, engine )) == NULL ) {
    ak_error_message_fmt( error = ak_error_get_value(), __func__ ,
                                           "incorrect oid extracting from given context pointer" );
    return NULL;
  }

 /* создаем контекст */
  if(( node = malloc( sizeof( struct context_node ))) == NULL ) {
    ak_error_message( ak_error_out_of_memory, __func__,
                                          "wrong memory allocation for new context manager node" );
    return NULL;
  }

 /* разбираемся с описанием: выделяем память и копируем */
  len = 1 + ak_min( strlen( description ), 127 );
  if(( description == NULL ) || ( len == 1 )) node->description = NULL;
    else {
      if(( node->description = malloc( len )) != NULL ) {
        memset( node->description, 0, len );
        memcpy( node->description, description, len-1 );
      }
  }

 /* присваиваем остальные данные */
  node->ctx = ctx;
  node->handle = handle;
  node->oid = oid;
  node->status = node_is_equal;

 return node;
}

/* ----------------------------------------------------------------------------------------------- */
/*! @param pointer указатель на элемент структуры управления контекстами.
    @return Функция всегда возвращает NULL.                                                        */
/* ----------------------------------------------------------------------------------------------- */
 ak_pointer ak_context_node_delete( ak_pointer pointer )
{
  ak_context_node node = ( ak_context_node ) pointer;

  if( node == NULL ) {
    ak_error_message( ak_error_null_pointer, __func__,
                                       "wrong deleting a null pointer to context manager node" );
    return NULL;
  }

 /* уничтожаем описание */
  if( node->description != NULL ) free( node->description );
 /* уничтожаем контекст */
  if( node->oid->func.delete != NULL ) {
    if( node->ctx != NULL )
      node->ctx = (( ak_function_free_object *) node->oid->func.delete )( node->ctx );
  }
  node->handle = ak_error_wrong_handle;
  node->oid = NULL;
  node->status = node_undefined;
  free( node );

 return NULL;
}

/* ----------------------------------------------------------------------------------------------- */
/*                                    класс ak_context_manager                                     */
/* ----------------------------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Мьютекс для блокировки структуры управления контекстами */
#ifdef LIBAKRYPT_HAVE_PTHREAD
 static pthread_mutex_t ak_context_manager_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ----------------------------------------------------------------------------------------------- */
/*! Функция инициализирует структуру управления контекстами, присваивая ее полям значения,
    необходимые для обеспечения корректной работы.
    Начальное значение ожидаемого структурой количества контекстов, с которыми будет произодится
    работа, является внешним параметром библиотеки. Данное значение устанавливается
    в файле `libakrypt.conf` (см. раздел \ref construction_options).

    @param manager Указатель на структуру управления ключами
    @return В случае успеха функция возвращает \ref ak_error_ok (ноль). В противном случае
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 int ak_context_manager_create( ak_context_manager manager )
{
  size_t idx = 0;
  int error = ak_error_ok;

  if( manager == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                            "using a null pointer to context manager structure" );
 /* инициализируем генератор ключей */
#if defined(__unix__) || defined(__APPLE__)
  if(( error = ak_random_context_create_urandom( &manager->key_generator )) != ak_error_ok )
    return ak_error_message( error, __func__,
                            "wrong initialization of /dev/urandom for random number generation" );
#else
 #ifdef _WIN32
   if(( error = ak_random_context_create_winrtl( &manager->key_generator )) != ak_error_ok ) {
     ak_error_message( error, __func__,
                         "wrong initialization a random generator from default crypto provider" );
     ak_error_message( ak_error_ok, __func__, "trying to use lcg generator" );
     if(( error = ak_random_context_create_lcg( &manager->key_generator )) != ak_error_ok )
       return ak_error_message( error, __func__,
                                "wrong initialization of all types of random generators" );
   }
 #else
   #error ak_context_manager_create(): using a non defined path of compilation
 #endif
#endif

 /* инициализируем указатели контекстов */
  if(( manager->size = ( size_t )ak_libakrypt_get_option("context_manager_size")) == 0 )
    manager->size = 32;

  if(( manager->array = malloc( manager->size*sizeof( ak_pointer ))) == NULL ) {
    ak_context_manager_destroy( manager );
    return ak_error_message( ak_error_out_of_memory, __func__ ,
                                            "wrong memory allocation for context manager nodes" );
  }
  for( idx = 0; idx < manager->size; idx++ ) manager->array[idx] = NULL;

 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция удаляет структуру управления контекстами, уничтожая данные, которыми она владеет.
    При выполнении функции:
    - уничтожаются контексты, хранящиеся в структуре,
    - уничтожается генератор псевдо-случайных чисел, использовавшийся для генерации
      ключевой информации.

    @param manager Указатель на структуру управления ключами
    @return В случае успеха функция возвращает \ref ak_error_ok (ноль). В противном случае
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 int ak_context_manager_destroy( ak_context_manager manager )
{
  size_t idx = 0;
  int error = ak_error_ok;

  if( manager == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                            "using a null pointer to context manager structure" );
  if( manager->array == NULL ) {
    ak_error_message( error = ak_error_undefined_value, __func__ ,
                                                   "cleaning context manager with empty memory" );
  } else {
          /* удаляем ключевые структуры */
           for( idx = 0; idx < manager->size; idx++ )
              if( manager->array[idx] != NULL )
                manager->array[idx] = ak_context_node_delete( manager->array[idx] );

           /* очищаем и уничтожаем память */
           memset( manager->array, 0, manager->size);
           free( manager->array );
           manager->array = NULL;
  }
  manager->size = 0;

 /* удаляем генератор ключей */
  if(( error = ak_random_context_destroy( &manager->key_generator )) != ak_error_ok )
    return ak_error_message( error, __func__ , "using a null pointer to random key generator" );

 return error;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция выделяет дополнительную память под массив хранения контекстов.
    При выделении памяти производится перенос содержащихся в массиве указателей
    (элементов структуры управления контекстами) в новую область памяти.
    Старая область памяти уничтожается.

    При выделении памяти размер новой области увеличивается в два раза, по сравнению с предыдущим
    объемом, то есть происходит двукратное увеличение. Максимальное число
    хранимых в структуре управления контекстов является внешним параметром библиотеки.
    Данное значение устанавливается в файле `libakrypt.conf` (см. раздел \ref construction_options).

    @param manager Указатель на структуру управления ключами
    @return В случае успеха функция возвращает \ref ak_error_ok (ноль). В противном случае
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 int ak_context_manager_morealloc( ak_context_manager manager )
{
  size_t idx, newsize , msize = ( size_t )ak_libakrypt_get_option("context_manager_max_size");

  if( manager == NULL ) return ak_error_message( ak_error_null_pointer, __func__ ,
                                            "using a null pointer to context manager structure" );
  if(( newsize = ( manager->size << 1 )) <= manager->size )
    return ak_error_message( ak_error_context_manager_size, __func__ ,
                                      "unexpected value of new value of context manager's size" );
  if( newsize > msize ) return ak_error_message( ak_error_context_manager_max_size, __func__,
                                   "current size of context manager exceeds permissible bounds" );

  if(( manager->array = realloc( manager->array, sizeof( ak_pointer )*newsize )) == NULL )
    return ak_error_message( ak_error_out_of_memory, __func__ ,
                                            "wrong memory allocation for context manager nodes" );
 /* инициализируем новые ячейки массива */
  for( idx = manager->size; idx < newsize; idx++ ) manager->array[idx] = NULL;
  manager->size = newsize;

 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! По заданному значению индекса массива idx функция вычисляет значение дескриптора,
    доступного пользователю. Обратное преобразование задается функцией
    ak_context_manager_handle_to_idx().

    @param manager Указатель на структуру управления контекстами
    @param idx Индекс контекста в массиве
    @return Функция возвращает значение дескриптора контекста.                                     */
/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_context_manager_idx_to_handle( ak_context_manager manager, size_t idx )
{
  if( manager == NULL ) ak_error_message( ak_error_null_pointer, __func__ ,
                                            "using a null pointer to context manager structure" );
 return ( ak_handle )idx;
}

/* ----------------------------------------------------------------------------------------------- */
/*! По заданному значению дескриптора контекста handle функция вычисляет значение
    индекса массива, по адресу которого располагается контекст.
    Обратное преобразование задается функцией ak_context_manager_idx_to_handle().

    @param manager Указатель на структуру управления контекстами
    @param handle Дескриптор контектса
    @return Функция возвращает значение дескриптора контекста.                                     */
/* ----------------------------------------------------------------------------------------------- */
 size_t ak_context_manager_handle_to_idx( ak_context_manager manager, ak_handle handle )
{
  if( manager == NULL ) ak_error_message( ak_error_null_pointer, __func__ ,
                                            "using a null pointer to context manager structure" );
 return ( size_t ) handle;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция находит первый равный NULL адрес элемента структуры и
    помещает по этому адресу новый элемент. В случае, если текущей объем памяти недостаточен для
    размещения нового элемента, происходит выделение нового фрагмента памяти.

    @param manager Указатель на структуру управления контекстами
    @param ctx Контекст, который будет храниться в структуре управленияя контекстами
    @param engine тип контекста: блочный шифр, функия хеширования, массив с данными и т.п.
    @param description пользовательское описание контекста
    @return Функция возвращает дескриптор созданного контекста. В случае
    возникновения ошибки возвращается значение \ref ak_error_wrong_handle. Код ошибки может быть
    получен с помощью вызова функции ak_error_get_value().                                         */
/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_context_manager_add_node( ak_context_manager manager, const ak_pointer ctx,
                                              const oid_engines_t engine, const char *description )
{
  size_t idx = 0;
  int error = ak_error_ok;
  ak_context_node node = NULL;
  ak_handle handle = ak_error_wrong_handle;
  const char *defaultstr = description;

 /* минимальные проверки */
  if( manager == NULL ) {
    ak_error_message( ak_error_null_pointer, __func__, "using a null pointer to context manager" );
    return ak_error_wrong_handle;
  }
  if( ctx == NULL ) {
    ak_error_message( ak_error_null_pointer, __func__, "using a null pointer to context" );
    return ak_error_wrong_handle;
  }
  if( defaultstr == NULL )  defaultstr = "";

 /* блокируем доступ к структуре управления контекстами */
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_lock( &ak_context_manager_mutex );
#endif

 /* ищем свободный адрес */
  for( idx = 0; idx < manager->size; idx++ ) {
     if( manager->array[idx] == NULL ) break;
  }
  if( idx == manager->size ) {
    if(( error =  ak_context_manager_morealloc( manager )) != ak_error_ok ) {
      ak_error_message( error, __func__, "wrong allocation a new memory for context manager" );
      #ifdef LIBAKRYPT_HAVE_PTHREAD
        pthread_mutex_unlock( &ak_context_manager_mutex );
      #endif
      return ak_error_wrong_handle;
    }
  }

 /* адрес найден, теперь размещаем контекст */
  handle = ak_context_manager_idx_to_handle( manager, idx );
  if(( node = ak_context_node_new( ctx, handle, engine, defaultstr )) == NULL ) {
    ak_error_message( ak_error_get_value(), __func__, "wrong creation of context manager node" );
    #ifdef LIBAKRYPT_HAVE_PTHREAD
     pthread_mutex_unlock( &ak_context_manager_mutex );
    #endif
    return ak_error_wrong_handle;
  }
  manager->array[idx] = node;
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_unlock( &ak_context_manager_mutex );
#endif

 return handle;
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_context_manager_delete_node( ak_context_manager manager, ak_handle handle )
{
  size_t idx = 0;
  int error = ak_error_ok;

  if( manager == NULL ) return ak_error_message( ak_error_null_pointer, __func__,
                                                      "using a null pointer to context manager" );
 /* получаем индекс из значения дескриптора */
  if(( error = ak_context_manager_handle_check( manager, handle, &idx )) != ak_error_ok )
    return ak_error_message( error, __func__, "incorrect handle" );

 /* блокируем доступ и удаляем объект */
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_lock( &ak_context_manager_mutex );
#endif
  manager->array[idx] = ak_context_node_delete( manager->array[idx] );
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_unlock( &ak_context_manager_mutex );
#endif
 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция проверяет, что внутренний массив контекстов содержит в себе отличный от NULL контекст
    с заданным значеним дескриптора ключа. Функция не экспортируется.

    @param manager Контекст структуры управления контекстами.
    @param handle Дескриптор контекста.
    @param idx Указатель на индекс контекста в массиве контекстов.
    @return В случае ошибки возвращается ее код. В случае успеха, возвращается значение
    \ref ak_error_ok                                                                               */
/* ----------------------------------------------------------------------------------------------- */
 int ak_context_manager_handle_check( ak_context_manager manager, ak_handle handle, size_t *idx )
{
 /* проверяем менеджер контекстов */
  if( manager == NULL ) return ak_error_message( ak_error_null_pointer, __func__,
                                                      "using null pointer to context manager" );
 /* проверяем, что значение handle определено */
  if( handle == ak_error_wrong_handle ) return ak_error_message( ak_error_wrong_handle,
                                                  __func__, "using an undefined handle value" );
 /* определяем индекс */
  *idx = ak_context_manager_handle_to_idx( manager, handle );

 /* проверяем границы */
  if( *idx >= manager->size )
    return ak_error_message( ak_error_wrong_handle, __func__, "invalid handle index" );

 /* проверяем наличие node */
  if( manager->array[*idx] == NULL )
    return ak_error_message( ak_error_null_pointer, __func__,
                                               "using a null pointer to context manager node" );
 /* проверяем наличие контекста */
  if( manager->array[*idx]->ctx == NULL )
    return ak_error_message( ak_error_null_pointer, __func__, "using null pointer to context" );

 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
/*                                 теперь глобальный ak_context_manager                            */
/* ----------------------------------------------------------------------------------------------- */
 static ak_context_manager libakrypt_manager = NULL;

/* ----------------------------------------------------------------------------------------------- */
 int ak_libakrypt_create_context_manager( void )
{
  int error = ak_error_ok;

 /* блокируем доступ */
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_lock( &ak_context_manager_mutex );
#endif

 /* если уже создан, то выходим с уведомлением об ошибке */
  if( libakrypt_manager != NULL ) {
   #ifdef LIBAKRYPT_HAVE_PTHREAD
    pthread_mutex_unlock( &ak_context_manager_mutex );
   #endif
    return ak_error_message( ak_error_context_manager_usage, __func__,
                                                     "trying to create existing context manager" );
  }

  if(( libakrypt_manager = malloc( sizeof( struct context_manager ))) == NULL )
    ak_error_message( error = ak_error_out_of_memory, __func__,
                                                   "wrong memory allocation for context manager" );
  else {
         if(( error = ak_context_manager_create( libakrypt_manager )) != ak_error_ok )
           ak_error_message( error, __func__, "incorrect initialization of context manager" );
       }

 /* разблокируем доступ */
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_unlock( &ak_context_manager_mutex );
#endif

 if(( error == ak_error_ok ) && ( ak_log_get_level() >= ak_log_maximum ))
   ak_error_message( ak_error_ok, __func__ , "creation of context manager is Ok");

 return error;
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_libakrypt_destroy_context_manager( void )
{
  int error = ak_error_ok;

 /* блокируем доступ */
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_lock( &ak_context_manager_mutex );
#endif

  if( libakrypt_manager == NULL ) {
   #ifdef LIBAKRYPT_HAVE_PTHREAD
    pthread_mutex_unlock( &ak_context_manager_mutex );
   #endif
    return ak_error_message( ak_error_null_pointer, __func__ ,
                                                 "destroying a null pointer to context manager" );
  }
  if(( error = ak_context_manager_destroy( libakrypt_manager )) != ak_error_ok )
    ak_error_message( error, __func__, "wrong destroing of context manager" );

  free( libakrypt_manager );

 /* разблокируем доступ */
#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_unlock( &ak_context_manager_mutex );
#endif

 if(( error == ak_error_ok ) && ( ak_log_get_level() >= ak_log_maximum ))
   ak_error_message( ak_error_ok, __func__ , "destroying of context manager is Ok");
 return error;
}

/* ----------------------------------------------------------------------------------------------- */
 ak_context_manager ak_libakrypt_get_context_manager( void )
{
 ak_context_manager result = NULL;

 #ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_lock( &ak_context_manager_mutex );
 #endif

  result = libakrypt_manager;

#ifdef LIBAKRYPT_HAVE_PTHREAD
  pthread_mutex_unlock( &ak_context_manager_mutex );
#endif

  if( !result )
    ak_error_message( ak_error_null_pointer, __func__, "using null pointer to context manager" );
 return result;
}

/* ----------------------------------------------------------------------------------------------- */
/*! Для существующего контекста, под который ранее выделена память с помощью вызова malloc(),
    функция создает его дескриптор, и размещает контекст в глобальной структуре управления контекстами.
    Данная функция используется производящими функциями пользовательского интерфейса.

    \b Внимание! В случае возникновения ошибки помещаемый контекст уничтожается.

    @param ctx контекст объекта.
    @param engine тип контекста: блочный шифр, функия хеширования, массив с данными и т.п.
    @param description пользовательское описание контекста
    @return Функция возвращает дескриптор созданного контекста. В случае
    возникновения ошибки возвращается значение \ref ak_error_wrong_handle. Код ошибки может быть
    получен с помощью вызова функции ak_error_get_value().                                         */
/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_libakrypt_add_context( ak_pointer ctx,
                                               const oid_engines_t engine, const char *description )
{
  ak_context_manager manager = NULL;
  ak_handle handle = ak_error_wrong_handle;
  ak_oid oid = ak_context_node_get_context_oid( ctx, engine );

 /* проверяем входные данные */
  if( oid == NULL ) {
    ak_error_message( ak_error_get_value(), __func__, "using incorrect context or engine" );
    return ak_error_wrong_handle;
  }

 /* получаем доступ к структуре управления контекстами */
  if(( manager = ak_libakrypt_get_context_manager()) == NULL ) {
    ak_error_message( ak_error_get_value(), __func__ , "using a non initialized context manager" );
    if( oid->func.delete != NULL ) {
      if( ctx != NULL )
        ctx = (( ak_function_free_object *) oid->func.delete )( ctx );
    }
    return ak_error_wrong_handle;
  }

 /* создаем элемент структуры управления контекстами */
  if(( handle = ak_context_manager_add_node(
                                  manager, ctx, engine, description )) == ak_error_wrong_handle ) {
    ak_error_message( ak_error_get_value(), __func__ , "wrong creation of context manager node" );
    if( oid->func.delete != NULL ) {
      if( ctx != NULL )
        ctx = (( ak_function_free_object *) oid->func.delete )( ctx );
    }
    return ak_error_wrong_handle;
  }

 return handle;
}


/* ----------------------------------------------------------------------------------------------- */
/*                              Функции для работы с дескрипторами                                 */
/* ----------------------------------------------------------------------------------------------- */
/*! \param ni Строка символов, которая определяет криптографическое преобразование.
    Это может быть одно из допустимых имен или идентификатор преобразования.
    \param description Произвольная строка символов, которой пользователь может описать
    криптографичекое преобразование. Как правило, используется для хранения комментариев к
    секретным или открытым ключам. Допускается использование значения NULL.
    \return Дескриптор криптографического преобразования. В случае ошибки возвращается
    значение \ref ak_error_wrong_handle. Код ошибки может быть получен с помощью
    функции ak_error_get_value().                                                                  */
/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_handle_new( const char *ni, const char *description )
{
  ak_pointer ctx = NULL;
  int error = ak_error_ok;
  ak_oid oid = ak_oid_context_find_by_ni( ni );

 /* проверяем входные параметры */
  if( oid == NULL ) {
    ak_error_message( ak_error_get_value(), __func__ ,
                              "incorrect value of name/identifier (object identifier not found)" );
    return ak_error_wrong_handle;
  }

 /* определяем тип, создаем и инициализируем контекст криптографического преобразования */
  switch( oid->engine ) {

    case hash_function:
      if(( ctx = malloc( sizeof( struct hash ))) == NULL ) {
        ak_error_message( ak_error_out_of_memory, __func__,
                                         "incorrect allocation memory for hash function context" );
        return ak_error_wrong_handle;
      }
      if(( error = ((ak_function_hash_context_create *)oid->func.create)( ctx )) != ak_error_ok ) {
        free( ctx );
        ak_error_message( error, __func__, "incorrect creation of hash function context" );
        return ak_error_wrong_handle;
      }
      break;

    case hmac_function:
      if(( ctx = malloc( sizeof( struct hmac ))) == NULL ) {
        ak_error_message( ak_error_out_of_memory, __func__,
                                         "incorrect allocation memory for hmac function context" );
        return ak_error_wrong_handle;
      }
      if(( error = ((ak_function_hash_context_create *)oid->func.create)( ctx )) != ak_error_ok ) {
        free( ctx );
        ak_error_message( error, __func__, "incorrect creation of hmac function context" );
        return ak_error_wrong_handle;
      }
      break;

    case block_cipher:
    case random_generator:

    default: ak_error_message( ak_error_wrong_oid, __func__,
                                                        "object identifier has incorrect engine" );
      return ak_error_wrong_handle;
  }

 /* помещаем контекст в менеджер контекстов и возвращаем полученный дескриптор */
  return ak_libakrypt_add_context( ctx, oid->engine, description );
}

/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_handle_new_streebog256( void ) { return ak_handle_new( "streebog256", NULL ); }

/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_handle_new_streebog512( void ) { return ak_handle_new( "streebog512", NULL ); }

/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_handle_new_hmac_streebog256( void )
{
  return ak_handle_new( "hmac-streebog256", NULL );
}

/* ----------------------------------------------------------------------------------------------- */
 ak_handle ak_handle_new_hmac_streebog512( void )
{
  return ak_handle_new( "hmac-streebog512", NULL );
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_handle_delete( ak_handle handle )
{
  int error = ak_error_ok;
  ak_context_manager manager = NULL;

 /* получаем доступ к структуре управления контекстами */
  if(( manager = ak_libakrypt_get_context_manager()) == NULL )
    return ak_error_message(
                      ak_error_get_value(), __func__ , "using a non initialized context manager" );

 /* уничтожаем контекст */
  if(( error = ak_context_manager_delete_node( manager, handle )) != ak_error_ok )
    ak_error_message( error, __func__, "incorrect context destruction" );
 return error;
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_handle_get_oid( ak_handle handle, oid_engines_t *engine, oid_modes_t *mode,
                                                           const char **oid, const char ***names )
{
  size_t idx = 0;
  int error = ak_error_ok;
  ak_context_manager manager = NULL;

 /* получаем доступ к структуре управления контекстами */
  if(( manager = ak_libakrypt_get_context_manager()) == NULL )
    return ak_error_message( ak_error_get_value(), __func__ ,
                                                       "using a non initialized context manager" );

  if(( error = ak_context_manager_handle_check( manager, handle, &idx )) != ak_error_ok )
    return ak_error_message( error, __func__, "wrong handle" );

  if( manager->array[idx]->oid == NULL )
    return ak_error_message( ak_error_null_pointer, __func__,
                                                  "using null pointer in internal context node" );

  *engine = manager->array[idx]->oid->engine;
  *mode = manager->array[idx]->oid->mode;
  *oid =  manager->array[idx]->oid->id;
  *names = manager->array[idx]->oid->names;

 return ak_error_ok;
}

/* ----------------------------------------------------------------------------------------------- */
 bool_t ak_handle_has_tag( ak_handle handle )
{
  size_t idx = 0;
  int error = ak_error_ok;
  ak_context_manager manager = NULL;

 /* получаем доступ к структуре управления контекстами */
  if(( manager = ak_libakrypt_get_context_manager()) == NULL ) {
    ak_error_message( ak_error_get_value(), __func__ , "using a non initialized context manager" );
    return ak_false;
  }

  if(( error = ak_context_manager_handle_check( manager, handle, &idx )) != ak_error_ok ) {
    ak_error_message( error, __func__, "wrong handle" );
    return ak_false;
  }

 /* возвращаем ответ */
  switch( manager->array[idx]->oid->engine ) {
    case hash_function:
    case hmac_function:
    case block_cipher:
    case omac_function:
    case mgm_function:
    case sign_function:
     return ak_true;

    default: return ak_false;
  }
}

/* ----------------------------------------------------------------------------------------------- */
 size_t ak_handle_get_tag_size( ak_handle handle )
{
  size_t idx = 0;
  int error = ak_error_ok;
  ak_context_manager manager = NULL;

 /* получаем доступ к структуре управления контекстами */
  if(( manager = ak_libakrypt_get_context_manager()) == NULL ) {
    ak_error_message( ak_error_get_value(), __func__ , "using a non initialized context manager" );
    return 0;
  }

  if(( error = ak_context_manager_handle_check( manager, handle, &idx )) != ak_error_ok ) {
    ak_error_message( error, __func__, "wrong handle" );
    return 0;
  }

 /* возвращаем ответ */
  switch( manager->array[idx]->oid->engine ) {
    case hash_function:
      return (( ak_hash )manager->array[idx]->ctx )->data.sctx.hsize;

    case hmac_function:
      return (( ak_hmac )manager->array[idx]->ctx )->ctx.data.sctx.hsize;

    case block_cipher:
      return (( ak_bckey )manager->array[idx]->ctx )->bsize;

    default:
      ak_error_message( ak_error_wrong_oid, __func__, "this handle has'nt tag" );
      return 0;
  }
}

/* ----------------------------------------------------------------------------------------------- */
 ak_pointer ak_handle_get_context( ak_handle handle, ak_oid *oid )
{
  size_t idx = 0;
  int error = ak_error_ok;
  ak_context_manager manager = NULL;

 /* получаем доступ к структуре управления контекстами */
  if(( manager = ak_libakrypt_get_context_manager()) == NULL ) {
    ak_error_message( ak_error_get_value(), __func__ , "using a non initialized context manager" );
    return NULL;
  }

  if(( error = ak_context_manager_handle_check( manager, handle, &idx )) != ak_error_ok ) {
    ak_error_message( error, __func__, "wrong handle" );
    return NULL;
  }

  *oid = manager->array[idx]->oid;
 return manager->array[idx]->ctx;
}

/* ----------------------------------------------------------------------------------------------- */
 bool_t ak_handle_has_key( ak_handle handle )
{
  ak_oid oid = NULL;
  ak_pointer ctx = NULL;

 /* получаем данные */
  if(( ctx = ak_handle_get_context( handle, &oid )) == NULL ) {
    ak_error_message( ak_error_get_value(), __func__, "incorect handle value" );
    return ak_false;
  }

 /* возвращаем ответ */
  switch( oid->engine ) {
    case block_cipher:
    case stream_cipher:
    case hybrid_cipher:
    case hmac_function:
    case omac_function:
    case mgm_function:
    case sign_function:
     return ak_true;

    default: return ak_false;
  }
}

/* ----------------------------------------------------------------------------------------------- */
/*! \param handle Дескриптор криптографического алгоритма.
    \param hexstr Строка (null-строка), содержащая шестнадцетиричное представление области памяти,
    содержащей ключевое значение
    \param reverse Флаг, при истинном значении которого данные, после преобразования
    из строки симовлов, будут побайтно развернуты.

    \return В случае успеха функция возвращает ноль (\ref ak_error_ok). В противном случае
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 int ak_handle_set_key_from_hexstr( ak_handle handle, const char *hexstr, const bool_t reverse )
{
  ak_oid oid = NULL;
  ak_uint64 key[64];
  struct random rnd;
  ak_pointer ctx = NULL;
  int error = ak_error_ok;

  if(( ctx = ak_handle_get_context( handle, &oid )) == NULL )
    return ak_error_message( ak_error_get_value(), __func__, "incorect handle value" );

  if(( error = ak_hexstr_to_ptr( hexstr, key, sizeof( key ), reverse )) != ak_error_ok )
    return ak_error_message( error, __func__, "incorrect hexademal string with secret key value" );

 /* присваиваем ключ */
  switch( oid->engine ) {
    case hmac_function:
      error = ak_hmac_context_set_key( ctx, key, 64 );
      break;

    case block_cipher:
      error = ak_bckey_context_set_key( ctx, key, 32 );
      break;

    default: error = ak_error_message( ak_error_wrong_oid, __func__,
                                                            "this handle not accept a key value" );
  }

 /* очищаем временную переменную */
  if( ak_random_context_create_lcg( &rnd ) != ak_error_ok ) {
    memset( key, 0, sizeof( key ));
  } else {
           ak_ptr_context_wipe( key, sizeof( key ), &rnd );
           ak_random_context_destroy( &rnd );
         }

 return error;
}

/* ----------------------------------------------------------------------------------------------- */
 int ak_handle_set_key_from_password( ak_handle handle,
                                           const ak_pointer pass, const size_t pass_size,
                                                     const ak_pointer salt, const size_t salt_size )
{
  ak_oid oid = NULL;
  ak_pointer ctx = NULL;
  int error = ak_error_ok;

 /* получаем контекст */
  if(( ctx = ak_handle_get_context( handle, &oid )) == NULL )
    return ak_error_message( ak_error_get_value(), __func__, "incorect handle value" );

 /* присваиваем ключ */
  switch( oid->engine ) {
    case hmac_function:
      error = ak_hmac_context_set_key_from_password( ctx, pass, pass_size, salt, salt_size );
      break;

    case block_cipher:
      error = ak_bckey_context_set_key_from_password( ctx, pass, pass_size, salt, salt_size );
      break;

    default: error = ak_error_message( ak_error_wrong_oid, __func__,
                                                            "this handle not accept a key value" );
  }
 return error;
}

/* ----------------------------------------------------------------------------------------------- */
/*! \param handle Дескриптор криптографического алгоритма.
    \param filename Имя файла, для которого вычисляется хеш-код.
    \param out Область памяти, куда будет помещен результат. Память должна быть заранее выделена.
    Размер выделенной памяти должен быть не менее значения, возвращаемого
    функцией ak_handle_get_tag_size().
    \param out_size Размер области памяти (в октетах), в которую будет помещен результат.
    Если данное значение меньше, чем необходимо, то будет возвращена ошибка.

    \return В случае успеха функция возвращает ноль (\ref ak_error_ok). В противном случае
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 int ak_handle_mac_file( ak_handle handle, const char *filename,
                                                            ak_pointer out, const size_t out_size )
{
  ak_oid oid = NULL;
  ak_pointer ctx = NULL;

  if(( ctx = ak_handle_get_context( handle, &oid )) == NULL )
    return ak_error_message( ak_error_get_value(), __func__, "incorect handle value" );
  if( oid->mode != algorithm )
    return ak_error_message( ak_error_oid_mode, __func__, "using handle with wrong mode" );

 /* для тех, кто умеет, возвращаем результат */
   switch( oid->engine )
  {
    case hash_function: return ak_hash_context_file( ctx, filename, out, out_size );
    case hmac_function: return ak_hmac_context_file( ctx, filename, out, out_size );

   /* для остальных возвращаем ошибку */
    default:
        return ak_error_message( ak_error_oid_engine, __func__, "using handle with wrong engine" );
  }
}

/* ----------------------------------------------------------------------------------------------- */
/*! Функция позволяет вычислить значение хэш-кода (контрольную сумму) для бесключевых функций
    хэширования или значение имитовставки - для ключевых функций хэширования.

    \param handle Дескриптор криптографического алгоритма.
    \param in Указатель на входные данные для которых вычисляется контрольная сумма.
    \param size Размер входных данных в байтах.
    \param out Область памяти, куда будет помещен результат. Память должна быть заранее выделена.
    Размер выделенной памяти должен быть не менее значения, возвращаемого
    функцией ak_handle_get_tag_size().
    \param out_size Размер области памяти (в октетах), в которую будет помещен результат.
    Если данное значение меньше, чем необходимо, то будет возвращена ошибка.

    \return В случае успеха функция возвращает ноль (\ref ak_error_ok). В противном случае
    возвращается код ошибки.                                                                       */
/* ----------------------------------------------------------------------------------------------- */
 int ak_handle_mac_ptr( ak_handle handle, ak_pointer in, const size_t size,
                                                             ak_pointer out, const size_t out_size )
{
  ak_oid oid = NULL;
  ak_pointer ctx = NULL;

  if(( ctx = ak_handle_get_context( handle, &oid )) == NULL )
    return ak_error_message( ak_error_get_value(), __func__, "incorect handle value" );
  if( oid->mode != algorithm )
    return ak_error_message( ak_error_oid_mode, __func__, "using handle with wrong mode" );

 /* для тех, кто умеет, возвращаем результат */
   switch( oid->engine )
  {
    case hash_function: return ak_hash_context_ptr( ctx, in, size, out, out_size );
    case hmac_function: return ak_hmac_context_ptr( ctx, in, size, out, out_size );

   /* для остальных возвращаем ошибку */
    default:
        return ak_error_message( ak_error_oid_engine, __func__, "using handle with wrong engine" );
  }
}

/* ----------------------------------------------------------------------------------------------- */
/*! \example test-context-node.c                                                                   */
/*! \example test-context-manager.c                                                                */
/* ----------------------------------------------------------------------------------------------- */
/*                                                                           ak_context_manager.c  */
/* ----------------------------------------------------------------------------------------------- */
