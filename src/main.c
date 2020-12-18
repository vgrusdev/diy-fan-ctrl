#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <libgen.h>

#include "tools.h"


int main(int argc, char* argv[]) {

  int ret;
  sqlite3 *db = NULL;
  struct io_dev *io_dev;


// ================================================================== >>>
// Procinit - initialise parameters and demonise if needed.

  int opt;

  char *IIC_Dev = "/dev/i2c-0";
  char *dbfname = NULL;
  char *pidfname = NULL;
  char *dbfname_h = NULL;
  int   timeinterval = 1;
  int   daemonflag = 0;
  int   errflag = 0;

  FILE *pf;     // Pid File
  FILE *fl;	// File logger for daemon mode;
  int   nf;

#define DEF_PID_FILE "/var/run/fan-ctrl.pid"
#define DEF_DBFNAME "/tmp/fan-ctrl.db"

  while ((opt = getopt(argc, argv, "di:f:t:p:h")) != -1) {
    switch(opt) {
      case 'i':
        IIC_Dev = optarg;
        break;
      case 'f':
        dbfname = optarg;
        break;
      case 't':
        timeinterval = atoi(optarg);
        break;
      case 'p':
        pidfname = optarg;
      case 'd':
        daemonflag = 1;
        break;
      case 'h':
	dbfname_h = optarg;
	break;
      case '?':
      default :
        errflag = 1;
        break;
    }
  }

  if (daemonflag) {

    if ((fl = popen("logger -t fan_ctrl","w")) == NULL ) {
        fprintf(stderr, "fan_ctrl: Can not redirect STDOUT to logger\n");
        exit(1);
    }
    nf = fileno(fl);
    dup2(nf,STDOUT_FILENO);
    dup2(nf,STDERR_FILENO);
    close(nf);
    fclose(stdin);
    fflush(stdout);
    if (daemon(0, 1)) {
        fprintf(stderr, "fan_ctrl: Can not daemonise the process\n");
	pclose(fl);
        exit(1);
    }
  }

  if (errflag) {
    fprintf(stderr, "Usage: %s [-i iic_dev] [-f dbfile] [-t interval] [-d] [-p pidfile][-h dbfile-humidity]\n", basename(argv[0]));
    if (daemonflag) pclose(fl);
    exit(1);
  }
  if (daemonflag) {

    if (pidfname == NULL)
      pidfname = DEF_PID_FILE;

    if (dbfname == NULL) {
      dbfname = DEF_DBFNAME;
    }

    fprintf(stderr, "fan_ctrl runs in backgroud\n");
    fprintf(stderr, "  argv[0] = %s\n", basename(argv[0]));
    fprintf(stderr, "  IIC_Dev = %s\n", IIC_Dev);
    fprintf(stderr, "  dbfname = %s\n", (dbfname != NULL) ? dbfname : "N/A");
    fprintf(stderr, "  timeout = %d\n", timeinterval);
    fprintf(stderr, "  daemonize = %s\n", (daemonflag == 1) ? "Yes" : "No");
    fprintf(stderr, "  pidfname = %s\n", (pidfname != NULL) ? pidfname : "N/A");
    fprintf(stderr, "  dbfname_h = %s\n\n", (dbfname_h != NULL) ? dbfname_h : "N/A");

    if ((pf = fopen(pidfname, "w+")) == NULL) {
      fprintf(stderr, "fan_ctrl: pid file open error: %s\n", pidfname);
    } else {
      if (fprintf(pf, "%u", getpid()) < 0) {
        fprintf(stderr, "fan_ctrl: pid file write error: %s\n", pidfname);
      }
      fclose(pf);
    }
  }


// End of Procinit - initialise parameters and demonise if needed.
// <<< ==================================================================


  if ((db = db_init(dbfname)) == NULL) {
    if (daemonflag) pclose(fl);
    exit(1);
  }

  if ((io_dev = io_init(IIC_Dev, db, timeinterval, 9, 10)) == NULL) {
    fprintf(stderr, "fan_ctrl: i2c init error\n");
    sqlite3_close(db);
    if (daemonflag) pclose(fl);
    exit (1);
  }

  ret = fan_main_loop(io_dev);

  fprintf(stderr, "fan_ctrl was dropped out from the main loop with ret = %d\n", ret);
  pcf8575_close(io_dev->dev);
  sqlite3_close(db);
  if (daemonflag) pclose(fl);

  exit (ret);

}

