#ifndef TOOLS_H
#define TOOLS_H


#include <stdio.h>
#include <sqlite3.h>
#include <sys/time.h>
#include <stdint.h>

/**
    Set USE_STDINT_TYPES to 0 if your platform already has a stdint
    implementation.
*/
#define USE_STDINT_TYPES 0

/**
    Set USE_SHORT_TYPES to 0 if your platform already provides the
    type definitions u8, u16, u32, u64, s8, s16, s32, s64
*/
/* #define USE_SHORT_TYPES 0 */


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef USE_STDINT_TYPES
#define USE_STDINT_TYPES 1
#endif /* USE_STDINT_TYPES */

#ifndef USE_SHORT_TYPES
#define USE_SHORT_TYPES 1
#endif /* USE_SHORT_TYPES */

#if USE_STDINT_TYPES
typedef unsigned long long int uint64_t;
typedef long long int int64_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef char int8_t;
typedef unsigned char uint8_t;
#endif /* USE_STDINT_TYPES */

#if USE_SHORT_TYPES
typedef int64_t s64;
typedef uint64_t u64;
typedef int32_t s32;
typedef uint32_t u32;
typedef int16_t s16;
typedef unsigned short u16;
typedef int8_t s8;
typedef uint8_t u8;
#endif /* USE_SHORT_TYPES */



#include "pcf8575.h"

#ifndef __cplusplus
 typedef char bool;
 #define true 1     
 #define false 0    
#endif

#define OFF 0
#define  ON 1


#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef BIG_ENDIAN
#define be16_to_cpu(s) (s)
#define be32_to_cpu(s) (s)
#define be64_to_cpu(s) (s)
#else
#define be16_to_cpu(s) (((u16)(s) << 8) | (0xff & ((u16)(s)) >> 8))
#define be32_to_cpu(s) (((u32)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((u64)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((u64)be32_to_cpu((s) >> 32))))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif



  sqlite3 *db_init(char *dbname);

  struct io_dev {
    struct pcf8575_dev *dev;
    sqlite3 *db;
    int timeinterval;
    int pin_sw;
    int pin_fan;
  };

  struct app_state {
    int cur_app_state;
    struct timeval app_state_time;
    int cur_fan_state;
    struct timeval fan_start_time;
    struct timeval fan_stop_time;
    int fan_cycles;
    int rhum_state;
    struct timeval rhum_state_time;
    int switch_state;
    struct timeval switch_state_time;
  };


  struct io_dev* io_init( char * i2c_dev, sqlite3 *db, int timeinterval, int pin_sw, int pin_fan);
  int fan_main_loop(struct io_dev *io_dev);

  int set_fan_state(struct io_dev * io_dev, struct app_state * app_state, int fan_state);
  int get_rhum_state(struct io_dev * io_dev, struct app_state * app_state);
  int get_switch_state(struct io_dev * io_dev, struct app_state * app_state);

  void report_i2c_error(struct io_dev * io_dev, int errcode);

  int set_new_state(struct io_dev * io_dev, struct app_state * app_state, int new_state);

  void set_fan_start_time(struct app_state * app_state);
  void set_fan_start_time_h(struct app_state * app_state);
  void set_fan_stop_time(struct app_state * app_state);
  void set_fan_cycles(struct app_state * app_state);
  void set_fan_cycles_h(struct app_state * app_state);

  float get_rhum(struct io_dev * io_dev);

#define FAN_CYCLES1 4
#define FAN_START_DELAY_SEC 20
#define FAN_STOP_DELAY_SEC  180
#define RHUM_LOW_LEVEL 60
#define RHUM_HIGH_LEVEL 80
#define FAN_START_DELAY_SEC_H 5



#endif /* TOOLS_H */
