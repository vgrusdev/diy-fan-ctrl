#include <stdio.h>
#include <time.h>

#include <sqlite3.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <libgen.h>

// #include "pcf8575.h"
#include "tools.h"

// #define DEBUG

int fan_main_loop(struct io_dev *io_dev) {

  struct app_state app_state;
//  app_state.cur_app_state :
//
//      0       -  Initial state. FAN stopped (Move out only if SWITCH turned ON). Next:
//			- switch turn ON -> state=1.
//      1       -  SWITCH turned ON, FAN start delayed. Next:
//			- high humidity -> state=2,
//			(- switch turn OFF -> state=0, )
//			- ST_timeout expired -> state=2
//      2       -  SWITCH turned ON or high humidity, FAN turned ON during ON_timeout, then move to state=3, Next:
//			- ON_timeout expired -> state=4
//	3	-  Switch is ON or high humidity, FAN turned OFF during OFF_timeout, then move back to state=2, but
//			if number of cycles expired, then move to state=?? (humidity). Next:
//			- if switch is OFF and low humidity state=0
//			- if  number of cycles expired state=4
//			- if OFF_timeout expired -> state=2
//      4       -  Number of cycles expired. Next:
//			- if switch is OFF state=0
//
//


  int new_switch_state;
  int rhum_state;

// Main loop timeout parameters
  struct timeval steptime;
  struct timeval currtime;
  struct timeval nexttime;
  struct timeval difftime;
// ----------------------------

  float rhum;


  int ret = 0;

  if ((ret = set_new_state(io_dev, &app_state, 0)) < 0) { 
     return -1;
  }
  if ((ret = set_fan_state(io_dev, &app_state, OFF)) < 0) {
    return ret;
  }
  app_state.rhum_state   = OFF;
  app_state.switch_state = OFF;
  gettimeofday(&(app_state.rhum_state_time), NULL);
  gettimeofday(&(app_state.switch_state_time), NULL);


  /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

  steptime.tv_sec  = io_dev->timeinterval;
  steptime.tv_usec = 0;
//  gettimeofday(&currtime, NULL);

  int i;
  i = 2;

  while (true) {

    gettimeofday(&currtime, NULL);
    timeradd(&currtime, &steptime, &nexttime);

    /*    -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-     */
    if ((new_switch_state = get_switch_state(io_dev, &app_state)) < 0) {   // check switch state
      return new_switch_state;
    }
    if ((rhum_state = get_rhum_state(io_dev, &app_state)) < 0) {
        return rhum_state;
    }
    /*    -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-     */


    switch (app_state.cur_app_state) {
    case 0:
//  Initial state. FAN stopped. Switch turned off. Humidity is low.
//  Move out if 
//    - SWITCH turned from OFF -> to ON
//    - HUMIDITY is above high level
//  .. then go to state=1
//

      if (new_switch_state == ON) {	                 // if switch was turned on...
#ifdef DEBUG
printf("Switch is ON now. Moving to state #1\n");
#endif
        if (set_new_state(io_dev, &app_state, 1) < 0) {  // ... then next app_state will be 1
	  return -1;
	}
	set_fan_start_time(&app_state);     // delay to switch on fan
	                                    //   set the time app_state.fan_start_time 
	  				    //   when fan should start 
	  				    //   after switch was turned on, after some dealy
					    //   will be used at app_state 1
	set_fan_cycles(&app_state);

      } else {     /*    (new_switch_state == ON)    */

        if (rhum_state == ON) {		                   // if Humidity is above cliplevel
#ifdef DEBUG
printf("RHUM is upper the limit now. Moving to state #1\n");
#endif
	  if (set_new_state(io_dev, &app_state, 1) < 0) {  // ... then next app_state will be 1
            return -1;
	  }
          set_fan_start_time_h(&app_state); // delay to switch on fan
                                            //   set the time app_state.fan_start_time 
                                            //   when fan should start 
                                            //   after switch was turned on, after some dealy
                                            //   will be used at app_state 1
          set_fan_cycles_h(&app_state);

        }        /*     (rhum_state == ON)       */
      }

      break;

    case 1:
//  SWITCH turned ON, FAN start delayed for the next st_timeout secs. Next state:
      if (new_switch_state == ON) {	    // if switch is still on...
        gettimeofday(&currtime, NULL);
        if timercmp(&currtime, &(app_state.fan_start_time), >=) {   // ..is it the time for fan turning on ?
#ifdef DEBUG
printf("state #1, turning FAN ON and going to state #2... \n");
#endif
	  if ((ret = set_fan_state(io_dev, &app_state, ON)) < 0) {  // switch on the fan
		  return ret;  
	  }
	  if (set_new_state(io_dev, &app_state, 2) < 0) { 
            return -1;
	  }
	  set_fan_stop_time(&app_state);    // set the time app_state.fan_stop_time
	  				    //   when fan should stop for timeout
					    //   it will be used at app_state 2
        } else {}		// ...  still not the time to switch fan on. Initial fan-on timeout.
		 		//        may be depends on day time or other condition.
				//        see function set_function_start_time();

      } else {				    // switch was suddenly switched off before fan start timeout
#ifdef DEBUG
printf("Switch turned OFF. going to state #0\n");
#endif
	if (set_new_state(io_dev, &app_state, 0) < 0) {
          return -1;
	}
      }      /*  (new_switch_state == ON)    */

      break;
    case 2:
//  SWITCH turned ON, FAN turned ON, and should stay during ON until app_state.fan_stop_time.
//  then check number of cyscles....
//        ... if less, then set app_state.fan_start_time and move to state=3
//        ... if higher, then  go to state=4
//
      gettimeofday(&currtime, NULL);
      if timercmp(&currtime, &(app_state.fan_stop_time), >=) {   // ..is it the time for fan turning off ?
#ifdef DEBUG
printf("state #2. turning FAN OFF. Cycles left %d\n", app_state.fan_cycles-1);
#endif
        if ((ret = set_fan_state(io_dev, &app_state, OFF)) < 0) { // switch on the fan
          return ret;
        }
	if (( --app_state.fan_cycles) > 0) {
#ifdef DEBUG
printf("  going to state #1\n");
#endif
	  if (set_new_state(io_dev, &app_state, 1) < 0) {  // ... then next app_state will be 1
            return -1;
	  }
          set_fan_start_time(&app_state);   // delay to switch on fan
                                            //   set the time app_state.fan_start_time
                                            //   when fan should start
                                            //   after switch was turned on, after some dealy
                                            //   will be used at app_state 1
        } else {                            // fan_cycles -> 0
#ifdef DEBUG
printf("  going to state #3\n");
#endif
          if (set_new_state(io_dev, &app_state, 3) < 0) {  // ... then next app_state will be 1
            return -1;
	  }
	}
      }
      break;


    case 3:
      if (new_switch_state == OFF) {         // if switch is still on...
#ifdef DEBUG
printf("state #3, switch is OFF. going to state #0\n");
#endif
	if (set_new_state(io_dev, &app_state, 0) < 0) { // ... then next app_state will be 0
            return -1;
	}
      }

      break;

    case 5:

      break;

    }  /* switch (app_state.cur_app_state)  */




    // ++++++++  Main loop timout calculation +++++++
    gettimeofday(&currtime, NULL);
    if (timercmp(&nexttime, &currtime, >) ) {
      timersub(&nexttime, &currtime, &difftime);
      sleep(difftime.tv_sec);
      usleep(difftime.tv_usec);
    } else {
      usleep(70*1000);
    }
    //  currtime.tv_sec = nexttime.tv_sec;
    //  currtime.tv_usec= nexttime.tv_usec;
  } // while(1) - main loop




  ret = set_fan_state(io_dev, &app_state, ON);
  printf("Called sw_fan(io_dev, ON);   ret = %d\n", ret);
  ret = set_fan_state(io_dev, &app_state, OFF);
  printf("Called sw_fan(io_dev, OFF);  ret = %d\n", ret);
  ret = set_fan_state(io_dev, &app_state, ON);
  printf("Called sw_fan(io_dev, ON);   ret = %d\n", ret);
  ret = set_fan_state(io_dev, &app_state, OFF);
  printf("Called sw_fan(io_dev, OFF);  ret = %d\n", ret);


  ret = get_switch_state(io_dev, &app_state);
  printf("Called get_switch_state(io_dev, &app_state);  ret = %d\n", ret);
  ret = get_switch_state(io_dev, &app_state);
  printf("Called get_switch_state(io_dev, &app_state); ret = %d\n", ret);
  ret = get_switch_state(io_dev, &app_state);
  printf("Called get_switch_state(io_dev, &app_state);  ret = %d\n", ret);
  ret = get_switch_state(io_dev, &app_state);
  printf("Called get_switch_state(io_dev, &app_state); ret = %d\n", ret);

  rhum = get_rhum(io_dev);
  printf("Called get_rhum(io_dev);  rhum = %f\n", rhum);
  rhum = get_rhum(io_dev);
  printf("Called get_rhum(io_dev);  rhum = %f\n", rhum);

  return 0;
}

