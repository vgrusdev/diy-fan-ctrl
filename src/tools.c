#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <sys/time.h>

#include "tools.h"

// #define DEBUG 1

sqlite3 *db_init(char *dbname) {

   sqlite3 *db;
   char *sql;
   char *err_msg = 0;
   int rc;

   if ((rc = sqlite3_open(dbname, &db)) != SQLITE_OK ) {
      fprintf(stderr, "db_init: Can't open database: \'%s\': %s.\n", dbname, sqlite3_errmsg(db));

      sqlite3_close(db);
      return(NULL);
   }
   fprintf(stderr, "db_init: Database opened successfully: \'%s\'.\n", dbname);

   sql = "DROP TABLE IF EXISTS Fan;"
         "CREATE TABLE Fan(id INTEGER PRIMARY KEY, \
	                   app_state INTEGER NOT NULL, \
                           switch_state BOOLEAN NOT NULL CHECK (switch_state IN (0,1)), \
			   rhum_value FLOAT NOT NULL, \
			   rhum_state BOOLEAN NOT NULL CHECK (rhum_state IN (0,1)), \
                           fan_state BOOLEAN NOT NULL CHECK (fan_state IN (0,1)), \
			   i2cerror INTEGER NOT NULL);"
         "INSERT INTO  Fan (id, app_state, \
	 			switch_state, \
				rhum_value, \
				rhum_state, \
				fan_state, \
				i2cerror) \
			VALUES(1, 0, \
				  0, \
				  0.0, \
				  0, \
				  0, \
				  0);";

   if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
     fprintf(stderr, "db_init: Failed to create Fan table: %s.\n", err_msg);

     sqlite3_free(err_msg);
     sqlite3_close(db);
     return(NULL);
   }
   fprintf(stderr, "db_init: Fan table created successfully.\n");

   return(db);
}

int make_sql_update(struct io_dev * io_dev, char* column, int value) {

  int size = 0;
  char *sql = NULL;
  char *err_msg;

  const char *fmt = "UPDATE Fan SET %s=%d where id=1;";

           /* Determine required size */

  if ((size = snprintf(NULL, 0, fmt, column, value)) < 0)
    return -1;

  size += 1;             /* ++ For '\0' */
  if ((sql = malloc(size)) == NULL) {
    fprintf(stderr, "make_sql_update: malloc() error\n");
    return -1;
  }

  if ((size = snprintf(sql, size, fmt, column, value)) < 0) {
    free(sql);
    return -1;
  }

  if ((size = sqlite3_exec(io_dev->db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
     fprintf(stderr, "Failed to update Fan table: %s\n", err_msg);
     fprintf(stderr, "  SQL = \'%s\'\n", sql);
     free(sql);
     sqlite3_free(err_msg);
     return -1;
  }
  free(sql);

  return 0;
}

int sw_fan(struct io_dev * io_dev, int fan_state) {

  int ret;
  int pin_state;

  pin_state = ((fan_state == OFF) ? 1 : 0);

  if ((ret = (int)pinWrite (io_dev->dev, io_dev->pin_fan, pin_state)) < 0){
    pinWrite (io_dev->dev, io_dev->pin_fan, OFF);
    fprintf(stderr, "sw_fan: i2c write error\n");
    report_i2c_error(io_dev, ret);
    return -1;
  }
  return fan_state;
}


int set_fan_state(struct io_dev * io_dev, struct app_state * app_state, int fan_state) {

  int    ret  = 0;;
  int    rslt = 0;
  
  if (fan_state != app_state->cur_fan_state) {
    app_state->cur_fan_state = fan_state;
    if ((rslt = make_sql_update(io_dev, "fan_state", fan_state)) < 0) {
      ret = -2;
    }
  }

  if ((rslt = sw_fan(io_dev, fan_state)) < 0) {
      ret = -1;
    }

  if (ret == 0) ret = fan_state;

  return ret;
}


void set_fan_cycles(struct app_state * app_state) {
  app_state->fan_cycles = FAN_CYCLES1;
}

void set_fan_cycles_h(struct app_state * app_state) {
  app_state->fan_cycles = 1;
}

	
int set_new_state(struct io_dev * io_dev, struct app_state * app_state, int new_state) {

  int    ret;

  app_state->cur_app_state = new_state;
  gettimeofday(&(app_state->app_state_time), NULL);

  if ((ret = make_sql_update(io_dev, "app_state", new_state)) < 0) {
    return -2;
  }
  return new_state;
}


static int rhum_callback(void *data, int argc, char **argv, char **azColName){

   float rhum;

   if (argc >= 2) {
     rhum = (argv[1] != NULL) ? atof(argv[1]) : -1.0;
   } else {
     rhum = -1.0;
   }
   ((float*)data)[0] = rhum;
   return 0;
}

float get_rhum(struct io_dev * io_dev) {

  const char * sql = "SELECT id, rhum from Env where id=1;";
  int rc;
//  char *err_msg;
  float rhum;

  if ((rc = sqlite3_exec(io_dev->db, sql, rhum_callback, (void*)&rhum, NULL)) != SQLITE_OK ) {

//     fprintf(stderr, "get_rhum SELECT error: %s\n", err_msg);
//     sqlite3_free(err_msg);

     return -2.0;
   }
   return rhum;
}

int make_rhum_update(struct io_dev * io_dev, char* column, float value) {

  int size = 0;
  char *sql = NULL;
  char *err_msg;

  const char *fmt = "UPDATE Fan SET %s=%f where id=1;";

           /* Determine required size */

  if ((size = snprintf(NULL, 0, fmt, column, value)) < 0)
    return -3;

  size += 1;             /* ++ For '\0' */
  if ((sql = malloc(size)) == NULL) {
#ifdef DEBUG
    fprintf(stderr, "make_rhum_update: malloc() error\n");
#endif
    return -3;
  }

  if ((size = snprintf(sql, size, fmt, column, value)) < 0) {
    free(sql);
    return -3;
  }

  if ((size = sqlite3_exec(io_dev->db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
#ifdef DEBUG
     fprintf(stderr, "Failed to update Fan table: %s\n", err_msg);
     fprintf(stderr, "  SQL = \'%s\'\n", sql);
#endif
     free(sql);
     sqlite3_free(err_msg);
     return -2;
  }
  free(sql);

  return 0;
}

int get_rhum_state(struct io_dev * io_dev, struct app_state * app_state) {

  float rhum;
  int ret;

  if ((rhum = get_rhum(io_dev)) < 0) { return -3; }

  if ( rhum > RHUM_HIGH_LEVEL) {
    if (app_state->rhum_state == OFF) {
      app_state->rhum_state = ON;
      if ((ret = make_sql_update(io_dev, "rhum_state", ON)) < 0) {
        return -2;
      }
      if ((ret = make_rhum_update(io_dev, "rhum_value", rhum)) < 0) {
        return -2;
      }
    } else {}
  }
  if ( rhum < RHUM_LOW_LEVEL) {
    if (app_state->rhum_state == ON) {
      app_state->rhum_state = OFF;
      if ((ret = make_sql_update(io_dev, "rhum_state", OFF)) < 0) {
        return -2;
      }
      if ((ret = make_rhum_update(io_dev, "rhum_value", rhum)) < 0) {
        return -2;
      }
    } else {}
  }
  return app_state->rhum_state;
}    /* get_rhum_state() */


int get_switch_state(struct io_dev * io_dev, struct app_state * app_state) { 

  int ret;
  int new_sw;

  if ((new_sw = (int)pinRead (io_dev->dev, io_dev->pin_sw)) < 0) {
    fprintf(stderr, "get_sw: i2c read error.\n");
    report_i2c_error(io_dev, new_sw);
    return -1;
  } 

  if (new_sw == 0) new_sw = ON;
  else             new_sw = OFF;
#ifdef DEBUG
printf ("------> get_switch_state: new_sw = %d\n", new_sw);
printf ("------> get_switch_state: app_state->switch_state = %d\n", app_state->switch_state);
#endif

  if (new_sw != app_state->switch_state) {
    app_state->switch_state = new_sw;
    gettimeofday(&(app_state->switch_state_time), NULL);
#ifdef DEBUG
printf ("get_switch_state:  new_sw = %d\n", new_sw);
#endif
    if ((ret = make_sql_update(io_dev, "switch_state", new_sw)) < 0) {
      return -1;
    }
  }
  return new_sw;
}

void set_fan_start_time(struct app_state * app_state) {

  struct timeval delay;
  struct timeval currtime;

  gettimeofday(&currtime, NULL);

  delay.tv_sec  = FAN_START_DELAY_SEC;
  delay.tv_usec = 0;

  timeradd(&currtime, &delay, &(app_state->fan_start_time));
//  timeradd(&(app_state->app_state_time), &delay, &(app_state->fan_start_time));

}

void set_fan_start_time_h(struct app_state * app_state) {

  struct timeval delay;
  struct timeval currtime;

  gettimeofday(&currtime, NULL);

  delay.tv_sec  = FAN_START_DELAY_SEC_H;
  delay.tv_usec = 0;

  timeradd(&currtime, &delay, &(app_state->fan_start_time));
//  timeradd(&(app_state->app_state_time), &delay, &(app_state->fan_start_time));

}


void set_fan_stop_time(struct app_state * app_state) {

  struct timeval delay;
  struct timeval currtime;

  gettimeofday(&currtime, NULL);

  delay.tv_sec  = FAN_STOP_DELAY_SEC;
  delay.tv_usec = 0;

  timeradd(&currtime, &delay, &(app_state->fan_stop_time));
//  timeradd(&(app_state->app_state_time), &delay, &(app_state->fan_stop_time));

}

struct io_dev* io_init( char *i2c_dev, sqlite3 *db, int timeinterval, int pin_sw, int pin_fan) {

  struct io_dev * io_dev;

  if ((io_dev = (struct io_dev*) malloc(sizeof(struct io_dev))) == NULL) {
    fprintf(stderr, "io_init: No memory\n");
    return NULL;
  }


  if ((io_dev->dev = pcf8575_init(i2c_dev, 0x20)) == NULL) {
    fprintf(stderr, "io_init: i2c init error\n");
    free(io_dev);
    return NULL;
  }

  io_dev->db      = db;
  io_dev->timeinterval = timeinterval;
  io_dev->pin_sw  = pin_sw;
  io_dev->pin_fan = pin_fan;

  setPinMode(io_dev->dev, io_dev->pin_sw,  INPUT);
  setPinMode(io_dev->dev, io_dev->pin_fan, OUTPUT);

  if (sw_fan(io_dev, OFF) != 0) {
    fprintf(stderr, "io_init: w_fan->OFF error\n");
    free(io_dev);
    return NULL;
  }
  return io_dev;
}

void report_i2c_error(struct io_dev * io_dev, int errcode) {

  int ret;

  ret = make_sql_update(io_dev, "i2cerror", errcode);
}

