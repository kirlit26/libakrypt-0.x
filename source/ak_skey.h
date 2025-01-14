/* ----------------------------------------------------------------------------------------------- */
/*  Copyright (c) 2014 - 2019 by Axel Kenzo, axelkenzo@mail.ru                                     */
/*                                                                                                 */
/*  Файл ak_skey.h                                                                                 */
/*  - содержит описания функций, предназначенных для хранения и обработки ключевой информации.     */
/* ----------------------------------------------------------------------------------------------- */
#ifndef __AK_SKEY_H__
#define __AK_SKEY_H__

/* ----------------------------------------------------------------------------------------------- */
 #include <ak_random.h>

/* ----------------------------------------------------------------------------------------------- */
#ifdef LIBAKRYPT_HAVE_DEBUG_FUNCTIONS
 #include <stdio.h>
#endif
#ifdef LIBAKRYPT_HAVE_STDALIGN_H
 #include <stdalign.h>
#endif

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Указатель на структуру секретного ключа. */
 typedef struct skey *ak_skey;

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Однопараметрическая функция для проведения действий с секретным ключом, возвращает код ошибки. */
 typedef int ( ak_function_skey )( ak_skey );
/*! \brief Однопараметрическая функция для проведения действий с секретным ключом, возвращает истину или ложь. */
 typedef bool_t ( ak_function_skey_check )( ak_skey );

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Перечисление определяет возможные типы счетчиков ресурса секретного ключа. */
 typedef enum {
  /*! \brief Счетчик числа использованных блоков. */
    block_counter_resource,
  /*! \brief Счетчик числа использований ключа, например, для выработки производных ключей. */
    key_using_resource,
} counter_resource_t;

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Структура определяет тип и значение счетчика ресурса ключа. */
 typedef struct key_resource_counter {
  /*! \brief Тип ресурса */
    counter_resource_t type;
  /*! \brief Дополнение */
    ak_uint8 padding[4];
  /*! \brief Cчетчик числа использований, например, зашифрованных/расшифрованных блоков. */
    ssize_t counter;
} *ak_key_resoure_counter;

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Структура определяет временной интервал действия ключа. */
 typedef struct time_interval {
  /*! \brief Время, до которого ключ недействителен. */
   time_t not_before;
  /*! \brief Время, после которого ключ недействителен. */
   time_t not_after;
} *ak_time_intermal;

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Структура для хранения ресурса ключа. */
 typedef struct resource {
  /*! \brief Счетчик числа использований ключа. */
   struct key_resource_counter value;
  /*! \brief Временной интервал использования ключа. */
   struct time_interval time;
 } *ak_resource;

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Перечисление, определяющее флаги хранения и обработки секретных ключей. */
 typedef ak_uint64 key_flags_t;

/* константные битовые маски, используемые как флаги */
/*! \brief Неопределенное значение */
 #define ak_key_flag_undefined          (0x0000000000000000ULL)

/*! \brief Младший (нулевой) бит отвечает за установку значения ключа:
             0 - ключ не установлен, 1 установлен. */
 #define ak_key_flag_set_key            (0x0000000000000001ULL)

/*! \brief Первый бит отвечает за установку маски:
             0 - маска не установлена, 1 - маска установлена. */
 #define ak_key_flag_set_mask           (0x0000000000000002ULL)

/*! \brief Второй бит отвечает за установку контрольной суммы ключа:
             0 - контрольная сумма не установлена не установлена, 1 - контрольная сумма установлена. */
 #define ak_key_flag_set_icode          (0x0000000000000004ULL)

/*! \brief Третий бит отвечает за то, кто из классов skey или его наследники уничтожают внутреннюю
 память. Если флаг установлен, то класс skey очистку не производит и возлагает это на методы
 классов-наследников: 0 - очистка производится, 1 - очистка памяти не производится */
 #define ak_key_flag_data_not_free      (0x0000000000000008ULL)

/*! \brief Флаг, который запрещает использование функции ctr без указания синхропосылки. */
 #define ak_key_flag_not_ctr            (0x0000000000000100ULL)

/*! \brief Флаг, который определяет, можно ли использовать значение внутреннего буффера в режиме omac. */
 #define ak_key_flag_omac_buffer_used   (0x0000000000000200ULL)

/* ----------------------------------------------------------------------------------------------- */
/*! \brif Способ выделения памяти для хранения секретной информации. */
 typedef enum {
  /*! \brief Механизм выделения памяти не определен. */
   undefined_policy,
  /*! \brief Выделение памяти через стандартный malloc */
   malloc_policy

} memory_allocation_policy_t;

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Структура секретного ключа -- базовый набор данных и методов контроля. */
 struct skey {
  /*! \brief ключ */
#ifdef LIBAKRYPT_HAVE_STDALIGN_H
   alignas(16)
#endif
   ak_uint8 *key;
  /*! \brief размер ключа (в октетах) */
   size_t key_size;
  /*! \brief OID алгоритма для которого предназначен секретный ключ */
   ak_oid oid;
  /*! \brief уникальный номер ключа */
   ak_uint8 number[32];
  /*! \brief контрольная сумма ключа */
   ak_uint32 icode;
  /*! \brief генератор случайных масок ключа */
   struct random generator;
  /*! \brief ресурс использования ключа */
   struct resource resource;
  /*! \brief указатель на внутренние данные ключа */
   ak_pointer data;
 /*! \brief Флаги текущего состояния ключа */
   key_flags_t flags;
 /*! \brief Способ выделения памяти. */
   memory_allocation_policy_t policy;
  /*! \brief указатель на функцию маскирования ключа */
   ak_function_skey *set_mask;
  /*! \brief указатель на функцию демаскирования ключа */
   ak_function_skey *unmask;
  /*! \brief указатель на функцию выработки контрольной суммы от значения ключа */
   ak_function_skey *set_icode;
  /*! \brief указатель на функцию проверки контрольной суммы от значения ключа */
   ak_function_skey_check *check_icode;
};

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Функция выделения памяти для ключевой информации. */
 int ak_skey_context_alloc_memory( ak_skey , size_t , memory_allocation_policy_t );
/*! \brief Функция освобождения выделенной ранее памяти. */
 int ak_skey_context_free_memory( ak_skey );
/*! \brief Инициализация структуры секретного ключа. */
 int ak_skey_context_create( ak_skey , size_t );
/*! \brief Очистка структуры секретного ключа. */
 int ak_skey_context_destroy( ak_skey );
/*! \brief Присвоение секретному ключу уникального номера. */
 int ak_skey_context_set_unique_number( ak_skey );
/*! \brief Присвоение секретному ключу константного значения. */
 int ak_skey_context_set_key( ak_skey , const ak_pointer , const size_t );
/*! \brief Присвоение секретному ключу случайного значения. */
 int ak_skey_context_set_key_random( ak_skey , ak_random );
/*! \brief Присвоение секретному ключу значения, выработанного из пароля */
 int ak_skey_context_set_key_from_password( ak_skey , const ak_pointer , const size_t ,
                                                                 const ak_pointer , const size_t );

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Наложение или смена маски путем сложения по модулю 2 случайной последовательности с ключом. */
 int ak_skey_context_set_mask_xor( ak_skey );
/*! \brief Снятие маски с ключа. */
 int ak_skey_context_unmask_xor( ak_skey );
/*! \brief Вычисление значения контрольной суммы ключа. */
 int ak_skey_context_set_icode_xor( ak_skey );
/*! \brief Проверка значения контрольной суммы ключа. */
 bool_t ak_skey_context_check_icode_xor( ak_skey );

/* ----------------------------------------------------------------------------------------------- */
/*! \brief Функция устанавливает ресурс ключа. */
 int ak_skey_context_set_resource( ak_skey , counter_resource_t , const char * , time_t , time_t );
/*! \brief Функция устанавливает временной интервал действия ключа. */
 int ak_skey_context_set_resource_time( ak_skey skey, time_t not_before, time_t not_after );

/* ----------------------------------------------------------------------------------------------- */
#ifdef LIBAKRYPT_HAVE_DEBUG_FUNCTIONS
/*! \brief Функция выводит информацию о контексте секретного ключа в заданный файл. */
 int ak_skey_context_print_to_file( ak_skey , FILE *fp );
#endif

#endif
/* ----------------------------------------------------------------------------------------------- */
/*                                                                                      ak_skey.h  */
/* ----------------------------------------------------------------------------------------------- */
