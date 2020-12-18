#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "pcf8575.h"


int main(int argc, char* argv[]) {

  struct pcf8575_dev *dev;
  int8_t rslt;
  int i;
  uint16_t buf16;

printf("going to pcf8575_init\n");
  if ((dev = pcf8575_init("/dev/i2c-0", 0x20)) == NULL) {
    fprintf(stderr, "pcf-test: i2c init error\n");
    exit (1);
  }

printf("after pcf-init...\n");
fflush(stdout);
  setPinMode(dev, 1, INPUT);
  setPinMode(dev, 2, OUTPUT);

printf("pin mode set done\n");
fflush(stdout);

  fprintf(stderr, "pcf-test: writing LOW level to pin10\n");
  if ((rslt = pinWrite (dev, 2, LOW)) < 0){
    fprintf(stderr, "pcf-test: i2c write error\n");
    exit (2);
  }

i = 20;
while (--i) {

  if ((rslt = pinRead (dev, 1)) < 0) {
    fprintf(stderr, "pcf-test: i2c read error\n");
    exit (2);
  }
  if (buf16Read(dev, &buf16) != 0) {
    fprintf(stderr, "pcf-test: i2c read error\n");
    exit (2);
  }


  printf("Pin 9 = %d, rw_buffer = 0x%04x\n", rslt, buf16);

  sleep(2);

}

  fprintf(stderr, "pcf-test: writing HIGH level to pin10\n");
  if ((rslt = pinWrite (dev, 2, HIGH)) < 0){
    fprintf(stderr, "pcf-test: i2c write error\n");
    exit (2);
  }

i = 20;
while (--i) {

  if ((rslt = pinRead (dev, 1)) < 0) {
    fprintf(stderr, "pcf-test: i2c read error\n");
    exit (2);
  }
  if (buf16Read(dev, &buf16) != 0) {
    fprintf(stderr, "pcf-test: i2c read error\n");
    exit (2);
  }


  printf("Pin 9 = %d, rw_buffer = 0x%04x\n", rslt, buf16);


  sleep(2);

}



/*
  for (i = 1; i <= 16; ++i) {

  setPinMode(dev, i, INPUT);

//  if ((rslt = pinWrite (dev, i, LOW)) < 0){
//    fprintf(stderr, "pcf-test: i2c write error\n");
//    exit (2);
//  }

  if ((rslt = pinRead (dev, i)) < 0) {
    fprintf(stderr, "pcf-test: i2c read error\n");
    exit (2);
  }

  printf("Pin %d = %d, rw_buffer = 0x%x\n", i, rslt, dev->rw_buffer);

//  rslt = pinWrite (dev, i, HIGH);
//  setPinMode(dev, i, INPUT);
  }
*/


/*
  setPinMode(dev, 10, OUTPUT);
  fprintf(stderr, "pcf-test: writing LOW level to pin10\n");
  if ((rslt = pinWrite (dev, 10, LOW)) < 0){
    fprintf(stderr, "pcf-test: i2c write error\n");
    exit (2);
  }
  printf("sleep 5 sec...\n");
  sleep(5);
  printf("press enter to continue...\n");
  i = getchar();
  fprintf(stderr, "pcf-test: writing HIGH level to pin10\n");
  if ((rslt = pinWrite (dev, 10, HIGH)) < 0){
    fprintf(stderr, "pcf-test: i2c write error\n");
    exit (2);
  }
  printf("sleep 5 sec...\n");
  sleep(5);
  printf("press enter to continue...\n");
  i = getchar();
*/

  pcf8575_close(dev);
//  close(dev->iic_fd);
//  free(dev);
  exit (0);
}

